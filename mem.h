#ifndef D46AFE7A_7823_4C7A_A759_A5737B4A74D1
#define D46AFE7A_7823_4C7A_A759_A5737B4A74D1

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define HEAP_CAPACITY (65536)
#define FREE_CAPACITY (1024)

#define MAX_ALIGNMENT (ALIGN_64)
#define MAX_ALIGNMENT_INT (64)
#define DEFAULT_ALIGNMENT (ALIGN_8)

typedef enum
{
    ALIGN_1 = 1,
    ALIGN_2 = 2,
    ALIGN_4 = 4,
    ALIGN_8 = 8,
    ALIGN_16 = 16,
    ALIGN_32 = 32,
    ALIGN_64 = 64,
} alignment_t;

typedef struct
{
    void *chunk_ptr;
    void *prev_chunk_ptr;
    alignment_t current_alignment;
    size_t size;
} metadata_t;

metadata_t free_array[FREE_CAPACITY] = {0};
size_t free_array_size = 0;

uint8_t heap[HEAP_CAPACITY] __attribute__((aligned(MAX_ALIGNMENT_INT))) = {0};

// worst case O(n)
static ssize_t search_by_ptr_in_free_array(void *ptr)
{
    return -1;
}

// worst case O(log(n))
static ssize_t search_by_size_in_free_array(size_t size, alignment_t alignment)
{
    size_t left = 0;
    size_t right = free_array_size - 1;
    size_t mid;
    size_t result = -1;

    while (left <= right)
    {
        mid = (left + right) / 2;
        size_t mid_size = free_array[mid].size;

        if (mid_size >= size)
        {
            result = mid;
            right = mid - 1;
        }
        else
        {
            left = mid + 1;
        }
    }

    if (result == -1)
    {
        return -1;
    }

    for (size_t index = (size_t)result; index < free_array_size; index++)
    {
        metadata_t *current_chunk = &(free_array[index]);
        alignment_t current_alignment = current_chunk->current_alignment;

        if (current_alignment >= alignment)
        {
            return index;
        }

        size_t required_padding = ((alignment - (size_t)current_chunk->chunk_ptr) & (alignment - 1));

        if (current_chunk->size >= required_padding + size)
        {
            return index;
        }
    }

    return -1;
}

static inline alignment_t calculate_alignment(const void *ptr)
{
    uintptr_t chunk_data = (uintptr_t)ptr;
    for (uint8_t align = ALIGN_64; align >= ALIGN_1; align >>= 1)
    {
        if (!(chunk_data & (align - 1)))
        {
            return (alignment_t)align;
        }
    }

    return ALIGN_1;
}

// worst case O(n)
static bool remove_from_free_array(size_t elt_index)
{
    return false;
}

// worst case O(n)
static bool add_into_free_array(void *chunk_ptr, void *prev_chunk_ptr, size_t size)
{
    return false;
}

void defragment_heap()
{
    for (size_t i = 0; i < free_array_size; i++)
    {
        metadata_t *current_chunk = &(free_array[i]);
        ssize_t prev_chunk_index = search_by_ptr_in_free_array(current_chunk->prev_chunk_ptr);
        if (prev_chunk_index >= 0)
        {
            metadata_t *prev_chunk = &(free_array[prev_chunk_index]);
            prev_chunk->size += current_chunk->size;
            remove_from_free_array(i);
        }
    }
}

void heap_init()
{
    free_array[free_array_size++] = (metadata_t){.chunk_ptr = heap, .prev_chunk_ptr = NULL, .current_alignment = MAX_ALIGNMENT, .size = HEAP_CAPACITY};
}

void *heap_alloc(size_t size, alignment_t alignment)
{
    if (!size)
    {
        return NULL;
    }

    if (((alignment) & (alignment - 1)) || (alignment > ALIGN_64))
    {
        alignment = DEFAULT_ALIGNMENT;
    }

    size_t best_fit_index = search_by_size_in_free_array(size, alignment);

    return NULL;
}

#endif /* D46AFE7A_7823_4C7A_A759_A5737B4A74D1 */
