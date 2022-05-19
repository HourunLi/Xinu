#pragma once
#include <kernel.h>

extern int mm_init(void);
extern void *malloc(uint32 size);
extern void free(void *ptr);
extern void *realloc(void *ptr, uint32 size);