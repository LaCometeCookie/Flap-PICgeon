#include "sysconfig.h"
#include <xc.h>
#include "main.h"
#include "usb_cdc_lib.h"

#define _XTAL_FREQ 48000000 // System clock frequency for delays

// --- Pin Definitions ---
#define BUZZER_PIN      LATCbits.LATC2
#define BUZZER_TRIS     TRISCbits.TRISC2

#define BUTTON_PIN      PORTAbits.RA1 // Using PORTA to read
#define BUTTON_TRIS     TRISAbits.TRISA1

void main(void) 
{
    // A variable to prevent sending "4" continuously while the button is held
    char button_was_pressed = 0;

    initUSBLib();

    // --- Initializing routines ---
    BUZZER_TRIS = 0;    // Set RC2 as an output for the buzzer
    BUZZER_PIN = 0;     // Buzzer is off initially

    // VITAL: Configure PORTA pins as DIGITAL inputs
    ADCON1 = 0x0F;
    CMCON = 0x07;
    
    BUTTON_TRIS = 1;    // Set RA0 as an input for the button
    
    while(1)
    {
        // === PART 1: Check for commands from the computer ===
        if(isUSBReady())
        {
            uint8_t numBytesRead;
            numBytesRead = getsUSBUSART(usbReadBuffer, sizeof(usbReadBuffer));

            if(numBytesRead > 0)
            {
                char command = usbReadBuffer[0];
                switch(command)
                {
                    case '2': BUZZER_PIN = 1; break;
                    case '3': BUZZER_PIN = 0; break;
                }
            }
        }
        
        // === PART 2: Check if the button on RA0 is pressed (PULL-DOWN LOGIC) ===
        
        // Is the button pressed (pin is HIGH) AND it was not pressed before?
        if(BUTTON_PIN == 1 && button_was_pressed == 0) // <-- MODIFIED to check for 1
        {
            __delay_ms(20); // Debounce delay
            if(BUTTON_PIN == 1) // Check again after delay
            {
                putUSBUSART("4", 2);      // Send "4" followed by a newline
                button_was_pressed = 1;     // Mark the button as pressed
            }
        }
        // If the button is released (pin is LOW), reset the flag
        else if(BUTTON_PIN == 0) // <-- MODIFIED to check for 0
        {
            button_was_pressed = 0;
        }

        // Keep the USB services running
        CDCTxService();
    }
    
    return;
}

void __interrupt() mainISR (void)
{
    processUSBTasks();
}