#include "sysconfig.h"      // Your CONFIG bits and _XTAL_FREQ
#include <xc.h>
#include <string.h>

#include "main.h"           // Our main header
#include "usb_cdc_lib.h"    // USB driver
#include "display.h"        // Our 7-segment display module
#include "protocol.h"       // Our command parser/handler module
#include "eeprom.h"         // Our EEPROM storage module

// --- Local Buffers ---
// Buffer for receiving data *from* the PC
static unsigned char usbReadBuffer[32];

/**
 * @brief Configures Timer2 to create a periodic interrupt.
 * This interrupt will be used to run the 7-segment display multiplexing.
 */
static void Timers_Init(void)
{
    // Configure Timer2 for a fast, periodic interrupt
    // (This rate is good for display multiplexing)
    PR2   = 249;
    T2CON = 0x1D;            // T2OUTPS=0010 (1:3), TMR2ON=1, T2CKPS=11 (1:16)
    
    PIR1bits.TMR2IF = 0;     // Clear interrupt flag
    PIE1bits.TMR2IE = 1;     // Enable Timer2 interrupt
}

/**
 * @brief Configures all hardware modules.
 */
static void System_Init(void)
{
    // 1. Configure all pins as digital
    ADCON1 = 0x0F;
    CMCON  = 0x07;

    // 2. Initialize our modules
    Timers_Init();
    Display_Init();     // Set up 7-segment display pins
    initUSBLib();       // Initialize the USB CDC driver

    // 3. Set up the button pin as an input
    BUTTON_TRIS = 1;    // 1 = Input
}


void main(void) 
{
    uint16_t initial_best_score = 0;
    char button_was_pressed = 0;
    
    // 1. Initialize all hardware
    System_Init();
    
    // 2. Read the default best score from EEPROM (for slot 0)
    initial_best_score = EEPROM_ReadBestScore(0);
    
    // 3. Initialize the protocol logic
    // This will also send the "CS:READY" message to the PC
    Protocol_Init(initial_best_score);
    
    // 4. Enable all interrupts
    INTCONbits.PEIE  = 1;    // Enable peripheral interrupts
    INTCONbits.GIE   = 1;    // Enable global interrupts
    
    // 5. Main application loop
    while(1)
    {
        // Must be called to keep the USB stack running
        USBDeviceTasks();
        
        // --- 1. Check for data *from* the PC ---
        if(isUSBReady())
        {
            uint8_t n = getsUSBUSART(usbReadBuffer, sizeof(usbReadBuffer));
            if(n > 0)
            {
                // Send the raw data to our protocol "brain" to be parsed
                Protocol_ParseBuffer((const char*)usbReadBuffer, n);
            }
        }
        
        // --- 2. Check for button press *from* the PIC ---
        // (Uses a simple flag for debounce)
        if(BUTTON_PIN == 1 && button_was_pressed == 0)
        {
            __delay_ms(20); // Basic debounce delay
            if(BUTTON_PIN == 1) // Check again
            {
                button_was_pressed = 1;
                // Tell the protocol "brain" to send the "CS:BTN,1" message
                Protocol_SendButtonPress();
            }
        }
        else if(BUTTON_PIN == 0)
        {
            button_was_pressed = 0; // Reset the flag when button is released
        }

        // Must be called to send any queued USB data *to* the PC
        CDCTxService();
    }
    
    return;
}

/**
 * @brief Main Interrupt Service Routine (ISR)
 */
void __interrupt() mainISR (void)
{
    // --- Timer2 Interrupt: Used for 7-Segment Display ---
    if (PIR1bits.TMR2IF) 
    {
        PIR1bits.TMR2IF = 0; // Clear the interrupt flag
        
        // Call the display scanner function. This must be run
        // very quickly and very often.
        Display_Scan_ISR();
    }
    
    // --- USB Interrupt: Handled by the USB library ---
    // (This calls processUSBTasks() or similar inside the driver)
    processUSBTasks();
}