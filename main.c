// --- Standard and Project Includes ---
#include <xc.h>
#include "sysconfig.h" // Assuming this contains your USB configuration
#include "main.h"      // Assuming this is part of your project structure
#include "usb_cdc_lib.h" // Your USB library

#define _XTAL_FREQ 48000000 // System clock frequency for delays

// --- Configuration Bits ---
        
#pragma config WDT = OFF        
#pragma config MCLRE = ON       
#pragma config DEBUG = OFF      
#pragma config CPUDIV = OSC1_PLL2     
#pragma config LVP = OFF        

// --- Global Variables ---
// 'volatile' is CRITICAL as this is modified in an interrupt.
volatile static char send_flap_command = 0;  // Flag to signal main loop

// --- Pin Definitions ---
#define BUTTON_PIN      PORTBbits.RB0
#define BUTTON_TRIS     TRISBbits.TRISB0

/**
 * @brief Main Interrupt Service Routine (ISR)
 * This function handles both USB tasks and the external button press using assembly.
 */
void __interrupt() mainISR (void)
{
    // It's vital to service the USB tasks first.
    processUSBTasks();

    // ===================================================================
    // === Check for our button press using INLINE ASSEMBLY            ===
    // ===================================================================
    asm(
        // Check if the INT0 Interrupt Flag is set (INTCON, bit 1).
        // If the flag is 0 (not our interrupt), skip the logic and jump to the end.
        "btfss   INTCON, 1, c\n"
        "goto    _END_OF_ISR_CHECK\n"

        // --- If we are here, the button interrupt occurred ---

        // Set our global C flag 'send_flap_command' to 1.
        // We access C variables from assembly by prefixing them with an underscore.
        "bcf INTCON, 4\n"//Désactive les interruptions
        "movlw   1\n"
        "movwf   _send_flap_command, c\n"

        // CRITICAL: Clear the interrupt flag to re-arm the interrupt for the next press.
        "bcf     INTCON, 1, c\n"

        "_END_OF_ISR_CHECK:\n"
        // This label is the target for our GOTO. The ISR will exit from here.
    );
}

void main(void) 
{
    initUSBLib();

    // --- Hardware Initialization ---
    // Configure PORTA/B as digital I/O (important for PIC18)
    ADCON1 = 0x0F;
    CMCON = 0x07;
    
    // Set pin directions
    BUTTON_TRIS = 1;      // Set RB0 as an input for the button

    // --- Interrupt Configuration ---
    INTCON2bits.INTEDG0 = 1; // Trigger on RISING edge (button press)
    INTCONbits.INT0IE = 1;   // Enable the INT0 external interrupt
    RCONbits.IPEN = 0;       // Disable interrupt priority (for simplicity)
    INTCONbits.GIE = 1;      // Enable Global Interrupts

    while(1)
    {
        // === PART 1: Check for commands from the computer ===
        if(isUSBReady())
        {
            uint8_t numBytesRead;
            numBytesRead = getsUSBUSART(usbReadBuffer, sizeof(usbReadBuffer));
            if(numBytesRead > 0)
            {
                // You can add new command handling here if needed.
            }
        }
        
        // === PART 2: Check the flag set by our ISR ===
        if (send_flap_command == 1)
        {
            // The assembly ISR told us the button was pressed.
            
            // A small delay helps prevent sending multiple messages if the
            // button is held down or bounces slightly.
            __delay_ms(100); 
            
            // Since the hardware interrupt already confirmed a rising edge,
            // we can trust that a press occurred and send the data directly.
            putUSBUSART("4", 1); // Send "4" over USB
            
            // Reset the flag so we don't send again until the next press.
            send_flap_command = 0;
            
             asm("bsf INTCON, 4\n");//Réactive l'interruption
        }

        // Keep the USB services running (this handles TX/RX buffers)
        CDCTxService();
    }
    return;
}