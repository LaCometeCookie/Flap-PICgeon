#ifndef EEPROM_H
#define	EEPROM_H

#include <stdint.h> // For uint16_t, uint8_t

/**
 * @brief Writes a 16-bit best score to a specific slot in the EEPROM.
 *
 * @param slot The slot to write to (0-3).
 * @param score The 16-bit score to save.
 */
void EEPROM_WriteBestScore(uint8_t slot, uint16_t score);

/**
 * @brief Reads a 16-bit best score from a specific slot in the EEPROM.
 *
 * @param slot The slot to read from (0-3).
 * @return The 16-bit score.
 */
uint16_t EEPROM_ReadBestScore(uint8_t slot);


#endif	/* EEPROM_H */