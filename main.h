#ifndef MAIN_H
#define	MAIN_H


// --- Pin Definitions ---
#define BUTTON_PIN      PORTEbits.RE0 // Using PORTE to read
#define BUTTON_TRIS     TRISEbits.TRISE0


// --- Type Definitions ---
#ifndef bool
    #define bool    unsigned char
    #define true    1
    #define false   0
#endif



static unsigned char usbReadBuffer[32];
static unsigned char usbWriteBuffer[32];

#endif	/* MAIN_H */