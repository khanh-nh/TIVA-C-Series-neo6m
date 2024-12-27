#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "lcd1602_i2c.h"
#include "libs/i2c/i2c.h"



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

int main(void) {
    // Set system clock to 40 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    // Initialize Port D for I2C3
    PortDInit();

    // Initialize I2C3
    I2C3Init();

    // Initialize the LCD
    LCD1602Init();

    // Clear the display
    LCD1602Clear();

    // Set the cursor to the first row, first column
    LCD1602Goto(0, 0);

    // Display "Hello" on the LCD
    LCD1602SetString("Hello123456789");

    while (1) {
        // Main loop to keep the program running
    }
}
