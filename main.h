#ifndef MAIN_H
#define	MAIN_H


// --- Pin Definitions ---
#define BUTTON_PIN      PORTAbits.RA1 // Using PORTA to read
#define BUTTON_TRIS     TRISAbits.TRISA1


// --- Type Definitions ---
#ifndef bool
    #define bool    unsigned char
    #define true    1
    #define false   0
#endif


static unsigned char usbReadBuffer[32];
static unsigned char usbWriteBuffer[32];

#endif	/* MAIN_H */