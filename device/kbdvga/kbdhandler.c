#include <xinu.h>
#include "kbdvga.h"

char kbdGetChar() {
    static uint32 shift;
    static uint8 *charcode[4] = {
        normalmap, shiftmap, ctlmap, ctlmap
    };
    uint32 st, data, c;

    st = inb(KBSTATP);
    if((st & KBS_DIB) == 0)
        return -1;
    data = inb(KBDATAP);

    if(data == 0xE0) {
        shift |= E0ESC;
        return 0;
    } else if(data & 0x80){
        // Key released
        data = (shift & E0ESC ? data : data & 0x7F);
        shift &= ~(shiftcode[data] | E0ESC);
        return 0;
    } else if(shift & E0ESC) {
        // Last character was an E0 escape; or with 0x80
        data |= 0x80;
        shift &= ~E0ESC;
    }

    shift |= shiftcode[data];
    shift ^= togglecode[data];
    c = charcode[shift & (CTL | SHIFT)][data];
    if(shift & CAPSLOCK) {
        if('a' <= c && c <= 'z')
        c += 'A' - 'a';
        else if('A' <= c && c <= 'Z')
        c += 'a' - 'A';
    }

    return c;
}

void kbdhandler(void) {
    // processing irq needs to forbid reschedule
    resched_cntl(DEFER_START);

    while (1) {
        // Get key
        char ch = kbdGetChar();
        if (ch == -1 || ch == 0 || semcount(kbdcb.tyisem) >= KBD_BUFFER_SIZE) break;

        // Append to buffer
        kbdcb.tyibuff[kbdcb.tyitail++] = ch;
        kbdcb.tyitail %= KBD_BUFFER_SIZE;
        signal(kbdcb.tyisem);
    }

    resched_cntl(DEFER_STOP);
}