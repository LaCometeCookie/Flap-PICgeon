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


//Variable statiques comme ça tous le monde y a accès 
static char button_was_pressed = 0;
static char process_button_press = 0;



void main(void) 
{
    initUSBLib();


    //Entrée numérique sur le port A
    ADCON1 = 0x0F;
    CMCON = 0x07;
    
    BUTTON_TRIS = 1;    // Set RA1 comme entrée
    
    // Enable interrupts so USB enumeration works
    INTCONbits.GIEH = 1;
    
    while(1)
    {
        process_button_press = 0; // Reset C flag

        // ===================================================================
        // === PART 1: Lecture des infos envoyées par le pc                ===
        // ===================================================================
        if(isUSBReady())
        {
            uint8_t numBytesRead;
            numBytesRead = getsUSBUSART(usbReadBuffer, sizeof(usbReadBuffer));

            if(numBytesRead > 0)
            {
                char command = usbReadBuffer[0];
                //Cas à changer plus tard pour savoir que faire en fonction de la commande
                switch(command)
                {
                    case '2': BUZZER_PIN = 1; break;//A rajouter plus tard
                    case '3': BUZZER_PIN = 0; break;//A rajouter plus tard
                }
            }
        }
        
        // ===================================================================
        // === PART 2: Envoi des données (à partir du bouton ici)          ===
        // ===================================================================

        //Partie assembleur
        asm(
            // Check if the button is currently held down
            "BTFSS   PORTA, 1 \n"            // Bit Test File (PORTA, bit 1), Skip if Set (if button is 1)
            "GOTO    _button_is_released \n" // If button is 0, jump to release logic

            // --- If we are here, the button pin is HIGH (1) ---

            // Now, check if it was already pressed before
            "MOVF    _button_was_pressed, W \n" // Move flag to W, sets Zero flag if W=0
            "BNZ     _end_asm_check \n"      // Branch if Not Zero (if flag was 1, we're done)
            
            // --- If we are here, it's a NEW button press ---
            "MOVLW   1 \n"                   // Move Literal Value 1 into W
            "MOVWF   _process_button_press \n"// Move W into our 'process_button_press' flag
            "GOTO    _end_asm_check \n"      // We're done, jump to the end

        "_button_is_released: \n" // Note the label and colon
            // --- If we are here, the button pin is LOW (0) ---
            "CLRF    _button_was_pressed \n" // Clear the main flag

        "_end_asm_check: \n" // Note the label and colon
            // All assembly paths end here.
        ); // --- End of the assembly block ---
        
        
        // C code runs only if the assembly conditions were met
        if(process_button_press == 1){
            __delay_ms(20); // Debounce
            if (BUTTON_PIN == 1) // Re-check pin after debounce
            {
                putUSBUSART("4", 1); // Send "4" AND a newline
                button_was_pressed = 1; // Set the flag
            }
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