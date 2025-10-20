#ifndef MAIN_H
#define	MAIN_H

#include <xc.h> // Include standard PIC definitions

// --- Type Definitions ---
#ifndef bool
    #define bool    unsigned char
    #define true    1
    #define false   0
#endif

// --- Pin Definitions ---

// Button pins (from main.c, for INT0 interrupt)
#define BUTTON_PIN      PORTBbits.RB0
#define BUTTON_TRIS     TRISBbits.TRISB0

// --- USB Buffers ---
static unsigned char usbReadBuffer[32];
static unsigned char usbWriteBuffer[32];


#endif	/* MAIN_H */