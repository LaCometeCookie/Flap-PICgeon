#include "sysconfig.h"      // Your CONFIG bits and _XTAL_FREQ
#include <xc.h>
#include <string.h>

#include "main.h"           // Our main header
#include "usb_cdc_lib.h"    // USB driver
#include "display.h"        // Our 7-segment display module
#include "protocol.h"       // Our command parser/handler module
#include "eeprom.h"         // Our EEPROM storage module
#include "glcd.h"

// --- Local Buffers ---
static unsigned char usbReadBuffer[32];

// --- File-Scope Flags for ASM ---
// These are declared here (static global) so the ASM block can see them.
static char button_was_pressed = 0;
static char process_button_flag = 0;


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
    // button_was_pressed is now a static global
    
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
        
        // --- 2. Check for button press *from* the PIC (using ASM) ---
        
        // Reset the C flag at the start of each loop
        process_button_flag = 0; 

        asm(
            // Check if the button is currently held down
            "BTFSS   PORTE, 0 \n"            // Bit Test (PORTE, bit 0), Skip if Set (if button is 1)
            "GOTO    _button_is_released \n" // If button is 0, jump to release logic

            // --- If we are here, the button pin (RE0) is HIGH (1) ---

            // Now, check if it was already pressed before
            "MOVF    _button_was_pressed, W \n" // Move flag to W, sets Zero flag if W=0
            "BNZ     _end_asm_check \n"      // Branch if Not Zero (if flag was 1, we're done)

            // --- If we are here, it's a NEW button press ---
            "MOVLW   1 \n"                   // Move Literal Value 1 into W
            "MOVWF   _process_button_flag \n" // Move W into our 'process_button_flag'
            "GOTO    _end_asm_check \n"      // We're done, jump to the end

        "_button_is_released: \n"
            // --- If we are here, the button pin (RE0) is LOW (0) ---
            "CLRF    _button_was_pressed \n" // Clear the main flag

        "_end_asm_check: \n"
            // All assembly paths end here.
        ); // --- End of the assembly block ---

        
        // C code checks the flag set by the assembly
        if(process_button_flag == 1)
        {
            __delay_ms(20); // Debounce
            if (BUTTON_PIN == 1) // Re-check pin after debounce
            {
                // Set the C flag that the ASM block reads
                button_was_pressed = 1; 
                
                // Send the new protocol message
                Protocol_SendButtonPress();
            }
        }

        // === Required Main-Loop Polling for Hybrid Stack ===
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