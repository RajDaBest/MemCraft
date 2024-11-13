#ifndef EFBB842C_C86D_4D43_862A_B356723F50A8
#define EFBB842C_C86D_4D43_862A_B356723F50A8

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_ALIGNMENT (ALIGN_8) // an 8 byte alignment is enough for standard C types
#define MAX_ALIGNMENT (ALIGN_16)
#define HEAP_CAPACITY (65536)

typedef enum
{
    ALIGN_4 = 4,
    ALIGN_8 = 8,
    ALIGN_16 = 16,
    ALIGN_DEFAULT = DEFAULT_ALIGNMENT,
    ALIGN_MAX = MAX_ALIGNMENT,
} alignment_t;

typedef struct __attribute__((packed))
{
    size_t chunk_size;
    bool is_allocated;
    uint8_t _padding[ALIGN_MAX - (sizeof(size_t) + sizeof(bool))];
} metadata_t;

uint8_t heap[HEAP_CAPACITY] __attribute__((aligned(ALIGN_MAX)));

void *heap_alloc(size_t size, alignment_t alignment);
void *heap_free(void *ptr);

#ifndef MEM_IMPLEMENTATION
#define MEM_IMPLEMENTATION

#define HEAP_END ((uint8_t *)(&(heap[HEAP_CAPACITY - 1])))

void *heap_alloc(size_t size, alignment_t alignment)
{
    uint8_t *current_ptr = (uint8_t *)heap;

    while (current_ptr != HEAP_END)
    {
        metadata_t *metadata = (metadata_t *)current_ptr;
        if (metadata->is_allocated)
        {
            current_ptr += (sizeof(metadata_t) + metadata->chunk_size);
            continue;
        }
    }
}

#endif /* MEM_IMPLEMENTATION */

#endif /* EFBB842C_C86D_4D43_862A_B356723F50A8 */
