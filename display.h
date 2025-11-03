#ifndef DISPLAY_H
#define	DISPLAY_H

#include <stdint.h> // For uint16_t, uint8_t

/**
 * @brief Initializes TRIS registers for the 74HC595 shift registers.
 * Sets RA0, RA1, RA2 as outputs and clears the display.
 */
void Display_Init(void);

/**
 * @brief Updates the internal display buffer with a new score.
 *
 * @param score The 4-digit score to display (0-9999).
 * @param blankLeadingZeros 1 to blank leading zeros (e.g., "  21"), 0 to show them (e.g., "0021").
 */
void Display_SetScore(uint16_t score, uint8_t blankLeadingZeros);

/**
 * @brief Scans one digit of the 7-segment display via the 74HC595s.
 * This function is designed to be called rapidly from a periodic interrupt
 * (like your Timer2 ISR) to handle the multiplexing.
 */
void Display_Scan_ISR(void);

#endif	/* DISPLAY_H */