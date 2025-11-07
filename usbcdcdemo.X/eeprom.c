#include <xc.h>
#include "eeprom.h"

// --- Private Helper Functions ---

/**
 * @brief Reads a single byte from the internal EEPROM at a specific address.
 *
 * @param address The 8-bit EEPROM address (0x00-0xFF).
 * @return The 8-bit data byte read.
 */
static uint8_t s_EEPROM_ReadByte(uint8_t address)
{
    // 1. Set the address to read
    EEADR = address;
    
    // 2. Point to Data EEPROM
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    
    // 3. Start the read
    EECON1bits.RD = 1;
    
    // 4. Return the data
    return EEDATA;
}

/**
 * @brief Writes a single byte to the internal EEPROM at a specific address.
 *
 * @param address The 8-bit EEPROM address (0x00-0xFF).
 * @param data The 8-bit data byte to write.
 */
static void s_EEPROM_WriteByte(uint8_t address, uint8_t data)
{
    // 1. Wait for any previous write to complete
    while (EECON1bits.WR);
    
    // 2. Set the address to write
    EEADR = address;
    
    // 3. Load the data
    EEDATA = data;
    
    // 4. Point to Data EEPROM
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    
    // 5. Enable writes
    EECON1bits.WREN = 1;
    
    // 6. Disable interrupts (required for write sequence)
    uint8_t old_GIE = INTCONbits.GIE; // Save interrupt state
    INTCONbits.GIE = 0;
    
    // 7. Perform the required unlock sequence
    EECON2 = 0x55;
    EECON2 = 0xAA;
    
    // 8. Start the write
    EECON1bits.WR = 1;
    
    // 9. Disable writes
    EECON1bits.WREN = 0;
    
    // 10. Restore interrupts
    INTCONbits.GIE = old_GIE;
}


// --- Public Function Implementations ---

void EEPROM_WriteBestScore(uint8_t slot, uint16_t score)
{
    // Each score is 2 bytes (16-bit). We map slots to addresses.
    // Slot 0: Addr 0, 1
    // Slot 1: Addr 2, 3
    // Slot 2: Addr 4, 5
    // Slot 3: Addr 6, 7
    uint8_t baseAddress = slot * 2;
    
    // Split the 16-bit score into two 8-bit bytes
    uint8_t highByte = (score >> 8) & 0xFF;
    uint8_t lowByte = score & 0xFF;
    
    // Write the bytes to EEPROM
    s_EEPROM_WriteByte(baseAddress, highByte);
    s_EEPROM_WriteByte(baseAddress + 1, lowByte);
}

uint16_t EEPROM_ReadBestScore(uint8_t slot)
{
    uint8_t baseAddress = slot * 2;
    
    // Read the two 8-bit bytes from EEPROM
    uint8_t highByte = s_EEPROM_ReadByte(baseAddress);
    uint8_t lowByte = s_EEPROM_ReadByte(baseAddress + 1);
    
    // Combine them back into a single 16-bit score
    return ((uint16_t)highByte << 8) | lowByte;
}