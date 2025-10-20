// --Project Inclusions
//USB
#include "usb_cdc_lib.h" // Your USB library


//Macros, definitions .....
#include "main.h"      // Our header with 'bool' defined




//--- Configuration ---
#include "sysconfig.h" // Assuming this contains your USB configuration
#pragma config WDT = OFF        
#pragma config MCLRE = ON       
#pragma config DEBUG = OFF      
#pragma config CPUDIV = OSC1_PLL2     
#pragma config LVP = OFF        


// --- Global Variables ---
volatile static bool send_flap_command = false;  // Flag to signal main loop

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

        // This disables the INT0 Interrupt Enable bit (INTCON, 4)
        "bcf     INTCON, 4, c\n" 
        
        // Set our global flag 'send_flap_command' to 1 (which equals 'true')
        "movlw   1\n"
        "movwf   _send_flap_command, c\n"

        // CRITICAL: Clear the interrupt flag.
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
        if (send_flap_command == true)
        {
            // The assembly ISR told us the button was pressed.
            
            // A small delay for debounce
            __delay_ms(50); 
            
            // Send "4" over USB
            putUSBUSART("4", 1); 
            
            // Reset the flag
            send_flap_command = false;
            
            // Clear any latent interrupt flag (safety)
            asm("bcf INTCON, 1, c");
            
            // Re-enable the INT0 interrupt
            asm("bsf INTCON, 4, c");
        }

        // Keep the USB services running (this handles TX/RX buffers)
        CDCTxService();
    }
    return;
}