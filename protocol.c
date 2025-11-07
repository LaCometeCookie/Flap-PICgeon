#include <xc.h>
#include <string.h>     // For strncmp()
#include <stdlib.h>     // For strtol() (string to long)
#include <stdio.h>      // For sprintf()

#include "protocol.h"   // Our own header
#include "display.h"    // To set the 7-segment display
#include "eeprom.h"     // To read/write best scores (we'll create this next)
#include "usb_cdc_lib.h"// For sending data

// --- Private Module Variables ---

// Buffer to assemble commands from the PC (handles partial messages)
static char line_buffer[32];
static uint8_t line_idx = 0;

// Internal state
static uint8_t  current_slot = 0;
static uint16_t current_score = 0;
static uint16_t best_score = 0;

// Buffer for formatting messages to send *to* the PC
static char usbWriteBuffer[32];


// --- Private Helper Functions (Send) ---

/**
 * @brief Sends a pre-formatted, null-terminated string to the PC via USB.
 * @param str The string to send.
 */
static void s_SendString(const char* str)
{
    // Check if the USB port is ready to send
    if (isUSBReady())
    {
        // putUSBUSART sends a buffer of a specific length
        putUSBUSART((char*)str, strlen(str));
    }
}

/**
 * @brief Sends the "CS:READY" command.
 */
static void s_SendReady(void)
{
    // Format the string into our write buffer
    sprintf(usbWriteBuffer, "CS:READY,PROTO=1\r\n");
    s_SendString(usbWriteBuffer);
}

/**
 * @brief Sends the "CS:BEST" command with the given score.
 * @param score The score to send.
 */
static void s_SendBest(uint16_t score)
{
    sprintf(usbWriteBuffer, "CS:BEST,%u\r\n", score);
    s_SendString(usbWriteBuffer);
}


// --- Private Helper Functions (Receive/Parse) ---

/**
 * @brief Handles the "CC:SEL" (Select Slot) command.
 * @param args Pointer to the argument part of the string (e.g., "1").
 */
static void s_Handle_SEL(char* args)
{
    uint8_t slot = (uint8_t)strtol(args, NULL, 10);
    if (slot > 3) slot = 3; // Clamp to max slot 3

    current_slot = slot;
    best_score = EEPROM_ReadBestScore(current_slot); // Read new best score
    
    // Show the best score for the newly selected slot on the 7-segment display
    Display_SetScore(best_score, 1); // 1 = blank leading zeros
}

/**
 * @brief Handles the "CC:A" (Angle) command.
 * @param args Pointer to the argument part of the string (e.g., "90").
 */
static void s_Handle_A(char* args)
{
    uint16_t angle = (uint16_t)strtol(args, NULL, 10);
    if (angle > 180) angle = 180; // Clamp to 180 as per spec

    // TODO: Use this angle value
    // (e.g., set a PWM duty cycle for a servo or LED bargraph)
}

/**
 * @brief Handles the "CC:S" (Live Score) command.
 * @param args Pointer to the argument part of the string (e.g., "12").
 */
static void s_Handle_S(char* args)
{
    current_score = (uint16_t)strtol(args, NULL, 10);
    if (current_score > 999) current_score = 999; // Clamp to spec range

    // Update the 7-segment display with the new live score
    Display_SetScore(current_score, 1); // 1 = blank leading zeros
}

/**
 * @brief Handles the "CC:GO" (Game Over) command.
 * Checks if the live score beat the best score and saves if it did.
 * Sends the best score back to the PC.
 */
static void s_Handle_GO(void)
{
    if (current_score > best_score)
    {
        best_score = current_score;
        EEPROM_WriteBestScore(current_slot, best_score);
    }
    
    // Respond with the best score for this slot
    s_SendBest(best_score);
}

/**
 * @brief Handles the "CC:RB" (Request Best) command.
 * Sends the best score for the current slot back to the PC.
 */
static void s_Handle_RB(void)
{
    s_SendBest(best_score);
}


/**
 * @brief Dispatches a completed, null-terminated line to the correct handler.
 * @param line The command line to parse (e.g., "CC:S,10").
 */
static void s_ParseLine(char* line)
{
    // Use strncmp for safe, length-limited comparison
    
    if (strncmp(line, "CC:SEL,", 7) == 0)
    {
        s_Handle_SEL(&line[7]); // Pass pointer to args
    }
    else if (strncmp(line, "CC:A,", 5) == 0)
    {
        s_Handle_A(&line[5]);
    }
    else if (strncmp(line, "CC:S,", 5) == 0)
    {
        s_Handle_S(&line[5]);
    }
    else if (strncmp(line, "CC:GO,1", 7) == 0)
    {
        s_Handle_GO();
    }
    else if (strncmp(line, "CC:RB", 5) == 0)
    {
        s_Handle_RB();
    }
    // (Can add CC:CAL and CC:PING handlers here later)
}


// --- Public Function Implementations ---

void Protocol_Init(uint16_t initial_best_score)
{
    current_slot = 0;
    current_score = 0;
    best_score = initial_best_score;
    line_idx = 0;
    
    // Wait a moment for PC to be ready, then send READY
    // (This delay might be handled in main.c)
    s_SendReady();
}

void Protocol_ParseBuffer(const char* buffer, uint8_t len)
{
    for(uint8_t i = 0; i < len; i++)
    {
        char c = buffer[i];
        
        // Check for end-of-line characters
        if (c == '\r' || c == '\n')
        {
            if (line_idx > 0) // We have a complete line
            {
                // Null-terminate the string in our buffer
                line_buffer[line_idx] = '\0'; 
                
                // Dispatch the command
                s_ParseLine(line_buffer);
            }
            // Reset for next line
            line_idx = 0; 
        }
        else
        {
            // Add character to our line buffer
            if (line_idx < (sizeof(line_buffer) - 1))
            {
                line_buffer[line_idx++] = c;
            }
            else
            {
                // Overflow, reset the buffer
                line_idx = 0;
            }
        }
    }
}

void Protocol_SendButtonPress(void)
{
    // Format and send the button press command
    sprintf(usbWriteBuffer, "CS:BTN,1\r\n");
    s_SendString(usbWriteBuffer);
}