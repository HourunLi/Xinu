#include <xinu.h>
#include "kbdvga.h"


void memset16(void *dest, int32 count, uint16 value) {
    uint16 *dest16 = (uint16 *)dest;
    for (int32 i = 0; i < count; i++)
        dest16[i] = value;
}

uint16 getCursorPosition(uint8 row, uint8 column) {
    return (uint16)row * SCREEN_WIDTH + column;
}

void setCursorPosition(uint8 row, uint8 column) {
    uint16 index = getCursorPosition(row, column);
 
    outb(0x3D4, 15);
    outb(0x3D5, index & 0xFF);
    outb(0x3D4, 14);
    outb(0x3D5, (index >> 8) & 0xFF);

    cursor.row = row;
    cursor.column = column;
}

devcall kbdinit(struct dentry *devptr) {
    kbdcb.tyisem = semcreate(0);
    if (kbdcb.tyisem == SYSERR)
        return SYSERR;

    kbdcb.tyihead = kbdcb.tyitail = 0;
    kbdcb.tyicursor = 0;
    set_evec(devptr->dvirq, (uint32)devptr->dvintr);

    return OK;
}

void clearScreen() {
    memset16(
        TEXT_MODE_BUFFER,
        (int32)SCREEN_WIDTH * SCREEN_HEIGHT,
        PACK(BLACK_WHITE, ' ')
    );
}

devcall vgainit(struct dentry *devptr) {
    clearScreen();
    setCursorPosition(0, 0);
    return OK;
}

extern devcall kbdvgainit(struct dentry *devptr) {
    kbdinit(devptr);
    vgainit(devptr);
    return OK;
}