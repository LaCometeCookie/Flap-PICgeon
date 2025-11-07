#include "sysconfig.h"      // Your CONFIG bits and _XTAL_FREQ
#include <xc.h>
#include <string.h>

#include "main.h"           // Our main header
#include "usb_cdc_lib.h"    // USB driver
#include "display.h"        // Our 7-segment display module
#include "protocol.h"       // Our command parser/handler module
#include "eeprom.h"         // Our EEPROM storage module
#include "glcd.h"
#include "inputs.h"

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
    
    // === NEW: Initialize the GLCD ===
    glcd_Init(GLCD_ON);
    glcd_FillScreen(0); // Clear screen once at startup

    // 3. Set up the button pin as an input
    BUTTON_TRIS = 1;    // 1 = Input (RE0)
    
    // 4. Enable the USB Interrupt Source
    PIE2bits.USBIE = 1;
}


void main(void) 
{
    uint16_t initial_best_score = 0;
    
    System_Init();
    initial_best_score = EEPROM_ReadBestScore(0);
    Protocol_Init(initial_best_score);
    
    INTCONbits.PEIE  = 1;
    INTCONbits.GIE   = 1;
    
    Inputs_Init();
    
    
    while(1)
    {
        USBDeviceTasks();
        
        if(isUSBReady())
        {
            uint8_t n = getsUSBUSART(usbReadBuffer, sizeof(usbReadBuffer));
            if(n > 0)
            {
                Protocol_ParseBuffer((const char*)usbReadBuffer, n);
            }
        }
        
        // === NEW: Cleaned-up flap logic ===
        if (DidPlayerFlap())
        {
            // Send the generic "flap" command, no matter which sensor
            Protocol_SendButtonPress(); 
        }

        CDCTxService();
    }
    
    return;
}

/**
 * @brief Main Interrupt Service Routine (ISR)
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
    if (PIE2bits.USBIE && PIR2bits.USBIF)
    {
        processUSBTasks();
    }
}