#ifndef PROTOCOL_H
#define	PROTOCOL_H

#include <stdint.h> // For uint8_t, uint16_t
#include "glcd.h"
/**
 * @brief Initializes the protocol handler and sends the "Ready" message.
 * Call this once at startup after initializing EEPROM and USB.
 *
 * @param initial_best_score The best score loaded from EEPROM for the default slot (slot 0).
 */
void Protocol_Init(uint16_t initial_best_score);

/**
 * @brief Parses a raw buffer of data received from the USB CDC port.
 * This function handles partial commands (line buffering) and dispatches
 * completed commands to the correct handlers.
 *
 * @param buffer Pointer to the raw data (e.g., usbReadBuffer).
 * @param len Number of bytes received in the buffer.
 */
void Protocol_ParseBuffer(const char* buffer, uint8_t len);

/**
 * @brief Sends the button press command "CS:BTN,1\r\n" to the computer.
 * Call this from the main loop when a button press is detected.
 */
void Protocol_SendButtonPress(void);


#endif	/* PROTOCOL_H */