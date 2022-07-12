#include<xinu.h>
#include "kbdvga.h"

devcall	kbdread(
    struct dentry *devptr, /* Entry in device switch table */
    char	*buff,         /* Buffer to hold bytes		*/
    int32 count            /* Max bytes to read		*/
) {
    char ch;
    int32 cnt = 0;
    while (cnt < count && (ch = kbdgetc(devptr)) != '\n')
        buff[cnt++] = ch;
    return cnt;
}