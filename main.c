#define MEM_IMPLEMENTATION
#include "mem_alloc.h"

#include <stdio.h>
#include <assert.h>

// Include your heap allocator header here

int main()
{
    // Test basic allocation
    void *ptr1 = heap_alloc(100, ALIGN_8);
    printf("\nFirst allocation (100 bytes): %p\n", ptr1);

    if (ptr1 != NULL)
    {
        // Test that returned pointer is properly aligned
        printf("Alignment check: %s\n",
               (((uintptr_t)ptr1 & (ALIGN_8 - 1)) == 0) ? "PASSED" : "FAILED");

        // Test second allocation
        void *ptr2 = heap_alloc(200, ALIGN_8);
        printf("Second allocation (200 bytes): %p\n", ptr2);

        if (ptr2 != NULL)
        {
            printf("Alignment check: %s\n",
                   (((uintptr_t)ptr2 & (ALIGN_8 - 1)) == 0) ? "PASSED" : "FAILED");
        }

        // Free the allocations
        heap_free(ptr1);
        heap_free(ptr2);

        // Try reallocating to ensure free worked
        void *ptr3 = heap_alloc(100, ALIGN_8);
        printf("Reallocation test: %s\n",
               (ptr3 != NULL) ? "PASSED" : "FAILED");
        heap_free(ptr3);
    }
    else
    {
        printf("Initial allocation failed!\n");
        printf("Heap capacity: %d\n", HEAP_CAPACITY);
        metadata_t *initial_meta = (metadata_t *)heap;
        printf("Initial chunk size: %zu\n", initial_meta->chunk_size);
        printf("Is allocated: %d\n", initial_meta->is_allocated);
        printf("Identifier: 0x%x\n", initial_meta->identifier);
    }

    return 0;
}

/* void *heap_alloc(size_t size, alignment_t alignment)
{
    if (size == 0 || size > HEAP_CAPACITY || alignment == 0)
    {
        return NULL;
    }

    heap_init();

    uint8_t *current_chunk = HEAP_START;
    uint8_t *previous_chunk = NULL;

    // Loop through chunks to find a suitable one
    while (current_chunk < HEAP_END && current_chunk + sizeof(metadata_t) <= HEAP_END)
    {
        metadata_t *current_metadata = (metadata_t *)current_chunk;

        printf("Checking chunk - Address: %p, Size: %zu, Allocated: %d\n",
               current_chunk, current_metadata->chunk_size, current_metadata->is_allocated);

        // Skip allocated chunks
        if (current_metadata->is_allocated)
        {
            current_chunk = (uint8_t *)current_chunk + sizeof(metadata_t) + current_metadata->chunk_size;
            continue;
        }

        // Calculate alignment requirements
        void *unaligned_data = CHUNK_DATA(current_chunk);
        void *aligned_data = ALIGNED_CHUNK_DATA(current_chunk, alignment);
        size_t padding = (uint8_t *)aligned_data - (uint8_t *)unaligned_data;
        size_t total_size = size + padding;

        // Check if chunk is big enough
        if (current_metadata->chunk_size >= total_size)
        {
            // If remaining space would be too small for a new chunk, use entire chunk
            if (current_metadata->chunk_size - total_size <= sizeof(metadata_t))
            {
                current_metadata->is_allocated = true;
                return aligned_data;
            }

            // Split the chunk
            size_t new_chunk_size = current_metadata->chunk_size - total_size - sizeof(metadata_t);
            current_metadata->chunk_size = total_size;
            current_metadata->is_allocated = true;

            // Setup the new chunk
            metadata_t *new_metadata = (metadata_t *)((uint8_t *)current_chunk + sizeof(metadata_t) + total_size);
            new_metadata->chunk_size = new_chunk_size;
            new_metadata->is_allocated = false;
            new_metadata->identifier = HEAP_METADATA_IDENTIFIER;

            return aligned_data;
        }

        current_chunk = (uint8_t *)current_chunk + sizeof(metadata_t) + current_metadata->chunk_size;
    }

    return NULL; // No suitable chunk found
} */