#include "sysconfig.h"
#include <xc.h>
#include "main.h"
#include "usb_cdc_lib.h"

#define _XTAL_FREQ 48000000 // System clock frequency for delays

// --- Etats d'affichage / logique ---
static volatile uint8_t segBuf[4] = {0}; // patrons segments pour 4 digits
static uint16_t prevScore = 0;
static uint16_t lastBeepMs = 0;          // si tu as un tick ms
static uint16_t nowMs = 0;               // idem
static uint16_t angleDeg = 0;            // 0..360 (stocké pour ton usage)






// Table segments (common-cathode). Inverse (~) si common-anode.
static const uint8_t segMap[10] = {
    0b00111111, //0
    0b00000110, //1
    0b01011011, //2
    0b01001111, //3
    0b01100110, //4
    0b01101101, //5
    0b01111101, //6
    0b00000111, //7
    0b01111111, //8
    0b01101111  //9
};

// --- helpers ---
static void updateSegBufFromScore(uint16_t score, uint8_t blankLeadingZeros)
{
    if(score > 9999) score = 9999;
    uint8_t d3 = (score / 1000) % 10;
    uint8_t d2 = (score / 100)  % 10;
    uint8_t d1 = (score / 10)   % 10;
    uint8_t d0 = score % 10;

    // par défaut, tout allumé selon mapping
    uint8_t s3 = segMap[d3];
    uint8_t s2 = segMap[d2];
    uint8_t s1 = segMap[d1];
    uint8_t s0 = segMap[d0];

    if(blankLeadingZeros) {
        if(score < 1000) s3 = 0;            // vide
        if(score < 100)  s2 = 0;
        if(score < 10)   s1 = 0;
        // s0 reste affiché même si score==0
    }

    segBuf[0] = s3; // Digit gauche
    segBuf[1] = s2;
    segBuf[2] = s1;
    segBuf[3] = s0; // Digit droit
}

static void handleScoreLine(const char* p) // p pointe sur les 4 chiffres "dddd"
{
    // Vérif stricte: 4 chiffres
    if(!(p[0]>='0'&&p[0]<='9' && p[1]>='0'&&p[1]<='9' && p[2]>='0'&&p[2]<='9' && p[3]>='0'&&p[3]<='9'))
        return;

    uint16_t val = (uint16_t)( (p[0]-'0')*1000 + (p[1]-'0')*100 + (p[2]-'0')*10 + (p[3]-'0') );
    updateSegBufFromScore(val, 1); // masquer zéros de tête

    // Loigique Buzzer à implémenter
    prevScore = val;
}

static void handleAngleLine(const char* p) // p pointe sur les 3 chiffres "ddd"
{
    // Vérif stricte: 3 chiffres
    if(!(p[0]>='0'&&p[0]<='9' && p[1]>='0'&&p[1]<='9' && p[2]>='0'&&p[2]<='9'))
        return;

    uint16_t val = (uint16_t)( (p[0]-'0')*100 + (p[1]-'0')*10 + (p[2]-'0') );
    if(val > 360) val = 360; // clamp
    angleDeg = val;
    // TODO: utiliser angleDeg (PWM/LED/bargraph?), mais ne pas bloquer ici
}

static char line[16];     // assez pour "S,dddd" ou "A,ddd" + CR
static uint8_t idx = 0;

static inline void SevenSeg_Init(void){
    // all digital first
    ADCON1 = 0x0F;
    CMCON  = 0x07;

    // preload latches to "all OFF" *before* TRIS = outputs
    LATA = 0x00;          // digits off (adjust if CA)
    LATD = 0x00;          // all segments off

    // then make them outputs
    TRISA &= 0xF0;        // RA3..0 outputs
    TRISD = 0x00;         // RD7..0 outputs
}


static inline void SevenSeg_ScanOnce(void){
    static uint8_t idx = 0; // idx = le digit qu'on va allumer MAINTENANT

    // 1. Eteindre le digit PRECEDENT
    //    On utilise (idx - 1) & 3 pour trouver le digit d'avant
    //    (par ex. si idx=0, on éteint 3. si idx=1, on éteint 0)
    switch((idx - 1) & 3){ 
        case 0:  LATAbits.LATA3 = 0; break; // Eteint Digit 3 (RA3)
        case 1:  LATAbits.LATA2 = 0; break; // Eteint Digit 2 (RA2)
        case 2:  LATAbits.LATA1 = 0; break; // Eteint Digit 1 (RA1)
        default: LATAbits.LATA0 = 0; break; // Eteint Digit 0 (RA0)
    }

    // 2. Mettre les bons segments pour le NOUVEAU digit
    LATD = segBuf[idx];

    // 3. Allumer le NOUVEAU digit
    switch(idx){
        case 0:  LATAbits.LATA3 = 1; break; // Allume Digit 3
        case 1:  LATAbits.LATA2 = 1; break; // Allume Digit 2
        case 2:  LATAbits.LATA1 = 1; break; // Allume Digit 1
        default: LATAbits.LATA0 = 1; break; // Allume Digit 0
    }
    
    idx = (idx + 1) & 3; // Préparer pour le prochain appel
}
void main(void) 
{
    
    // VITAL: Configure PORTA pins as DIGITAL inputs
    ADCON1 = 0x0F;
    CMCON = 0x07;
    
    // PORTD will show the score (binary)
    /*
    PORTD = 0x00;
    TRISD = 0x00;      // all PORTD pins as outputs
    LATD  = 0x00;      // start at 0
    */
    SevenSeg_Init();
    
    updateSegBufFromScore(0, 1);
    
    __delay_ms(20);
    
        //Init Timer 2
    // PR2 = 249, prescaler = 1:16, postscaler = 1:3
    PR2   = 249;
    T2CON = 0x1D;            // T2OUTPS=0010 (1:3), TMR2ON=1, T2CKPS=11 (1:16)
    PIR1bits.TMR2IF = 0;     // clear flag
    PIE1bits.TMR2IE = 1;     // enable IT Timer2

    
    BUTTON_TRIS = 1;    // Set RA0 as an input for the button
    

    initUSBLib();
    
    INTCONbits.PEIE  = 1;    // enable peripheral IT
    INTCONbits.GIE   = 1;    // enable global IT
    
        // A variable to prevent sending "4" continuously while the button is held
    char button_was_pressed = 0;
    
    while(1)
    {
        USBDeviceTasks();
        // === PART 1: Check for commands from the computer ===
        if(isUSBReady())
            {
            uint8_t n = getsUSBUSART(usbReadBuffer, sizeof(usbReadBuffer));
            for(uint8_t i=0;i<n;i++){
                char c = (char)usbReadBuffer[i];

                if(c=='\r' || c=='\n'){
                    // ligne complète ?
                    if(idx >= 3){ // min "S,0" ou "A,0"
                        line[idx] = '\0';
                    // Dispatch
                    if(line[0]=='S' && line[1]==',' && idx>=6){      // "S,dddd"
                        handleScoreLine(&line[2]);
                    } else if(line[0]=='A' && line[1]==',' && idx>=5){ // "A,ddd"
                        handleAngleLine(&line[2]);
                    }
                }
                idx = 0; // reset pour la prochaine ligne
                } else {
                    if(idx < sizeof(line)-1){
                        line[idx++] = c;
                    } else {
                        idx = 0; // overflow -> reset propre
                        }
                    }
                }
            }
        
        // === PART 2: Check if the button on RA0 is pressed (PULL-DOWN LOGIC) ===
        
        // Is the button pressed (pin is HIGH) AND it was not pressed before?
        if(BUTTON_PIN == 1 && button_was_pressed == 0) // <-- MODIFIED to check for 1
        {
            //__delay_ms(20); // Debounce delay
            if(BUTTON_PIN == 1) // Check again after delay
            {
                putUSBUSART("4", 1);      // Send "4" followed by a newline
                button_was_pressed = 1;     // Mark the button as pressed
            }
        }
        // If the button is released (pin is LOW), reset the flag
        else if(BUTTON_PIN == 0) // <-- MODIFIED to check for 0
        {
            button_was_pressed = 0;
        }

        // Keep the USB services running
        CDCTxService();
    }
    
    return;
}

void __interrupt() mainISR (void)
{
    if (PIR1bits.TMR2IF) {
        PIR1bits.TMR2IF = 0;
        nowMs++;            // ? 1 ms par interruption
        SevenSeg_ScanOnce();
        // (tu peux aussi faire ici tes tâches ?temps? : couper buzzer, etc.)
    }
    
    processUSBTasks();
}