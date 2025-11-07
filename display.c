#include <xc.h>
#include "display.h"

// --- Pin Definitions for 74HC595 ---
#define SR_DATA_LAT     LATAbits.LATA0      // RA0 -> SER (Pin 14)
#define SR_CLOCK_LAT    LATAbits.LATA1      // RA1 -> SRCLK (Pin 11)
#define SR_LATCH_LAT    LATAbits.LATA2      // RA2 -> RCLK (Pin 12)

#define SR_DATA_TRIS    TRISAbits.TRISA0
#define SR_CLOCK_TRIS   TRISAbits.TRISA1
#define SR_LATCH_TRIS   TRISAbits.TRISA2

// --- Private Module Variables ---

static volatile uint8_t segBuf[4] = {10, 10, 10, 0}; // Start with "   0"

// ===================================================================
// === FIX #1: segMap is now active-HIGH (1 = ON) for Common-Cathode ===
// ===================================================================
// 7-segment pattern map (Common-Cathode, 1 = ON)
// segMap[10] is a blank display (all segments OFF)
static const uint8_t segMap[11] = {
    //
    //    gfedcba
    0b00111111, //0
    0b00000110, //1
    0b01011011, //2
    0b01001111, //3
    0b01100110, //4
    0b01101101, //5
    0b01111101, //6
    0b00000111, //7
    0b01111111, //8
    0b01101111, //9
    0x00         //10 (Blank)
};

// --- Private Helper Functions ---

/**
 * @brief Sends 16 bits of data to the two chained 74HC595s.
 * (This function is unchanged)
 */
static void s_ShiftOut16(uint16_t data)
{
    SR_LATCH_LAT = 0;
    
    for (uint8_t i = 0; i < 16; i++)
    {
        if (data & 0x8000)
        {
            SR_DATA_LAT = 1;
        }
        else
        {
            SR_DATA_LAT = 0;
        }
        
        SR_CLOCK_LAT = 1;
        SR_CLOCK_LAT = 0;
        
        data <<= 1;
    }
    
    SR_LATCH_LAT = 1;
}

/**
 * @brief Internal function to calculate digits and update the buffer.
 * (This function is unchanged)
 */
static void s_UpdateSegBuf(uint16_t score, uint8_t blankLeadingZeros)
{
    if(score > 9999) score = 9999;
    
    uint8_t d3 = (score / 1000) % 10; // Thousands
    uint8_t d2 = (score / 100)  % 10; // Hundreds
    uint8_t d1 = (score / 10)   % 10; // Tens
    uint8_t d0 = score % 10;           // Ones
    
    segBuf[0] = (blankLeadingZeros && d3 == 0) ? 10 : d3;
    segBuf[1] = (blankLeadingZeros && d3 == 0 && d2 == 0) ? 10 : d2;
    segBuf[2] = (blankLeadingZeros && d3 == 0 && d2 == 0 && d1 == 0) ? 10 : d1;
    segBuf[3] = d0; 
}


// --- Public Function Implementations ---

void Display_Init(void)
{
    SR_DATA_TRIS = 0;
    SR_CLOCK_TRIS = 0;
    SR_LATCH_TRIS = 0;
    
    SR_DATA_LAT = 0;
    SR_CLOCK_LAT = 0;
    SR_LATCH_LAT = 0;

    // ===================================================================
    // === FIX #2: Send 0xFF00 to turn all digits (IC2) and segments (IC1) OFF ===
    // Digits OFF (Cathodes) = 0xFF (all HIGH)
    // Segments OFF (Anodes) = 0x00 (all LOW)
    // ===================================================================
    s_ShiftOut16(0xFF00);
    
    s_UpdateSegBuf(0, 1);
}

void Display_SetScore(uint16_t score, uint8_t blankLeadingZeros)
{
    s_UpdateSegBuf(score, blankLeadingZeros);
}

void Display_Scan_ISR(void)
{
    static uint8_t idx = 0; 

    // 1. Get the segment pattern (0b00111111 for "0")
    //    This is the data for IC1
    uint8_t segment_data = segMap[segBuf[idx]];

    // ===================================================================
    // === FIX #3: Digit data is now active-LOW (0 = ON) ===
    // This is the data for IC2
    //    idx=0 -> 0b11111110 (D1 ON, others OFF)
    //    idx=1 -> 0b11111101 (D2 ON, others OFF)
    //    idx=2 -> 0b11111011 (D3 ON, others OFF)
    //    idx=3 -> 0b11110111 (D4 ON, others OFF)
    // ===================================================================
    uint8_t digit_data = ~(1 << idx);

    // 4. Combine into a 16-bit word
    //    IC2 (Digits) = digit_data
    //    IC1 (Segments) = segment_data
    uint16_t data_to_send = ((uint16_t)digit_data << 8) | segment_data;
    
    // 5. Send the data
    s_ShiftOut16(data_to_send);
    
    // 6. Move to the next digit
    idx = (idx + 1) & 3; 
}