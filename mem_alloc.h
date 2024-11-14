#ifndef EFBB842C_C86D_4D43_862A_B356723F50A8
#define EFBB842C_C86D_4D43_862A_B356723F50A8

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_ALIGNMENT (ALIGN_8)
#define MAX_ALIGNMENT (ALIGN_16)
#define HEAP_CAPACITY (65536)
#define HEAP_METADATA_IDENTIFIER 0xABCDEF01
#define DEBUG_PRINT 1 // set to 0 to disable debug prints

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
    uint32_t identifier;
    uint8_t _padding[ALIGN_MAX - (sizeof(size_t) + sizeof(bool) + sizeof(uint32_t))];
} metadata_t;

static uint8_t heap[HEAP_CAPACITY] __attribute__((aligned(ALIGN_MAX))) = {0};

void *heap_alloc(size_t size, alignment_t alignment);
void heap_free(void *ptr);

#ifndef MEM_IMPLEMENTATION
#define MEM_IMPLEMENTATION

#define HEAP_START ((uint8_t *)heap)
#define HEAP_END (HEAP_START + HEAP_CAPACITY)
#define NEXT_CHUNK(ptr) ((uint8_t *)(ptr) + sizeof(metadata_t) + ((metadata_t *)(ptr))->chunk_size)
#define CHUNK_DATA(ptr) ((uint8_t *)(ptr) + sizeof(metadata_t))

static inline size_t align_up(size_t n, size_t align);
static inline void *align_ptr(void *ptr, size_t align);
static inline bool is_chunk_allocated(const metadata_t *metadata);
static inline bool is_within_heap(const void *ptr);
static inline void heap_init(void);

static inline size_t align_up(size_t n, size_t align)
{
    return (n + align - 1) & ~(align - 1);
}

static inline void *align_ptr(void *ptr, size_t align)
{
    return (void *)align_up((uintptr_t)ptr, align);
}

static inline bool is_chunk_allocated(const metadata_t *metadata)
{
    return metadata->is_allocated;
}

static inline bool is_within_heap(const void *ptr)
{
    return ptr >= (void *)HEAP_START && ptr < (void *)HEAP_END;
}

static inline void heap_init(void)
{
    static bool has_run = false;
    if (has_run)
        return;
    has_run = true;

    metadata_t *initial_metadata = (metadata_t *)heap;
    initial_metadata->chunk_size = HEAP_CAPACITY - sizeof(metadata_t);
    initial_metadata->is_allocated = false;
    initial_metadata->identifier = HEAP_METADATA_IDENTIFIER;

    if (DEBUG_PRINT)
    {
        printf("Heap initialized:\n");
        printf("Heap start: %p\n", (void *)HEAP_START);
        printf("Initial metadata size: %zu\n", sizeof(metadata_t));
        printf("Initial chunk size: %zu\n", initial_metadata->chunk_size);
    }
}

static void create_free_chunk(metadata_t *chunk, size_t size)
{
    chunk->chunk_size = size;
    chunk->is_allocated = false;
    chunk->identifier = HEAP_METADATA_IDENTIFIER;
}

void *heap_alloc(size_t size, alignment_t alignment)
{
    if (size == 0 || alignment == 0 || size > HEAP_CAPACITY)
    {
        return NULL;
    }

    heap_init();

    if (DEBUG_PRINT)
    {
        printf("\nAllocation request:\n");
        printf("Requested size: %zu\n", size);
        printf("Requested alignment: %u\n", alignment);
    }

    metadata_t *current = (metadata_t *)HEAP_START;
    while (is_within_heap(current))
    {
        if (current->identifier != HEAP_METADATA_IDENTIFIER)
        {
            if (DEBUG_PRINT)
                printf("Invalid chunk identifier at %p\n", (void *)current);
            return NULL;
        }

        if (!current->is_allocated)
        {
            // Calculate aligned data address
            void *data_start = CHUNK_DATA(current);
            void *aligned_data = align_ptr(data_start, alignment);
            size_t padding = (uint8_t *)aligned_data - (uint8_t *)data_start;
            size_t total_size = size + padding;

            if (DEBUG_PRINT)
            {
                printf("Found free chunk at: %p\n", (void *)current);
                printf("Chunk size: %zu\n", current->chunk_size);
                printf("Data start: %p\n", data_start);
                printf("Aligned data: %p\n", aligned_data);
                printf("Padding needed: %zu\n", padding);
                printf("Total size needed: %zu\n", total_size);
            }

            if (current->chunk_size >= total_size)
            {
                size_t remaining = current->chunk_size - total_size;

                // If there's enough space for a new chunk (including metadata)
                if (remaining >= sizeof(metadata_t) + alignment)
                {
                    metadata_t *next_chunk = (metadata_t *)((uint8_t *)current + sizeof(metadata_t) + total_size);
                    create_free_chunk(next_chunk, remaining - sizeof(metadata_t));
                    current->chunk_size = total_size;

                    if (DEBUG_PRINT)
                    {
                        printf("Split chunk. New free chunk at: %p, size: %zu\n",
                               (void *)next_chunk, next_chunk->chunk_size);
                    }
                }

                current->is_allocated = true;

                if (DEBUG_PRINT)
                {
                    printf("Allocation successful. Returning: %p\n", aligned_data);
                }

                return aligned_data;
            }
        }

        current = (metadata_t *)NEXT_CHUNK(current);

        if (!is_within_heap(current))
        {
            if (DEBUG_PRINT)
                printf("Reached end of heap\n");
            break;
        }
    }

    if (DEBUG_PRINT)
        printf("No suitable chunk found\n");
    return NULL;
}

void heap_free(void *ptr)
{
    if (!ptr || !is_within_heap(ptr))
    {
        return;
    }

    // Find the metadata by searching backward from the pointer
    metadata_t *metadata = (metadata_t *)((uint8_t *)ptr - sizeof(metadata_t));
    while ((uint8_t *)metadata >= HEAP_START && metadata->identifier != HEAP_METADATA_IDENTIFIER)
    {
        metadata = (metadata_t *)((uint8_t *)metadata - 1);
    }

    if ((uint8_t *)metadata >= HEAP_START &&
        metadata->identifier == HEAP_METADATA_IDENTIFIER &&
        metadata->is_allocated)
    {
        metadata->is_allocated = false;

        if (DEBUG_PRINT)
        {
            printf("Freed chunk at: %p, size: %zu\n", (void *)metadata, metadata->chunk_size);
        }
    }
}

#endif /* MEM_IMPLEMENTATION */
#endif /* EFBB842C_C86D_4D43_862A_B356723F50A8 */