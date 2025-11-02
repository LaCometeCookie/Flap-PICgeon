#include <xc.h>         // For LATA, LATD, TRIS, etc.
#include "display.h"    // Include our own header

// --- Private Module Variables ---

// Buffer holding the 7-segment patterns for the 4 digits
// 'volatile' is critical: it's modified by the main loop (via SetScore)
// and read by the ISR (Scan_ISR).
static volatile uint8_t segBuf[4] = {0, 0, 0, 0};

// 7-segment pattern map for digits 0-9 (Common Anode)
// If your display is Common Cathode, you will need to invert these (e.g., ~0b00111111)
static const uint8_t segMap[10] = {
    0b00111111, //0
    0b00000110, //1
    0b01011011, //2
    0b01001111, //3
    0b01100110, //4
    0b01101101, //5
    0b01111101, //6
    0b00000111, //7
    0b01111111, //8
    0b01101111  //9
};

// --- Private Helper Functions ---

/**
 * @brief Internal function to calculate segment patterns and update the buffer.
 */
static void s_UpdateSegBuf(uint16_t score, uint8_t blankLeadingZeros)
{
    if(score > 9999) score = 9999;
    
    // Break score into individual digits
    uint8_t d3 = (score / 1000) % 10; // Thousands
    uint8_t d2 = (score / 100)  % 10; // Hundreds
    uint8_t d1 = (score / 10)   % 10; // Tens
    uint8_t d0 = score % 10;           // Ones

    // Map digits to segment patterns
    uint8_t s3 = segMap[d3];
    uint8_t s2 = segMap[d2];
    uint8_t s1 = segMap[d1];
    uint8_t s0 = segMap[d0];

    // Handle blanking of leading zeros
    if(blankLeadingZeros) {
        if(score < 1000) {
            s3 = 0; // Blank
            if(score < 100) {
                s2 = 0; // Blank
                if(score < 10) {
                    s1 = 0; // Blank
                    // We always show the last digit, even if 0
                }
            }
        }
    }

    // Update the volatile buffer
    // This is the "critical section" but should be fast enough.
    // For extreme safety, one could disable interrupts here.
    segBuf[0] = s3; // Digit 3 (Thousands)
    segBuf[1] = s2; // Digit 2 (Hundreds)
    segBuf[2] = s1; // Digit 1 (Tens)
    segBuf[3] = s0; // Digit 0 (Ones)
}


// --- Public Function Implementations ---

void Display_Init(void)
{
    // 1. Set all AN pins to digital I/O
    ADCON1 = 0x0F;
    CMCON  = 0x07;

    // 2. Pre-load latches to "all OFF" *before* setting TRIS to output
    LATA = 0x00;          // Digits off (Common Anode assumed: 0 = on)
    LATD = 0x00;          // All segments off
    
    // 3. Set PortD (segments) and PortA (digit select) to outputs
    TRISD = 0x00;         // RD7..0 outputs
    TRISA &= 0xF0;        // RA3..0 outputs (preserve RA4+)

    // 4. Set the display to its initial state ("   0")
    s_UpdateSegBuf(0, 1); // Blank leading zeros
}

void Display_SetScore(uint16_t score, uint8_t blankLeadingZeros)
{
    // This is the public "setter" function.
    s_UpdateSegBuf(score, blankLeadingZeros);
}

void Display_Scan_ISR(void)
{
    // This static variable persists between calls
    static uint8_t idx = 0; // idx = the digit we are about to light up

    // 1. Turn OFF the *previous* digit to prevent ghosting
    // We use (idx - 1) & 3 to get the previous index (wraps 0-1 = 3)
    switch((idx - 1) & 3)
    { 
        case 0:  LATAbits.LATA3 = 0; break; // Turn off Digit 3 (RA3)
        case 1:  LATAbits.LATA2 = 0; break; // Turn off Digit 2 (RA2)
        case 2:  LATAbits.LATA1 = 0; break; // Turn off Digit 1 (RA1)
        default: LATAbits.LATA0 = 0; break; // Turn off Digit 0 (RA0)
    }

    // 2. Set the segment data (PORTD) for the *new* digit
    // We read from the volatile buffer
    LATD = segBuf[idx];

    // 3. Turn ON the *new* digit
    switch(idx)
    {
        case 0:  LATAbits.LATA3 = 1; break; // Turn on Digit 3
        case 1:  LATAbits.LATA2 = 1; break; // Turn on Digit 2
        case 2:  LATAbits.LATA1 = 1; break; // Turn on Digit 1
        default: LATAbits.LATA0 = 1; break; // Turn on Digit 0
    }
    
    // 4. Move to the next digit for the next ISR call
    idx = (idx + 1) & 3; // (0, 1, 2, 3, 0, 1...)
}