#include "sysconfig.h"
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "usb_cdc_lib.h"


// --------- Board/IO ----------
#define PORTD_ACTIVE_LOW 1     // set to 0 if your LED block is active-high

#define BUZZER_PIN      LATCbits.LATC2
#define BUZZER_TRIS     TRISCbits.TRISC2

#define BUTTON_PIN      PORTAbits.RA1     // your code used RA1
#define BUTTON_TRIS     TRISAbits.TRISA1

// Show a byte on PORTD, handling LED polarity
static inline void show_on_portd(uint8_t v) {
#if PORTD_ACTIVE_LOW
    LATD = ~v;     // active-low LED bar
#else
    LATD = v;
#endif
}

// --------- EEPROM (internal, PIC18F4550) ----------
static inline uint8_t ee_read(uint8_t addr) {
   EEADR  = addr;
    EECON1bits.EEPGD = 0;          // data EEPROM
    EECON1bits.CFGS  = 0;          // not config space
    EECON1bits.RD    = 1;          // start read
    return EEDATA;
}

// returns true if write + verify succeeded
static bool ee_write_verify(uint8_t addr, uint8_t val) {
    while (EECON1bits.WR) { }      // wait any prior write

    EEADR  = addr;
    EEDATA = val;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS  = 0;
    EECON1bits.WREN  = 1;

    uint8_t gie = INTCONbits.GIE;  // required unlock sequence
    INTCONbits.GIE = 0;
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    INTCONbits.GIE = gie;

    while (EECON1bits.WR) { }      // wait internal write complete
    EECON1bits.WREN = 0;

    // verify
    return (ee_read(addr) == val);
}

// --------- Tiny utils ----------
static void debounce_delay(void){ for(volatile uint32_t i=0;i<50000UL;i++){} }

static void send_hex_line(uint8_t v) {
    char msg[16];
    // prints like: 0x5A\r\n
    msg[0]='0'; msg[1]='x';
    const char hex[]="0123456789ABCDEF";
    msg[2]=hex[(v>>4)&0x0F];
    msg[3]=hex[v&0x0F];
    msg[4]='\r'; msg[5]='\n'; msg[6]=0;
    putUSBUSART((uint8_t*)msg, 6);
}

static void send_status(const char* s) {
    // send short status strings
    const char* p = s;
    uint8_t buf[48]; uint8_t n=0;
    while (*p && n<sizeof(buf)) buf[n++]=(uint8_t)(*p++);
    if (n) putUSBUSART(buf, n);
}

// --------- Init ----------
static void init_hw(void) {
    // Digital I/O
    ADCON1 = 0x0F;     // all digital
    CMCON  = 0x07;     // comparators off

#ifdef ANSELD
    ANSELD = 0x00;
#endif
#ifdef ANSELA
    ANSELA = 0x00;
#endif

    // Buzzer
    BUZZER_TRIS = 0;
    BUZZER_PIN  = 0;

    // Button
    BUTTON_TRIS = 1;

    // PORTD LED bar
    TRISD = 0x00;
    LATD  = PORTD_ACTIVE_LOW ? 0xFF : 0x00; // all off visually
}

// ======================================================
void main(void)
{
    char button_was_pressed = 0;

    initUSBLib();
    init_hw();

    while (1)
    {
        USBDeviceTasks();

        // --- USB RX: simple protocol ---
        if (isUSBReady())
        {
            uint8_t numBytesRead = getsUSBUSART(usbReadBuffer, sizeof(usbReadBuffer));
            if (numBytesRead > 0)
            {
                char cmd = (char)usbReadBuffer[0];

                if (cmd == '1' || cmd == '2') {
                    uint8_t val = (cmd == '1') ? 0x01 : 0x02;
                    bool ok = ee_write_verify(0x00, val);
                    show_on_portd(val);
                    if (ok) send_status("WROTE OK\r\n");
                    else    send_status("WRITE FAIL\r\n");
                }
                else if (cmd == 'r' || cmd == 'R') {
                    uint8_t v = ee_read(0x00);
                    show_on_portd(v);
                    send_hex_line(v);         // e.g., 0x5A
                }
                else {
                    // echo unknown, also mirror byte to LEDs for debugging
                    show_on_portd((uint8_t)cmd);
                    send_status("?\r\n");
                }
            }
        }

        // --- Button on RA1: read and report once per press ---
        if (BUTTON_PIN == 1 && button_was_pressed == 0) {
            debounce_delay();
            if (BUTTON_PIN == 1) {
                uint8_t v = ee_read(0x00);
                show_on_portd(v);
                send_hex_line(v);
                button_was_pressed = 1;
            }
        } else if (BUTTON_PIN == 0) {
            button_was_pressed = 0;
        }

        CDCTxService();
    }
}

// If your stack needs it:
void __interrupt() mainISR(void)
{
    processUSBTasks();
}
