#include <xinu.h>
#include "kbdvga.h"

devcall	kbdgetc(struct dentry *devptr) {
    wait(kbdcb.tyisem);         // consume one slot
    int i = kbdcb.tyihead;
    char ch = kbdcb.tyibuff[kbdcb.tyihead++];
    kbdcb.tyihead %= KBD_BUFFER_SIZE;
    return ch;
}
