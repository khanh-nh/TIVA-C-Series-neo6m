#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"

// UART buffer for collecting data
#define BUFFER_SIZE 512
char gpsBuffer[BUFFER_SIZE];
uint32_t bufferIndex = 0;

// UART1 interrupt handler (GPS module)
void UARTIntHandler(void) {
    uint32_t ui32Status;
    uint32_t i;

    // Get interrupt status
    ui32Status = UARTIntStatus(UART1_BASE, true);

    // Clear the asserted interrupts
    UARTIntClear(UART1_BASE, ui32Status);

    // Loop while there are characters available in the UART FIFO
    while (UARTCharsAvail(UART1_BASE)) {
        char inp = UARTCharGetNonBlocking(UART1_BASE);

        // Append character to buffer if space allows
        if (bufferIndex < BUFFER_SIZE - 1) {
            gpsBuffer[bufferIndex++] = inp;

            // Check for the end of an NMEA sentence
            if (inp == '*') { // End of sentence detected
                // Add newline and terminate the string
                gpsBuffer[bufferIndex++] = '\n';
                gpsBuffer[bufferIndex] = '\0';

                // Echo the formatted sentence to UART0
                for (i = 0; i < bufferIndex; i++) {
                    UARTCharPutNonBlocking(UART0_BASE, gpsBuffer[i]);
                }

                // Reset the buffer index for the next sentence
                bufferIndex = 0;
            }
        } else {
            // Reset buffer if overflow occurs
            bufferIndex = 0;
        }
    }
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
    IntMasterEnable(); // Enable global interrupts

    while (1) {
        // Main loop can remain empty; all work is handled in the interrupt
    }
}
