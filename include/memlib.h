#pragma once
#include <kernel.h>
void mem_init();
void *mem_sbrk(int32 delta);
void mem_reset_brk(); 
void *mem_heap_lo();
void *mem_heap_hi();
uint32 mem_heapsize();
uint32 mem_pagesize();

