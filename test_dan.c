#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>  // For printf
#include <math.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"

// Constants and Buffers
#define BUFFER_SIZE 256
char UARTBuffer[BUFFER_SIZE];  // Circular buffer for UART data
char GPGLLData[128];           // String to store extracted $GPGLL sentence
uint16_t writeIdx = 0;         // Circular buffer write index
uint16_t readIdx = 0;          // Circular buffer read index
char acBuffer[128];            // General-purpose buffer

double lat1 = 20.98947517, lon1 = 105.944372;
double lat2 = 20.9896245, lon2 = 105.94459583;

// Function Prototypes
void UARTIntHandler(void);
void Timer0A_Handler(void);
void extractGPGLLData(void);
void processGPSData(const char *sentence);
int validateChecksum(const char *sentence);
double convertToDecimalDegrees(const char *coordinate, char direction);
double calculateDistance(double lat1, double lon1, double lat2, double lon2);

// Function Definitions
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371e3; // Earth's radius in meters
    double phi1 = lat1 * M_PI / 180.0;
    double phi2 = lat2 * M_PI / 180.0;
    double deltaPhi = (lat2 - lat1) * M_PI / 180.0;
    double deltaLambda = (lon2 - lon1) * M_PI / 180.0;

    double a = sin(deltaPhi / 2) * sin(deltaPhi / 2) +
               cos(phi1) * cos(phi2) *
               sin(deltaLambda / 2) * sin(deltaLambda / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return R * c; // Distance in meters
}

int validateChecksum(const char *sentence) {
    const char *asterisk = strchr(sentence, '*'); // Correct usage of strchr
    if (!asterisk) return 0; // No checksum found

    // Compute checksum
    unsigned char checksum = 0;
    const char *ptr; // Declare `ptr` properly
    for (ptr = sentence + 1; ptr < asterisk; ++ptr) {
        checksum ^= *ptr;
    }

    // Compare computed checksum with the received one
    unsigned int receivedChecksum;
    sscanf(asterisk + 1, "%x", &receivedChecksum);
    return checksum == receivedChecksum;
}

double convertToDecimalDegrees(const char *coordinate, char direction) {
    double degrees = atof(coordinate) / 100;
    double minutes = fmod(degrees, 1) * 100;
    degrees = (int)degrees + minutes / 60.0;

    return (direction == 'S' || direction == 'W') ? -degrees : degrees;
}

void UARTIntHandler(void) {
    uint32_t ui32Status = UARTIntStatus(UART1_BASE, true);
    UARTIntClear(UART1_BASE, ui32Status);

    while (UARTCharsAvail(UART1_BASE)) {
        char c = UARTCharGetNonBlocking(UART1_BASE);
        UARTCharPutNonBlocking(UART0_BASE, c);

        UARTBuffer[writeIdx++] = c;
        writeIdx %= BUFFER_SIZE;

        if (c == '\n') {
            extractGPGLLData();
        }
    }
}

void extractGPGLLData(void) {
    char sentence[128];
    uint16_t idx = 0;

    while (readIdx != writeIdx) {
        char c = UARTBuffer[readIdx++];
        readIdx %= BUFFER_SIZE;

        sentence[idx++] = c;

        if (c == '\n') {
            sentence[idx] = '\0';
            break;
        }
    }

    if (strncmp(sentence, "$GPGLL", 6) == 0) {
        strncpy(GPGLLData, sentence, sizeof(GPGLLData) - 1);
        GPGLLData[sizeof(GPGLLData) - 1] = '\0';
    }

    char gpsBuffer[BUFFER_SIZE] = {0};
    int bufferIndex = 0;
    char incomingChar;
    int i;
    for (i = 0; i < strlen(GPGLLData); ++i) { // Declare `i` correctly
        incomingChar = GPGLLData[i];

        if (incomingChar == '$') {
            if (bufferIndex > 0) {
                gpsBuffer[bufferIndex] = '\0';
                processGPSData(gpsBuffer);
            }
            bufferIndex = 0;
        }

        if (bufferIndex < BUFFER_SIZE - 1) {
            gpsBuffer[bufferIndex++] = incomingChar;
        }
    }

    if (bufferIndex > 0) {
        gpsBuffer[bufferIndex] = '\0';
        processGPSData(gpsBuffer);
    }
}

void processGPSData(const char *sentence) {
    if (strncmp(sentence, "$GPGLL", 6) == 0 && validateChecksum(sentence)) {
        char latitude[16], longitude[16];
        char latDir, longDir;
        char sentenceCopy[BUFFER_SIZE];
        strcpy(sentenceCopy, sentence);

        char *token = strtok(sentenceCopy, ",");
        token = strtok(NULL, ",");
        strcpy(latitude, token);
        token = strtok(NULL, ",");
        latDir = *token;
        token = strtok(NULL, ",");
        strcpy(longitude, token);
        token = strtok(NULL, ",");
        longDir = *token;

        double latDecimal = convertToDecimalDegrees(latitude, latDir);
        double longDecimal = convertToDecimalDegrees(longitude, longDir);

        // Print results (optional)
    }
}

void Timer0A_Handler(void) {
//    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
//
//    double distance = calculateDistance(lat1, lon1, lat2, lon2);
//    uint32_t noBytes = sprintf(acBuffer, "Distance: %.2f meters\n", distance);
//    uint32_t i;
//    for ( i = 0; i < noBytes; i++) {
//        UARTCharPut(UART0_BASE, acBuffer[i]);
//    }
}

int main(void) {
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600, UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 9600, UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

    UARTIntRegister(UART1_BASE, UARTIntHandler);
    IntEnable(INT_UART1);
    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);

    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / 1);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER0_BASE, TIMER_A);

    IntMasterEnable();

    while (1) {
    }
}
