#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
//Pin map on Tiva C board
  // Rs   -> A5
  // Rw   -> A6
  // E    -> A7
  //D0 - D7 (Data) -> B0-B7
// Pin configurations for LCD
#define LCD_RS GPIO_PIN_5  // Register Select
#define LCD_RW GPIO_PIN_6  // Read/Write
#define LCD_E  GPIO_PIN_7  // Enable
#define LCD_CTRL_PORT GPIO_PORTA_BASE
#define LCD_DATA_PORT GPIO_PORTB_BASE

// Function Prototypes
void LCD_Init(void);
void LCD_Command(uint8_t cmd);
void LCD_Data(uint8_t data);
void LCD_Write(char *str);
void LCD_SetCursor(uint8_t line, uint8_t block);
void LCD_Clear(void);
void LCD_DelayMs(uint32_t milliseconds);

// Main function to display 'M' on the LCD
int main(void) {
    // Set system clock to 40 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    // Initialize the LCD
    LCD_Init();

    // Clear the LCD and set cursor to the first position
    LCD_Clear();
    LCD_SetCursor(0, 0);

    // Write 'M' on the LCD
    LCD_Data('M');

    while (1) {
        // Infinite loop
    }
}

// Function Definitions

// Initialize the LCD
void LCD_Init(void) {
    // Enable peripherals for LCD control and data ports
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // Control pins
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // Data pins

    // Configure control pins as outputs
    GPIOPinTypeGPIOOutput(LCD_CTRL_PORT, LCD_RS | LCD_RW | LCD_E);

    // Configure data pins as outputs
    GPIOPinTypeGPIOOutput(LCD_DATA_PORT, 0xFF); // All pins (B0-B7)

    // Initialize LCD in 8-bit mode
    LCD_Command(0x38); // 8-bit, 2-line, 5x7 dots
    LCD_Command(0x0C); // Display ON, Cursor OFF
    LCD_Command(0x06); // Entry mode: Auto increment cursor
    LCD_Command(0x01); // Clear display
    LCD_DelayMs(2);
}

// Send a command to the LCD
void LCD_Command(uint8_t cmd) {
    GPIOPinWrite(LCD_CTRL_PORT, LCD_RS | LCD_RW | LCD_E, 0x00); // RS=0, RW=0
    GPIOPinWrite(LCD_DATA_PORT, 0xFF, cmd);                     // Send command
    GPIOPinWrite(LCD_CTRL_PORT, LCD_E, LCD_E);                  // Enable high
    LCD_DelayMs(1);                                             // Short delay
    GPIOPinWrite(LCD_CTRL_PORT, LCD_E, 0x00);                   // Enable low
    LCD_DelayMs(2);                                             // Wait for the command to execute
}

// Send data to the LCD
void LCD_Data(uint8_t data) {
    GPIOPinWrite(LCD_CTRL_PORT, LCD_RS | LCD_RW | LCD_E, LCD_RS); // RS=1, RW=0
    GPIOPinWrite(LCD_DATA_PORT, 0xFF, data);                      // Send data
    GPIOPinWrite(LCD_CTRL_PORT, LCD_E, LCD_E);                    // Enable high
    LCD_DelayMs(1);                                               // Short delay
    GPIOPinWrite(LCD_CTRL_PORT, LCD_E, 0x00);                     // Enable low
    LCD_DelayMs(2);                                               // Wait for the data to be written
}

// Write a string to the LCD
void LCD_Write(char *str) {
    while (*str) {
        LCD_Data(*str++);
    }
}

// Set the cursor position
void LCD_SetCursor(uint8_t line, uint8_t block) {
    uint8_t address = (line == 0) ? 0x80 : 0xC0; // Line 0: 0x80, Line 1: 0xC0
    address += block;
    LCD_Command(address);
}

// Clear the LCD
void LCD_Clear(void) {
    LCD_Command(0x01); // Clear display command
    LCD_DelayMs(2);    // Wait for the clear command to execute
}

// Delay in milliseconds using SysTick
void LCD_DelayMs(uint32_t milliseconds) {
    SysCtlDelay((SysCtlClockGet() / 3000) * milliseconds);
}
