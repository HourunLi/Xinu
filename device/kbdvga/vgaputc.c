#include <xinu.h>
#include "kbdvga.h"

void scrollUpScreen() {
    // Move everything in the buffer except the last line
    memmove(
        TEXT_MODE_BUFFER,                // destination: buffer address from 0-th line
        TEXT_MODE_BUFFER + SCREEN_WIDTH, // source: buffer address from 1th line
        (int32)SCREEN_WIDTH * (SCREEN_HEIGHT - 1) * sizeof(uint16)
    );

    // Fill the last line
    memset16(
        TEXT_MODE_BUFFER + (int32)SCREEN_WIDTH * (SCREEN_HEIGHT - 1),
        SCREEN_WIDTH,
        PACK(BLACK_WHITE, TY_BLANK)
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
    TEXT_MODE_BUFFER[index] = PACK(BLACK_WHITE, TY_BLANK);
}

devcall vgaputc(struct dentry *devptr, char ch) {
    switch(ch) {
    case TY_RETURN:
        newLine();
        break;
    case TY_TAB:
        for (int32 i = 0; i < 8 - (cursor.column % 8); i++)
            vgaputc(devptr, TY_BLANK);
        break;
    case TY_BACKSP:
        backspace();
        break;
    default:
        if(ch < TY_BLANK || ch == 0x7f) {
            vgaputc(devptr, TY_UPARROW);
            vgaputc(devptr, ch+0100);
            return;
        }
        TEXT_MODE_BUFFER[getCursorPosition(cursor.row, cursor.column)] = PACK(BLACK_WHITE, ch);
        if (cursor.column == SCREEN_WIDTH - 1)
            newLine();
        else
            setCursorPosition(cursor.row, cursor.column + 1);
    }
    return OK;
}

