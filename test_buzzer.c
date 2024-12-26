#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"

// Constants
#define BUZZER_PIN GPIO_PIN_3  // Buzzer connected to PF3
#define BUTTON_PIN GPIO_PIN_4  // User button connected to PF4

// Interrupt handler for Port F
void GPIOPortFIntHandler(void) {
    // Get and clear the interrupt status
    uint32_t status = GPIOIntStatus(GPIO_PORTF_BASE, true);
    GPIOIntClear(GPIO_PORTF_BASE, status);

    // Check if the interrupt was triggered by the user button (PF4)
    if (status & GPIO_INT_PIN_4) {
        // Toggle the buzzer state
        if (GPIOPinRead(GPIO_PORTF_BASE, BUZZER_PIN)) {
            GPIOPinWrite(GPIO_PORTF_BASE, BUZZER_PIN, 0); // Turn off the buzzer
        } else {
            GPIOPinWrite(GPIO_PORTF_BASE, BUZZER_PIN, BUZZER_PIN); // Turn on the buzzer
        }
    }
}

int main(void) {
    // Configure system clock to 40 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    // Enable GPIO Port F
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Unlock PF0 and configure pins (required on Tiva C Series for PF0 and PF4)
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= BUTTON_PIN;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;

    // Configure PF3 (buzzer) as output
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, BUZZER_PIN);
    GPIOPinWrite(GPIO_PORTF_BASE, BUZZER_PIN, 0); // Start with buzzer off

    // Configure PF4 (user button) as input with a pull-up resistor
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, BUTTON_PIN);
    GPIOPadConfigSet(GPIO_PORTF_BASE, BUTTON_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Register the interrupt handler for Port F
    GPIOIntRegister(GPIO_PORTF_BASE, GPIOPortFIntHandler);

    // Configure PF4 to trigger an interrupt on a falling edge (button press)
    GPIOIntTypeSet(GPIO_PORTF_BASE, BUTTON_PIN, GPIO_FALLING_EDGE);

    // Enable interrupts for PF4 and globally
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    IntMasterEnable();

    // Infinite loop
    while (1) {
        // Main loop does nothing; buzzer control is handled by the interrupt handler
    }
}
