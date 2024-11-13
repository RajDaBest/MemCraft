#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define HEAP_CAPACITY (65536)      /* 64 KiloBytes */
#define HEAP_ALLOC_CAPACITY (1024) /* 1 KiloByte */
#define HEAP_FREED_CAPACITY (1024) /* 1 KiloByte */

#define DEFAULT_ALIGNMENT (ALIGN_8)

typedef enum
{
    ALIGN_4,
    ALIGN_8,
    ALIGN_16,
    ALIGN_DEFAULT = DEFAULT_ALIGNMENT,
} alignment_t;

typedef struct
{
    void *chunk_start;
    size_t chunk_size;
} heap_chunk_t;

static heap_chunk_t *heap_alloced[HEAP_ALLOC_CAPACITY] = {0};
static size_t heap_alloced_size = 0;

static uint8_t heap[HEAP_CAPACITY] = {0};
static size_t heap_size = 0;

static heap_chunk_t heap_freed[HEAP_FREED_CAPACITY] = {(heap_chunk_t){.chunk_size = HEAP_CAPACITY, .chunk_start = (void *)heap}, 0};
static size_t heap_freed_size = 1;

void *heap_alloc(size_t size, alignment_t alignment)
{
    if (size == 0 || (size_t)HEAP_CAPACITY >= size || heap_size <= (size_t)HEAP_CAPACITY - size)
    {
        return NULL;
    }

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