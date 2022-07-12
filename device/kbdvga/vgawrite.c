#include <xinu.h>
#include "kbdvga.h"

devcall vgawrite(struct dentry *devptr, char *buff, int32 count) {
    for (int32 i = 0; i < count; i++)
        vgaputc(devptr, buff[i]);

    return OK;
}
