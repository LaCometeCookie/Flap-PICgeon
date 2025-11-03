#ifndef MAIN_H
#define	MAIN_H

// --- Pin Definitions ---
// Define the button pin (PORTE, pin 0)
#define BUTTON_PIN      PORTEbits.RE0
#define BUTTON_TRIS     TRISEbits.TRISE0


// --- Type Definitions ---
// Basic boolean type definitions
#ifndef bool
    #define bool    unsigned char
    #define true    1
    #define false   0
#endif

#endif	/* MAIN_H */