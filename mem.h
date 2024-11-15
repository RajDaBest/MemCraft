#ifndef D46AFE7A_7823_4C7A_A759_A5737B4A74D1
#define D46AFE7A_7823_4C7A_A759_A5737B4A74D1

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define HEAP_CAPACITY (65536)
#define FREE_CAPACITY (1024)
#define ALLOC_CAPACITY (1024)

#define MAX_ALIGNMENT (ALIGN_64)
#define MAX_ALIGNMENT_INT (64)
#define DEFAULT_ALIGNMENT (ALIGN_8)

#define SPLIT_CUTOFF (16)

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
    void *chunk_ptr; // Start of the chunk including padding
    void *data_ptr;  // Aligned pointer to usable memory
    void *prev_chunk_ptr;
    size_t size;        // Total size including padding
    size_t usable_size; // Size available to user
    alignment_t current_alignment;
} metadata_t;

metadata_t free_array[FREE_CAPACITY] = {0};
metadata_t alloc_array[ALLOC_CAPACITY] = {0};
size_t free_array_size = 0;
size_t alloc_array_size = 0;

uint8_t heap[HEAP_CAPACITY] __attribute__((aligned(MAX_ALIGNMENT_INT))) = {0};

static ssize_t search_by_ptr(void *ptr, metadata_t *array, size_t array_size)
{
    size_t left = 0;
    size_t right = array_size;

    while (left < right)
    {
        size_t mid = (left + right) / 2;
        if (array[mid].data_ptr == ptr)
        {
            return mid;
        }
        if (array[mid].data_ptr < ptr)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }
    return -1;
}

static ssize_t search_by_ptr_in_free_array(void *ptr)
{
    return search_by_ptr(ptr, free_array, free_array_size);
}

static ssize_t search_by_ptr_in_alloc_array(void *ptr)
{
    return search_by_ptr(ptr, alloc_array, alloc_array_size);
}

static ssize_t search_by_size_in_free_array(size_t size, alignment_t alignment)
{
    size_t left = 0;
    size_t right = free_array_size;
    ssize_t best_fit = -1;
    size_t smallest_sufficient_size = SIZE_MAX;

    while (left < right)
    {
        size_t mid = (left + right) / 2;
        metadata_t *current = &free_array[mid];

        size_t padding = ((alignment - (size_t)current->chunk_ptr) & (alignment - 1));
        size_t total_required = size + padding;

        if (current->size >= total_required)
        {
            if (current->size < smallest_sufficient_size)
            {
                smallest_sufficient_size = current->size;
                best_fit = mid;
            }
            right = mid;
        }
        else
        {
            left = mid + 1;
        }
    }

    return best_fit;
}

static inline alignment_t calculate_alignment(const void *ptr)
{
    uintptr_t addr = (uintptr_t)ptr;
    size_t alignment = ALIGN_1;

    while (alignment <= MAX_ALIGNMENT && !(addr & (alignment - 1)))
    {
        alignment = (alignment << 1);
    }

    return (alignment_t)(alignment >> 1);
}

static bool remove_from_array(size_t index, metadata_t *array, size_t *array_size)
{
    if (index >= *array_size)
    {
        return false;
    }

    memmove(&array[index], &array[index + 1],
            (*array_size - index - 1) * sizeof(metadata_t));
    (*array_size)--;
    return true;
}

static bool remove_from_free_array(size_t index)
{
    return remove_from_array(index, free_array, &free_array_size);
}

static bool remove_from_alloc_array(size_t index)
{
    return remove_from_array(index, alloc_array, &alloc_array_size);
}

static size_t find_insertion_position(void *data_ptr, metadata_t *array, size_t array_size)
{
    size_t left = 0;
    size_t right = array_size;

    while (left < right)
    {
        size_t mid = (left + right) / 2;
        if (array[mid].data_ptr <= data_ptr)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }
    return left;
}

static bool add_into_array(metadata_t chunk, metadata_t *array, size_t *array_size, size_t capacity)
{
    if (*array_size >= capacity)
    {
        return false;
    }

    size_t pos = find_insertion_position(chunk.data_ptr, array, *array_size);

    if (pos < *array_size)
    {
        memmove(&array[pos + 1], &array[pos],
                (*array_size - pos) * sizeof(metadata_t));
    }

    array[pos] = chunk;
    (*array_size)++;
    return true;
}

static bool add_into_free_array(void *chunk_ptr, void *data_ptr, void *prev_chunk_ptr,
                                size_t size, size_t usable_size, alignment_t alignment)
{
    metadata_t chunk = {
        .chunk_ptr = chunk_ptr,
        .data_ptr = data_ptr,
        .prev_chunk_ptr = prev_chunk_ptr,
        .size = size,
        .usable_size = usable_size,
        .current_alignment = alignment};
    return add_into_array(chunk, free_array, &free_array_size, FREE_CAPACITY);
}

static bool add_into_alloc_array(void *chunk_ptr, void *data_ptr, void *prev_chunk_ptr,
                                 size_t size, size_t usable_size, alignment_t alignment)
{
    metadata_t chunk = {
        .chunk_ptr = chunk_ptr,
        .data_ptr = data_ptr,
        .prev_chunk_ptr = prev_chunk_ptr,
        .size = size,
        .usable_size = usable_size,
        .current_alignment = alignment};
    return add_into_array(chunk, alloc_array, &alloc_array_size, ALLOC_CAPACITY);
}

void defragment_heap()
{
    bool defragmented;
    do
    {
        defragmented = false;
        for (size_t i = 0; i < free_array_size; i++)
        {
            metadata_t *current = &free_array[i];

            void *next_chunk = (uint8_t *)current->chunk_ptr + current->size;
            for (size_t j = 0; j < free_array_size; j++)
            {
                if (i != j && free_array[j].chunk_ptr == next_chunk)
                {
                    current->size += free_array[j].size;
                    remove_from_free_array(j);
                    defragmented = true;
                    break;
                }
            }

            ssize_t prev_idx = search_by_ptr_in_free_array(current->prev_chunk_ptr);
            if (prev_idx >= 0)
            {
                metadata_t *prev = &free_array[prev_idx];
                prev->size += current->size;
                remove_from_free_array(i);
                defragmented = true;
                break;
            }
        }
    } while (defragmented);
}

void heap_init()
{
    static bool has_run = false;
    if (has_run)
    {
        return;
    }

    has_run = true;
    free_array_size = 0;
    alloc_array_size = 0;
    add_into_free_array(heap, heap, NULL, HEAP_CAPACITY, HEAP_CAPACITY, MAX_ALIGNMENT);
}

void *heap_alloc(size_t size, alignment_t alignment)
{
    if (!size)
    {
        return NULL;
    }

    heap_init();

    if (((alignment) & (alignment - 1)) || (alignment > MAX_ALIGNMENT))
    {
        alignment = DEFAULT_ALIGNMENT;
    }

    ssize_t best_fit_index = search_by_size_in_free_array(size, alignment);
    if (best_fit_index < 0)
    {
        return NULL;
    }

    metadata_t *chunk = &free_array[best_fit_index];
    size_t padding = ((alignment - (size_t)chunk->chunk_ptr) & (alignment - 1));
    void *data_ptr = (uint8_t *)chunk->chunk_ptr + padding;

    if (padding >= SPLIT_CUTOFF)
    {
        add_into_free_array(chunk->chunk_ptr, chunk->chunk_ptr,
                            chunk->prev_chunk_ptr, padding, padding,
                            calculate_alignment(chunk->chunk_ptr));

        chunk->chunk_ptr = (uint8_t *)chunk->chunk_ptr + padding;
        chunk->size -= padding;
        chunk->prev_chunk_ptr = (uint8_t *)chunk->chunk_ptr - padding;
    }

    size_t remaining = chunk->size - size;
    if (remaining >= SPLIT_CUTOFF)
    {
        void *new_chunk_ptr = (uint8_t *)chunk->chunk_ptr + size;
        add_into_free_array(new_chunk_ptr, new_chunk_ptr,
                            chunk->chunk_ptr, remaining, remaining,
                            calculate_alignment(new_chunk_ptr));
        chunk->size = size;
    }

    add_into_alloc_array(chunk->chunk_ptr, data_ptr,
                         chunk->prev_chunk_ptr, chunk->size,
                         chunk->size - padding, alignment);
    remove_from_free_array(best_fit_index);

    return data_ptr;
}

void heap_free(void *ptr)
{
    if (!ptr)
    {
        return;
    }

    ssize_t alloc_index = search_by_ptr_in_alloc_array(ptr);
    if (alloc_index < 0)
    {
        return;
    }

    metadata_t *chunk = &alloc_array[alloc_index];
    add_into_free_array(chunk->chunk_ptr, chunk->data_ptr,
                        chunk->prev_chunk_ptr, chunk->size,
                        chunk->usable_size, chunk->current_alignment);
    remove_from_alloc_array(alloc_index);

    defragment_heap();
}

#endif /* D46AFE7A_7823_4C7A_A759_A5737B4A74D1 */