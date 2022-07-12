#include <xinu.h>
#include "kbdvga.h"

void scrollUpScreen() {
    // Move everything in the buffer except the last line
    memmove(
        TEXT_MODE_BUFFER,                // destination: buffer address from 0-th line
        TEXT_MODE_BUFFER + SCREEN_WIDTH, // source: buffer address from 1-st line
        (int32)SCREEN_WIDTH * (SCREEN_HEIGHT - 1) * sizeof(uint16)
    );

    // Fill the last line
    memset16(
        TEXT_MODE_BUFFER + (int32)SCREEN_WIDTH * (SCREEN_HEIGHT - 1),
        SCREEN_WIDTH,
        PACK(BLACK_WHITE, ' ')
    );
}

void newLine() {
    if (cursor.row == SCREEN_HEIGHT - 1) {
        scrollUpScreen();
        setCursorPosition(SCREEN_HEIGHT - 1, 0);
    } else {
        setCursorPosition(cursor.row + 1, 0);
    }
}

void backspace() {
    if (cursor.column == 0 && cursor.row == 0)
        // At first position of screen
        return;

    if (cursor.column != 0)
        // In current line
        setCursorPosition(cursor.row, cursor.column - 1);
    else
        // Move to previous line
        setCursorPosition(cursor.row - 1, SCREEN_WIDTH - 1);
    uint16 index = getCursorPosition(cursor.row, cursor.column);
    TEXT_MODE_BUFFER[index] = PACK(BLACK_WHITE, ' ');
}

devcall vgaputc(struct dentry *devptr, char ch) {
    // kprintf("vgaputc: char = [%d] %c\n", (int)ch, ch);
    switch(ch) {
    case '\n':
        newLine();
        break;
    case '\t':
        for (int32 i = 0; i < 8 - (cursor.column % 8); i++)
            vgaputc(devptr, ' ');
        break;
    case '\b':
        backspace();
        break;
    default:
        TEXT_MODE_BUFFER[getCursorPosition(cursor.row, cursor.column)] = PACK(BLACK_WHITE, ch);
        if (cursor.column == SCREEN_WIDTH - 1)
            newLine();
        else
            setCursorPosition(cursor.row, cursor.column + 1);
    }
    return OK;
}

