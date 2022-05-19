/**
 * @file stdlib.h
 *
 * $Id: stdlib.h 2051 2009-08-27 20:55:09Z akoehler $
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */
#include <kernel.h>
#define RAND_MAX 2147483646

int abs(int);
long labs(long);
int atoi(char *);
long atol(char *);
void bzero(void *, int);
void qsort(char *, unsigned int, int, int (*)(void));
int rand(void);
void srand(unsigned int);
syscall *malloc(unsigned int nbytes);
syscall free(void *pmem);
syscall *realloc(void *ptr, uint32 size);
