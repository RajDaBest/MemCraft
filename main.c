#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define CAPACITY (65536) /* 64 KiloBytes */

uint8_t heap[CAPACITY];

void *heap_alloc(size_t size)
{
    return NULL;
}

void heap_free(void *ptr)
{ 
}
