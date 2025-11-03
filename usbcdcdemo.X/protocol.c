#include <xc.h>
#include <string.h>     // For strncmp()
#include <stdlib.h>     // For strtol() (string to long)
#include <stdio.h>      // For sprintf()

#include "protocol.h"   // Our own header
#include "display.h"    // To set the 7-segment display
#include "eeprom.h"     // To read/write best scores
#include "usb_cdc_lib.h"// For sending data

// --- Private Module Variables ---

static char line_buffer[32];
static uint8_t line_idx = 0;

static uint8_t  current_slot = 0;
static uint16_t current_score = 0;
static uint16_t best_score = 0;

static char usbWriteBuffer[32];

// --- Private Helper Functions (Send) ---

static void s_SendString(const char* str)
{
    if (isUSBReady())
    {
        putUSBUSART((char*)str, strlen(str));
    }
}

static void s_SendReady(void)
{
    sprintf(usbWriteBuffer, "CS:READY,PROTO=1\r\n");
    s_SendString(usbWriteBuffer);
}

static void s_SendBest(uint16_t score)
{
    sprintf(usbWriteBuffer, "CS:BEST,%u\r\n", score);
    s_SendString(usbWriteBuffer);
}


// --- Private Helper Functions (Receive/Parse) ---

static void s_Handle_SEL(char* args)
{
    uint8_t slot = (uint8_t)strtol(args, NULL, 10);
    if (slot > 3) slot = 3; 

    current_slot = slot;
    best_score = EEPROM_ReadBestScore(current_slot); // Read new best score
    
    // ===================================================================
    // === FIX #1: If EEPROM reads 0xFFFF, treat it as 0 ===
    // ===================================================================
    if (best_score == 0xFFFF) {
        best_score = 0;
    }
    
    Display_SetScore(best_score, 1); // Show the best score for the new slot
}

static void s_Handle_A(char* args)
{
    // Read the velocity value from the string
    // We use strtol which is good for integers
    long velocity_int = strtol(args, NULL, 10);

    // Call our new GLCD function to draw the arrow
    GLCD_DrawAngleArrow((float)velocity_int);
}

static void s_Handle_S(char* args)
{
    current_score = (uint16_t)strtol(args, NULL, 10);
    if (current_score > 999) current_score = 999;
    Display_SetScore(current_score, 1);
}

static void s_Handle_GO(void)
{
    // ===================================================================
    // === FIX #2: Check if new score is > best, OR if best is empty ===
    // ===================================================================
    if (current_score > best_score || best_score == 0xFFFF)
    {
        best_score = current_score;
        EEPROM_WriteBestScore(current_slot, best_score);
    }
    
    s_SendBest(best_score);
}

static void s_Handle_RB(void)
{
    // When asked for the best, we should also check if it's 0xFFFF
    // and send 0 instead.
    uint16_t score_to_send = best_score;
    if (score_to_send == 0xFFFF) {
        score_to_send = 0;
    }
    s_SendBest(score_to_send);
}


static void s_ParseLine(char* line)
{
    if (strncmp(line, "CC:SEL,", 7) == 0)
    {
        s_Handle_SEL(&line[7]); 
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
}


// --- Public Function Implementations ---

void Protocol_Init(uint16_t initial_best_score)
{
    current_slot = 0;
    current_score = 0;
    best_score = initial_best_score;
    line_idx = 0;
    
    // ===================================================================
    // === FIX #3: Handle 0xFFFF on the very first init ===
    // ===================================================================
    if (best_score == 0xFFFF) {
        best_score = 0;
    }
    
    s_SendReady();
}

void Protocol_ParseBuffer(const char* buffer, uint8_t len)
{
    for(uint8_t i = 0; i < len; i++)
    {
        char c = buffer[i];
        
        if (c == '\r' || c == '\n')
        {
            if (line_idx > 0)
            {
                line_buffer[line_idx] = '\0'; 
                s_ParseLine(line_buffer);
            }
            line_idx = 0; 
        }
        else
        {
            if (line_idx < (sizeof(line_buffer) - 1))
            {
                line_buffer[line_idx++] = c;
            }
            else
            {
                line_idx = 0;
            }
        }
    }
}

void Protocol_SendButtonPress(void)
{
    sprintf(usbWriteBuffer, "CS:BTN,1\r\n");
    s_SendString(usbWriteBuffer);
}