/*
 * This is the final project
 * Implementing GPS module NEO 6m
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"

// UART interrupt handler
void UARTIntHandler(void)
{
    uint32_t ui32Status;


    // Get interrupt status
    ui32Status = UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts
    UARTIntClear(UART0_BASE, ui32Status);

    // Loop while there are characters available in the UART FIFO
    while (UARTCharsAvail(UART0_BASE))
    {
        // Echo the received character
        UARTCharPutNonBlocking(UART0_BASE, UARTCharGetNonBlocking(UART0_BASE));
    }
}



