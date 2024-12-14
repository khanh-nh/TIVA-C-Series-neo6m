#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>  // For printf
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"

// Constants and buffers
#define BUFFER_SIZE 256
char UARTBuffer[BUFFER_SIZE];  // Circular buffer for UART data
char GPGLLData[128];           // String to store extracted $GPGLL sentence
uint16_t writeIdx = 0;         // Circular buffer write index
uint16_t readIdx = 0;          // Circular buffer read index

// Function prototypes
void extractGPGLLData(void);
void printGPGLLSentence(void);

// UART1 interrupt handler (GPS module)
void UARTIntHandler(void) {
    uint32_t ui32Status;

    // Get interrupt status
    ui32Status = UARTIntStatus(UART1_BASE, true);
    // Clear the asserted interrupts
    UARTIntClear(UART1_BASE, ui32Status);

    // Read characters from UART FIFO into circular buffer
    while (UARTCharsAvail(UART1_BASE)) {
        UARTCharPutNonBlocking(UART0_BASE, UARTCharGetNonBlocking(UART1_BASE));
        char c = UARTCharGetNonBlocking(UART1_BASE);

        // Store the character in the circular buffer
        UARTBuffer[writeIdx++] = c;
        writeIdx %= BUFFER_SIZE; // Wrap around if the index exceeds buffer size

        // Extract the $GPGLL data if available
        if (c == '\n') {  // Check for end of an NMEA sentence
            extractGPGLLData();
        }
    }
}

// Function to extract $GPGLL sentence
void extractGPGLLData(void) {
    char sentence[128];
    uint16_t idx = 0;

    // Extract characters from the circular buffer
    while (readIdx != writeIdx) {
        char c = UARTBuffer[readIdx++];
        readIdx %= BUFFER_SIZE;

        sentence[idx++] = c;

        // Stop at the end of the sentence
        if (c == '\n') {
            sentence[idx] = '\0'; // Null-terminate the string
            break;
        }
    }

    // Check if the sentence starts with "$GPGLL"
    if (strncmp(sentence, "$GPGLL", 6) == 0) {
        strncpy(GPGLLData, sentence, sizeof(GPGLLData) - 1); // Save the $GPGLL sentence
        GPGLLData[sizeof(GPGLLData) - 1] = '\0'; // Ensure null-termination

        // Print the extracted $GPGLL sentence
        //printGPGLLSentence();
    }
}

// Function to print the $GPGLL sentence to the console
void printGPGLLSentence(void) {
    // Use printf to print the sentence to the console
    //printf("Extracted Sentence: %s\n", GPGLLData);
}

int main(void) {
    // Step 1: Set system clock to 40 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    // Step 2: Enable peripherals for UART0 (PC) and UART1 (GPS module)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // GPIO for UART0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // GPIO for UART1

    // Step 3: Configure GPIO pins for UART0 (USB-to-PC communication)
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Step 4: Configure GPIO pins for UART1 (GPS communication)
    GPIOPinConfigure(GPIO_PB0_U1RX);  // GPS TX to TIVA RX
    GPIOPinConfigure(GPIO_PB1_U1TX);  // GPS RX (optional, not typically used)
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Step 5: Configure UART0 (serial monitor)
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Step 6: Configure UART1 (GPS module)
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Step 7: Enable UART1 interrupt
    UARTIntRegister(UART1_BASE, UARTIntHandler);
    IntEnable(INT_UART1);
    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);
    IntMasterEnable();

    // Step 8: Main loop
    while (1) {
        // Main loop can be used for other tasks, data is handled in the interrupt
    }
}
