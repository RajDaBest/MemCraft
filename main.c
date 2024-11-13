#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define CAPACITY (65536) /* 64 KiloBytes */

uint8_t heap[CAPACITY];
size_t heap_size = 0;

void *heap_alloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    assert((size_t)CAPACITY >= size);
    assert(heap_size <= (size_t)CAPACITY - size);
    return NULL;
}

void heap_free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }
}

int main(void)
{
    return EXIT_SUCCESS;
}