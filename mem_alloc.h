/**
 * @file mem_alloc.h
 * @brief A simple heap memory allocator with alignment support
 *
 * This heap allocator implements a basic memory management system with the following features:
 * - Configurable alignment (4, 8, or 16 bytes)
 * - Memory corruption detection via CRC32
 * - Automatic chunk coalescing
 * - Debug logging support
 *
 * @note This allocator is intended for educational purposes and embedded systems
 * where a custom memory allocator is needed.
 */

#define MEM_IMPLEMENTATION

#ifndef MEM_ALLOCATOR_H
#define MEM_ALLOCATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "crc32.h" // Make sure to include your CRC32 implementation

/* Configuration */
#define HEAP_CAPACITY (65536) // 64KB heap size
#define DEFAULT_ALIGNMENT (8) // Default alignment in bytes
#define MAX_ALIGNMENT (16)    // Maximum supported alignment
#define DEBUG_LOGGING (1)     // Set to 0 to disable debug prints

/**
 * @brief Supported memory alignment options
 */
typedef enum
{
    ALIGN_4 = 4,     /**< 4-byte alignment */
    ALIGN_8 = 8,     /**< 8-byte alignment */
    ALIGN_16 = 16,   /**< 16-byte alignment */
    ALIGN_32 = 32,   /**< 32-byte alignment */
    ALIGN_64 = 64,   /**< 64-byte alignment */
    ALIGN_128 = 128, /**< 128-byte alignment */
    ALIGN_256 = 256, /**< 256-byte alignment */
    ALIGN_512 = 512, /**< 512-byte alignment */
    NO_ALIGNMENT = 0,
} alignment_t;

/**
 * @brief Initialize the heap allocator
 * @return true if initialization was successful, false otherwise
 */
bool heap_init(void);

/**
 * @brief Allocate memory from the heap
 * @param size Size of the requested memory block in bytes
 * @param alignment Desired memory alignment (must be 4, 8, or 16)
 * @return Pointer to the allocated memory or NULL if allocation failed
 */
void *heap_alloc(size_t size, alignment_t alignment);

/**
 * @brief Free previously allocated memory
 * @param ptr Pointer to the memory block to free
 */
void heap_free(void *ptr);

/**
 * @brief Get the current heap statistics
 * @param total_size [out] Total heap size in bytes
 * @param used_size [out] Currently used heap size in bytes
 * @param free_size [out] Available heap size in bytes
 * @param largest_free_block [out] Size of the largest contiguous free block
 */
void heap_get_stats(size_t *total_size, size_t *used_size,
                    size_t *free_size, size_t *largest_free_block);

/**
 * @file mem_alloc.h
 * @brief Implementation of the heap memory allocator
 */

/* Internal structures */
typedef struct __attribute__((packed))
{
    size_t chunk_size; /**< Size of the data chunk (excluding metadata) */
    bool is_allocated; /**< Whether the chunk is currently allocated */
    uint8_t current_alignment;
    uint32_t crc32; /**< CRC32 checksum for corruption detection */
    uint8_t padding[MAX_ALIGNMENT - (sizeof(size_t) + sizeof(bool) + sizeof(uint32_t) + sizeof(uint8_t))];
} metadata_t;

/* Static assertions to verify metadata structure */
_Static_assert(sizeof(metadata_t) == MAX_ALIGNMENT, "Metadata size must match MAX_ALIGNMENT");

/* Internal state */
static uint8_t heap[HEAP_CAPACITY] __attribute__((aligned(MAX_ALIGNMENT))) = {0};
static bool is_initialized = false;

/* Macros for heap navigation */
#define HEAP_START ((uint8_t *)heap)
#define HEAP_END (HEAP_START + HEAP_CAPACITY)
#define NEXT_CHUNK(ptr) ((uint8_t *)(ptr) + sizeof(metadata_t) + ((metadata_t *)(ptr))->chunk_size)
#define CHUNK_DATA(ptr) ((uint8_t *)(ptr) + sizeof(metadata_t))

#ifdef MEM_IMPLEMENTATION

/* Internal helper functions */
static inline size_t align_up(size_t n, size_t align)
{
    return (n + align - 1) & ~(align - 1);
}

static inline alignment_t calculate_alignment(const void *ptr)
{
    uintptr_t chunk_data = (uintptr_t)CHUNK_DATA(ptr);

    if (!(chunk_data & (ALIGN_512 - 1)))
    {
        return ALIGN_512;
    }
    else if (!(chunk_data & (ALIGN_256 - 1)))
    {
        return ALIGN_256;
    }
    else if (!(chunk_data & (ALIGN_128 - 1)))
    {
        return ALIGN_128;
    }
    else if (!(chunk_data & (ALIGN_64 - 1)))
    {
        return ALIGN_64;
    }
    else if (!(chunk_data & (ALIGN_32 - 1)))
    {
        return ALIGN_32;
    }
    else if (!(chunk_data & (ALIGN_16 - 1)))
    {
        return ALIGN_16;
    }
    else if (!(chunk_data & (ALIGN_8 - 1)))
    {
        return ALIGN_8;
    }
    else if (!(chunk_data & (ALIGN_4 - 1)))
    {
        return ALIGN_4;
    }

    return NO_ALIGNMENT;
}

static inline void *align_ptr(void *ptr, size_t align)
{
    return (void *)align_up((uintptr_t)ptr, align);
}

static inline bool is_within_heap(const void *ptr)
{
    return ptr >= (void *)HEAP_START && ptr < (void *)HEAP_END;
}

static inline uint32_t calculate_chunk_crc(const void *ptr)
{
    // Only calculate CRC for size and allocation status and current_alignment
    return crc32((const uint8_t *)ptr, sizeof(size_t) + sizeof(bool) + sizeof(uint8_t));
}

static bool validate_chunk(const metadata_t *chunk)
{
    if (!chunk || !is_within_heap(chunk))
    {
        return false;
    }
    return calculate_chunk_crc(chunk) == chunk->crc32;
}

static metadata_t *find_previous_chunk(metadata_t *current)
{
    if (current == (metadata_t *)HEAP_START)
    {
        return NULL;
    }

    metadata_t *prev = (metadata_t *)HEAP_START;
    while (prev && NEXT_CHUNK(prev) != (uint8_t *)current)
    {
        if (!is_within_heap(NEXT_CHUNK(prev)))
        {
            return NULL;
        }
        prev = (metadata_t *)NEXT_CHUNK(prev);
    }
    return prev;
}

static void create_free_chunk(metadata_t *chunk, size_t size)
{
    chunk->chunk_size = size;
    chunk->is_allocated = false;
    chunk->current_alignment = calculate_alignment((const void *)chunk);
    chunk->crc32 = calculate_chunk_crc(chunk);
}

/* Implementation of public functions */
bool heap_init(void)
{
    if (is_initialized)
    {
        return true;
    }

    metadata_t *initial_metadata = (metadata_t *)heap;
    initial_metadata->chunk_size = HEAP_CAPACITY - sizeof(metadata_t);
    initial_metadata->is_allocated = false;
    initial_metadata->current_alignment = MAX_ALIGNMENT;
    initial_metadata->crc32 = calculate_chunk_crc(initial_metadata);

    if (DEBUG_LOGGING)
    {
        printf("Heap initialized:\n"
               "- Start address: %p\n"
               "- Total size: %d bytes\n"
               "- Metadata size: %zu bytes\n"
               "- Initial free chunk: %zu bytes\n",
               (void *)HEAP_START, HEAP_CAPACITY,
               sizeof(metadata_t), initial_metadata->chunk_size);
    }

    is_initialized = true;
    return true;
}

void *heap_alloc(size_t size, alignment_t alignment)
{
    if (size == 0 || alignment == 0 || size > HEAP_CAPACITY || !is_initialized)
    {
        return NULL;
    }

    if (alignment != ALIGN_4 && alignment != ALIGN_8 && alignment != ALIGN_16)
    {
        alignment = DEFAULT_ALIGNMENT;
    }

    metadata_t *current = (metadata_t *)HEAP_START;
    while (is_within_heap(current))
    {
        if (!validate_chunk(current))
        {
            if (DEBUG_LOGGING)
            {
                printf("Warning: Corrupted chunk detected at %p\n", (void *)current);
            }
            return NULL;
        }

        if (!current->is_allocated)
        {
            // Try to coalesce with previous free chunk
            metadata_t *prev = find_previous_chunk(current);
            if (prev && !prev->is_allocated)
            {
                prev->chunk_size += sizeof(metadata_t) + current->chunk_size;
                prev->crc32 = calculate_chunk_crc(prev);
                current = prev;
            }

            size_t padding;
            size_t total_size;
            void *aligned_data;
            void *data_start = CHUNK_DATA(current);

            if (current->current_alignment >= alignment)
            {
                padding = 0;
                total_size = size + padding;
                aligned_data = data_start;
            }
            else
            {
                // Calculate alignment requirements
                aligned_data = align_ptr(data_start, alignment);
                padding = (uint8_t *)aligned_data - (uint8_t *)data_start;
                total_size = size + padding;
            }

            if (current->chunk_size >= total_size)
            {
                size_t remaining = current->chunk_size - total_size;

                // Split chunk if there's enough space for a new chunk
                if (remaining >= sizeof(metadata_t) + alignment)
                {
                    metadata_t *next_chunk = (metadata_t *)((uint8_t *)current +
                                                            sizeof(metadata_t) + total_size);
                    create_free_chunk(next_chunk, remaining - sizeof(metadata_t));
                    current->chunk_size = total_size;
                }

                current->is_allocated = true;
                current->current_alignment = alignment;
                current->crc32 = calculate_chunk_crc(current);

                if (DEBUG_LOGGING)
                {
                    printf("Allocated %zu bytes at %p (aligned to %d)\n",
                           size, aligned_data, alignment);
                }

                return aligned_data;
            }
        }

        current = (metadata_t *)NEXT_CHUNK(current);
    }

    if (DEBUG_LOGGING)
    {
        printf("Allocation failed: No suitable chunk found for %zu bytes\n", size);
    }
    return NULL;
}

void heap_get_stats(size_t *total_size, size_t *used_size,
                    size_t *free_size, size_t *largest_free_block)
{
    *total_size = HEAP_CAPACITY;
    *used_size = 0;
    *free_size = 0;
    *largest_free_block = 0;

    metadata_t *current = (metadata_t *)HEAP_START;
    while (is_within_heap(current))
    {
        if (!validate_chunk(current))
        {
            break;
        }

        if (current->is_allocated)
        {
            *used_size += current->chunk_size;
        }
        else
        {
            *free_size += current->chunk_size;
            if (current->chunk_size > *largest_free_block)
            {
                *largest_free_block = current->chunk_size;
            }
        }

        current = (metadata_t *)NEXT_CHUNK(current);
    }
}

void heap_free(void *ptr)
{
    if (!ptr || !is_within_heap(ptr) || !is_initialized)
    {
        return;
    }

    // Search backward for metadata since we don't know the exact padding
    metadata_t *metadata = (metadata_t *)((uint8_t *)ptr - sizeof(metadata_t));
    while ((uint8_t *)metadata >= HEAP_START)
    {
        if (validate_chunk(metadata) /* && (void *)align_ptr(metadata, metadata->current_alignment) == ptr */)
        {
            if (metadata->is_allocated)
            {
                // Found the correct chunk
                if (DEBUG_LOGGING)
                {
                    printf("Freeing chunk at %p (data at %p, size: %zu)\n",
                           (void *)metadata, ptr, metadata->chunk_size);
                }

                metadata->is_allocated = false;
                metadata->current_alignment = calculate_alignment((const void *)metadata);
                metadata->crc32 = calculate_chunk_crc(metadata);

                // Coalesce with next chunk if it's free
                metadata_t *next = (metadata_t *)NEXT_CHUNK(metadata);
                if (is_within_heap(next) && validate_chunk(next) && !next->is_allocated)
                {
                    metadata->chunk_size += sizeof(metadata_t) + next->chunk_size;
                    metadata->crc32 = calculate_chunk_crc(metadata);

                    if (DEBUG_LOGGING)
                    {
                        printf("Coalesced with next chunk, new size: %zu\n", metadata->chunk_size);
                    }
                }
                return;
            }
        }
        metadata = (metadata_t *)((uint8_t *)metadata - 1);
    }

    if (DEBUG_LOGGING)
    {
        printf("Warning: Could not find valid metadata for pointer %p\n", ptr);
    }
}

#endif // MEM_IMPLEMENTATION
#endif // HEAP_ALLOCATOR_H
