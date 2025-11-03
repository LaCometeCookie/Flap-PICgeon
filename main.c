#include "sysconfig.h"      // Your CONFIG bits and _XTAL_FREQ
#include <xc.h>
#include <string.h>

#include "main.h"           // Our main header
#include "usb_cdc_lib.h"    // USB driver
#include "display.h"        // Our 7-segment display module
#include "protocol.h"       // Our command parser/handler module
#include "eeprom.h"         // Our EEPROM storage module

// --- Local Buffers ---
static unsigned char usbReadBuffer[32];

/**
 * @brief Configures Timer2 to create a periodic interrupt.
 */
static void Timers_Init(void)
{
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
    
    // === NEW: Enable the USB Interrupt Source ===
    // We are still in the non-priority model (IPEN=0)
    // but we are enabling the USB interrupt as a source.
    PIE2bits.USBIE = 1;
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
    Protocol_Init(initial_best_score);
    
    // 4. Enable all interrupts
    INTCONbits.PEIE  = 1;    // Enable peripheral interrupts
    INTCONbits.GIE   = 1;    // Enable global interrupts
    
    // 5. Main application loop
    while(1)
    {
        // === Required Main-Loop Polling for Hybrid Stack ===
        // This handles the high-level device state (enumeration)
        USBDeviceTasks();
        
        // --- 1. Check for data *from* the PC ---
        if(isUSBReady())
        {
            uint8_t n = getsUSBUSART(usbReadBuffer, sizeof(usbReadBuffer));
            if(n > 0)
            {
                Protocol_ParseBuffer((const char*)usbReadBuffer, n);
            }
        }
        
        // --- 2. Check for button press *from* the PIC ---
        if(BUTTON_PIN == 1 && button_was_pressed == 0)
        {
            __delay_ms(20); // Basic debounce delay
            if(BUTTON_PIN == 1) // Check again
            {
                button_was_pressed = 1;
                Protocol_SendButtonPress();
            }
        }
        else if(BUTTON_PIN == 0)
        {
            button_was_pressed = 0; // Reset the flag
        }

        // === Required Main-Loop Polling for Hybrid Stack ===
        // This "pumps" the send buffer
        CDCTxService();
    }
    
    return;
}

/**
 * @brief Main Interrupt Service Routine (ISR)
 * (RCONbits.IPEN is 0, so this is the *only* ISR)
 */
void __interrupt() mainISR (void)
{
    // --- 1. Timer2 Interrupt: Used for 7-Segment Display ---
    if (PIE1bits.TMR2IE && PIR1bits.TMR2IF) 
    {
        PIR1bits.TMR2IF = 0; // Clear the interrupt flag
        Display_Scan_ISR();
    }
    
    // --- 2. USB Interrupt: Handled by the USB library ---
    // This is the "dedicated USB interrupt" check.
    // It is gated by its own flag, not Timer2's.
    if (PIE2bits.USBIE && PIR2bits.USBIF)
    {
        processUSBTasks();
        // The USBIF flag is cleared inside processUSBTasks()
    }
}