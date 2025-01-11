#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/systick.h"
#include "driverlib/i2c.h"
#include "lcd1602_i2c.h"
#include "libs/i2c/i2c.h"
// Constants
#define BUFFER_SIZE 256       // Circular buffer size
#define GPGLL_LINE_SIZE 128   // Max length of a $GPGLL sentence
#define BUZZER_PIN GPIO_PIN_3
// Global Variables
double current_latDecimal, current_longDecimal;
double inital_latDecimal, inital_longDecimal;
volatile bool gps_signal_ready = false, current_gps_ready = false;
char latitude[16], longitude[16];
char latDir, longDir;
char UARTBuffer[BUFFER_SIZE];
char GPGLLSentence[GPGLL_LINE_SIZE];
uint16_t writeIdx = 0, readIdx = 0;  // Circular buffer indices
volatile bool sentenceReady = false;
double calculated_distance;

// Function Prototypes
void extractGPGLL(void);
void processGPGLL(const char *gpgll);
int validateChecksum(const char *sentence);
double convertToDecimalDegrees(const char *coordinate, char direction);
void calculateDistance(double lat1, double lon1, double lat2, double lon2);

// GPIO Interrupt Handler
void GPIOPortFIntHandler(void) {
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4 | GPIO_INT_PIN_0);
    if (!(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4))) {
        inital_latDecimal = current_latDecimal;
        inital_longDecimal = current_longDecimal;
        current_gps_ready = true;
    }
}

// Function to calculate distance between two GPS coordinates
void calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371e3; // Earth's radius in meters
    double phi1 = lat1 * M_PI / 180.0;
    double phi2 = lat2 * M_PI / 180.0;
    double deltaPhi = (lat2 - lat1) * M_PI / 180.0;
    double deltaLambda = (lon2 - lon1) * M_PI / 180.0;

    double a = sin(deltaPhi / 2) * sin(deltaPhi / 2) +
               cos(phi1) * cos(phi2) *
               sin(deltaLambda / 2) * sin(deltaLambda / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    calculated_distance = R * c;
}
void SysTickIntHandler(void)
{
   LCD1602Clear();
   int int_distance = calculated_distance; // Assuming calculated_distance is a valid double
   char distance_str[16];                  // Buffer to hold the string representation

   // Convert integer to string
   sprintf(distance_str, "%d", int_distance);

   // Pass the string to LCD1602SetString
   LCD1602SetString(distance_str);
}
// UART Interrupt Handler
void UARTIntHandler(void) {
    uint32_t ui32Status = UARTIntStatus(UART1_BASE, true);
    UARTIntClear(UART1_BASE, ui32Status);

    static bool capturing = false;
    static uint16_t sentenceIdx = 0;

    while (UARTCharsAvail(UART1_BASE)) {
        char c = UARTCharGetNonBlocking(UART1_BASE);

        if (c == '$') {
            capturing = true;
            sentenceIdx = 0;
        }

        if (capturing) {
            if (sentenceIdx < GPGLL_LINE_SIZE - 1) {
                GPGLLSentence[sentenceIdx++] = c;
            }

            if (c == '\n' || sentenceIdx >= GPGLL_LINE_SIZE - 1) {
                GPGLLSentence[sentenceIdx] = '\0';
                capturing = false;
                sentenceReady = true;
            }
        }
    }
}

// Function to extract and process $GPGLL sentences
void extractGPGLL(void) {
    if (sentenceReady) {
        sentenceReady = false;
        if (strncmp(GPGLLSentence, "$GPGLL", 6) == 0 && validateChecksum(GPGLLSentence)) {
            processGPGLL(GPGLLSentence);
            gps_signal_ready = true;
        } else {
            gps_signal_ready = false;
        }
    }
}

// Function to process the $GPGLL sentence
void processGPGLL(const char *gpgll) {
    char *token;
    char sentenceCopy[GPGLL_LINE_SIZE];

    strncpy(sentenceCopy, gpgll, GPGLL_LINE_SIZE - 1);
    sentenceCopy[GPGLL_LINE_SIZE - 1] = '\0';

    token = strtok(sentenceCopy, ",");  // Skip "$GPGLL"
    token = strtok(NULL, ",");          // Latitude
    if (token) strcpy(latitude, token);
    token = strtok(NULL, ",");          // Latitude direction
    if (token) latDir = *token;
    token = strtok(NULL, ",");          // Longitude
    if (token) strcpy(longitude, token);
    token = strtok(NULL, ",");          // Longitude direction
    if (token) longDir = *token;

    current_latDecimal = convertToDecimalDegrees(latitude, latDir);
    current_longDecimal = convertToDecimalDegrees(longitude, longDir);
}

// Function to validate checksum of NMEA sentence
int validateChecksum(const char *sentence) {
    const char *asterisk = strchr(sentence, '*');
    if (!asterisk) return 0;

    unsigned char checksum = 0;
    const char *ptr = sentence + 1;
    for (; ptr < asterisk; ++ptr)  // Compute XOR for the checksum
    {
        checksum ^= *ptr;
    }



    unsigned int receivedChecksum;
    sscanf(asterisk + 1, "%x", &receivedChecksum);
    return checksum == receivedChecksum;
}
void PortDInit(void) {
    // Enable the clock for port D
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3;
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R3) == 0) {
    }

    // Configure PD0 and PD1 for I2C3
    GPIO_PORTD_AFSEL_R |= 0x03;  // Enable alternate function on PD0, PD1
    GPIO_PORTD_ODR_R |= 0x02;    // Enable open drain on PD1
    GPIO_PORTD_DEN_R |= 0x03;    // Enable digital on PD0, PD1
    GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & 0xFFFFFF00) | (0x03 | (0x03 << 4)); // I2C3 SCL/SDA
}

void I2C3Init(void) {
    // Enable the clock for I2C3
    SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R3;
    while ((SYSCTL_PRI2C_R & SYSCTL_PRI2C_R3) == 0) {
    }

    // Initialize I2C3 as a master
    I2C3_MCR_R = I2C_MCR_MFE;     // Enable master function
    I2C3_MTPR_R = 39;             // Configure for standard speed (100 kbps)
}

// Function to convert coordinates to decimal degrees
double convertToDecimalDegrees(const char *coordinate, char direction) {
    double degrees = atof(coordinate) / 100;
    double minutes = fmod(degrees, 1) * 100;
    degrees = (int)degrees + minutes / 60.0;

    if (direction == 'S' || direction == 'W') {
        degrees = -degrees;
    }

    return degrees;
}
void BuzzerOn(void) {
    GPIOPinWrite(GPIO_PORTF_BASE, BUZZER_PIN, BUZZER_PIN);
}

// Function to turn the buzzer off
void BuzzerOff(void) {
    GPIOPinWrite(GPIO_PORTF_BASE, BUZZER_PIN, 0);
}
int main(void) {
    // Configure system clock to 40 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_20 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    // Enable peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Configure GPIO for UART0
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure GPIO for UART1
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure GPIO for interrupts
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, BUZZER_PIN);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_RISING_EDGE);
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_4 | GPIO_INT_PIN_0);
    IntEnable(INT_GPIOF);
    GPIOIntRegister(GPIO_PORTF_BASE, GPIOPortFIntHandler);

    // Configure UART0
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Configure UART1
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Enable UART1 interrupt
    UARTIntRegister(UART1_BASE, UARTIntHandler);
    IntEnable(INT_UART1);
    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);
    SysTickPeriodSet(SysCtlClockGet()/10-1);//Change value every second
    SysTickIntEnable();
    // Enable global interrupts
    IntMasterEnable();
    PortDInit();
    SysTickEnable();

       // Initialize I2C3
       I2C3Init();

       // Initialize the LCD
       LCD1602Init();

       // Clear the display
       LCD1602Clear();

       // Set the cursor to the first row, first column
       LCD1602Goto(0, 0);
       LCD1602SetString("lol");
       while (1) {
               extractGPGLL();
               if (gps_signal_ready && current_gps_ready) {
                   calculateDistance(inital_latDecimal, inital_longDecimal, current_latDecimal, current_longDecimal);
                   if (calculated_distance > 10) {
                                   BuzzerOn(); // Turn on the buzzer
                               } else {
                                   BuzzerOff(); // Turn off the buzzer
                               }

                           }
               }}
