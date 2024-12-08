#ifndef D46AFE7A_7823_4C7A_A759_A5737B4A74D1
#define D46AFE7A_7823_4C7A_A759_A5737B4A74D1

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define HEAP_CAPACITY (65536)
#define FREE_CAPACITY (1024)
#define ALLOC_CAPACITY (1024)

#define MAX_ALIGNMENT (ALIGN_64)
#define MAX_ALIGNMENT_INT (64)
#define DEFAULT_ALIGNMENT ((alignment_t)(sizeof(void *))) // by default align on pointer size, this is enough for most platforms and architectures

#define SPLIT_CUTOFF (16)

#define FREE_DEFRAG_CUTOFF (32) // must be a power of 2

#define THREAD_LOCAL __thread

#define BIN_8_SIZE (8)
#define BIN_16_SIZE (16)
#define BIN_32_SIZE (32)
#define BIN_8_CAPACITY (1024)
#define BIN_16_CAPACITY (512)
#define BIN_32_CAPACITY (256)

typedef enum
{
    ALLOC_TYPE_HEAP,
    ALLOC_TYPE_BIN_8,
    ALLOC_TYPE_BIN_16,
    ALLOC_TYPE_BIN_32
} allocation_type_t;

typedef enum
{
    ALIGN_1 = 1,
    ALIGN_2 = 2,
    ALIGN_4 = 4,
    ALIGN_8 = 8,
    ALIGN_16 = 16,
    ALIGN_32 = 32,
    ALIGN_64 = 64,
    ALIGN_MAX = ALIGN_64,
    ALIGN_DEFAULT = ALIGN_8,
    ALIGN_SAME = 0,
} alignment_t;

typedef struct
{
    void *chunk_ptr;
    void *data_ptr;
    void *prev_chunk_ptr;
    size_t size;
    size_t usable_size;
    alignment_t current_alignment;
    allocation_type_t alloc_type;
    bool mark;
} metadata_t;

static THREAD_LOCAL metadata_t free_array[FREE_CAPACITY] = {0};
static THREAD_LOCAL metadata_t alloc_array[ALLOC_CAPACITY] = {0};
static THREAD_LOCAL size_t free_array_size = 0;
static THREAD_LOCAL size_t alloc_array_size = 0;

static THREAD_LOCAL uint8_t heap[HEAP_CAPACITY] __attribute__((aligned(MAX_ALIGNMENT_INT))) = {0};

static THREAD_LOCAL uint8_t bin_8[BIN_8_CAPACITY * BIN_8_SIZE] __attribute__((aligned(MAX_ALIGNMENT_INT))) = {0};
static THREAD_LOCAL uint8_t bin_16[BIN_16_CAPACITY * BIN_16_SIZE] __attribute__((aligned(MAX_ALIGNMENT_INT))) = {0};
static THREAD_LOCAL uint8_t bin_32[BIN_32_CAPACITY * BIN_32_SIZE] __attribute__((aligned(MAX_ALIGNMENT_INT))) = {0};

static THREAD_LOCAL metadata_t free_bin_8[BIN_8_CAPACITY] = {0};
static THREAD_LOCAL metadata_t alloc_bin_8[BIN_8_CAPACITY] = {0};
static THREAD_LOCAL metadata_t free_bin_16[BIN_16_CAPACITY] = {0};
static THREAD_LOCAL metadata_t alloc_bin_16[BIN_16_CAPACITY] = {0};
static THREAD_LOCAL metadata_t free_bin_32[BIN_32_CAPACITY] = {0};
static THREAD_LOCAL metadata_t alloc_bin_32[BIN_32_CAPACITY] = {0};

static THREAD_LOCAL size_t free_bin_8_size = 0;
static THREAD_LOCAL size_t alloc_bin_8_size = 0;
static THREAD_LOCAL size_t free_bin_16_size = 0;
static THREAD_LOCAL size_t alloc_bin_16_size = 0;
static THREAD_LOCAL size_t free_bin_32_size = 0;
static THREAD_LOCAL size_t alloc_bin_32_size = 0;

static THREAD_LOCAL size_t num_of_free_called_on_heap = 0;

void *heap_alloc(size_t size, alignment_t alignment);
void heap_free(void *ptr);
void heap_init();
void *heap_realloc(void *ptr, size_t new_size, alignment_t new_alignment);

#define MEM_IMPLEMENTATION

#ifdef MEM_IMPLEMENTATION

static void init_bins();
static ssize_t search_by_ptr(void *ptr, metadata_t *array, size_t array_size);
static ssize_t search_by_ptr_in_free_array(void *ptr);
static ssize_t search_by_ptr_in_alloc_array(void *ptr);
static ssize_t search_by_size_in_free_array(size_t size, alignment_t alignment);
static inline alignment_t calculate_alignment(const void *ptr);
static bool remove_from_array(size_t index, metadata_t *array, size_t *array_size);
static bool remove_from_free_array(size_t index);
static bool remove_from_alloc_array(size_t index);
static size_t find_insertion_position(void *data_ptr, metadata_t *array, size_t array_size);
static bool add_into_array(metadata_t chunk, metadata_t *array, size_t *array_size, size_t capacity);
static bool add_into_free_array(void *chunk_ptr, void *data_ptr, void *prev_chunk_ptr,
                                size_t size, size_t usable_size, alignment_t alignment);
static bool add_into_alloc_array(void *chunk_ptr, void *data_ptr, void *prev_chunk_ptr,
                                 size_t size, size_t usable_size, alignment_t alignment);
static void defragment_heap();

#ifdef GC_COLLECT

/* I need to scan the stack of all the threads since a thread may reference a heap pointer of some other thread since the allocator uses thread-local storage */

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_PATH_LENGTH 256
#define THREAD_SCAN_LIMIT 1024
#define MAX_FILE_LENGTH 16000
#define LINE_BUFFER_LENGTH 1024

extern void *__data_start;
extern void *__bss_start;
extern void *_edata; // end of data section
extern void *_end;   // end of bss section

typedef struct
{
    pid_t thread_id;
    void *stack_base;
    void *stack_end;
} thread_info_t;

static thread_info_t thread_info_array[THREAD_SCAN_LIMIT];
static size_t thread_count = 0;

static void collect_on_personal_heap();
static void collect_static();
static void handle_error(const char *message);
static int parse_stack_addresses(const char *line, void **stack_base, void **stack_end);
static int get_thread_stack_addresses(pid_t thread_id, void **stack_base, void **stack_end);
static void collect_thread_info();
static void get_libc_heap_range(void **start, void **end);
static void mark_if_points_to_heap(uintptr_t potential_pointer);
static void collect_on_personal_heap();
static void collect_static();
static void collect_thread_stack();
static void collect_libc_heap();
static void sweep_unmarked();
static void garbage_collect();

void collect_on_personal_heap()
{
    uintptr_t heap_start = (uintptr_t)heap;
    uintptr_t heap_end = heap_start + (uintptr_t)HEAP_CAPACITY;

    for (size_t i = 0; i < alloc_array_size; i++)
    {
        metadata_t current_metadata = alloc_array[i];

        if (current_metadata.usable_size <= sizeof(void *))
        {
            continue;
        }

        uintptr_t block_start = (uintptr_t)current_metadata.data_ptr;
        uintptr_t block_end = block_start + current_metadata.usable_size;

        /* The thing is, if the compiler is putting any address in memory, it will always align it on it's natural boundary, i.e, sizeof (void *),
        but in our implementation we can use any aligned memory location to put pointers there as data, so we need to scan the heap byte by byte, while
        in compiler heap, stacks, static and BSS section we can scan in multiples of 8 (the start of these sections are at least aligned on sizeof (void *)) */

        for (uintptr_t address = block_start;
             address <= block_end - sizeof(void *);
             address++)
        {
            uintptr_t potential_pointer = *(uintptr_t *)address;

            if (potential_pointer >= heap_start && potential_pointer < heap_end)
            {
                printf("Found pointer %zu in heap\n", (uintptr_t)potential_pointer);
            }
        }
    }
}

void collect_static()
{
    size_t data_section_size = (size_t)_edata - (size_t)__data_start;
    size_t bss_section_size = (size_t)_end - (size_t)__bss_start;

    if (data_section_size < 8 || bss_section_size < 8)
    {
        return;
    }

    for (uintptr_t current_ptr = (uintptr_t)__data_start; current_ptr <= _edata - sizeof(void *); current_ptr += 8)
    {
        uintptr_t may_be_ptr = *(uint64_t *)current_ptr;

        check_if_allocated(may_be_ptr);
    }

    for (uintptr_t current_ptr = (uintptr_t)__bss_start; current_ptr <= _end - sizeof(void *); current_ptr += 8)
    {
        uintptr_t may_be_ptr = *(uint64_t *)current_ptr;

        check_if_allocated(may_be_ptr);
    }
}

void handle_error(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

int parse_stack_addresses(const char *line, void **stack_base, void **stack_end)
{
    char *endptr;
    *stack_base = (void *)strtoull(line, &endptr, 16);

    if (*endptr != '-')
    {
        fprintf(stderr, "Invalid address format\n");
        return 0;
    }

    *stack_end = (void *)strtoull(endptr + 1, &endptr, 16);

    if (*stack_base == NULL || *stack_end == NULL || *stack_base >= *stack_end)
    {
        fprintf(stderr, "Invalid stack address range\n");
        return 0;
    }

    return 1;
}

int get_thread_stack_addresses(pid_t thread_id, void **stack_base, void **stack_end)
{
    char maps_path[MAX_PATH_LENGTH];
    char line_buffer[LINE_BUFFER_LENGTH];

    snprintf(maps_path, sizeof(maps_path), "/proc/self/task/%d/maps", thread_id);

    FILE *maps_file = fopen(maps_path, "r");
    if (!maps_file)
    {
        fprintf(stderr, "Failed to open maps file for thread %d\n", thread_id);
        return 0;
    }

    while (fgets(line_buffer, sizeof(line_buffer), maps_file))
    {
        if (strstr(line_buffer, "[stack]"))
        {
            line_buffer[strcspn(line_buffer, "\n")] = 0;

            if (parse_stack_addresses(line_buffer, stack_base, stack_end))
            {
                fclose(maps_file);
                return 1;
            }
        }
    }

    fclose(maps_file);
    return 0;
}

void collect_thread_info()
{
    DIR *task_dir = opendir("/proc/self/task");
    if (!task_dir)
    {
        handle_error("Failed to open /proc/self/task");
    }

    struct dirent *entry;
    thread_count = 0;

    while ((entry = readdir(task_dir)) != NULL && thread_count < THREAD_SCAN_LIMIT)
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }

        pid_t thread_id = atoi(entry->d_name);
        void *stack_base = NULL, *stack_end = NULL;

        if (get_thread_stack_addresses(thread_id, &stack_base, &stack_end))
        {
            thread_info_array[thread_count].thread_id = thread_id;
            thread_info_array[thread_count].stack_base = stack_base;
            thread_info_array[thread_count].stack_end = stack_end;

            printf("Thread %d: Stack Base at %p, Stack End at %p (Size: %zu bytes)\n",
                   thread_id, stack_base, stack_end, (char *)stack_end - (char *)stack_base);
            thread_count++;
        }
    }

    closedir(task_dir);
}

static void get_libc_heap_range(void **start, void **end)
{
    char line[LINE_BUFFER_LENGTH];
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps)
    {
        handle_error("Failed to open /proc/self/maps");
    }

    while (fgets(line, sizeof(line), maps))
    {
        if (strstr(line, "[heap]"))
        {
            void *heap_start, *heap_end;
            if (parse_stack_addresses(line, &heap_start, &heap_end))
            {
                *start = heap_start;
                *end = heap_end;
                fclose(maps);
                return;
            }
        }
    }

    fclose(maps);
    *start = NULL;
    *end = NULL;
}

static void mark_if_points_to_heap(uintptr_t potential_pointer)
{
    for (size_t i = 0; i < alloc_array_size; i++)
    {
        metadata_t *chunk = &alloc_array[i];
        uintptr_t chunk_start = (uintptr_t)chunk->data_ptr;
        uintptr_t chunk_end = chunk_start + chunk->usable_size;

        if (potential_pointer >= chunk_start && potential_pointer < chunk_end)
        {
            chunk->mark = true;
            break;
        }
    }
}

void collect_on_personal_heap()
{
    for (size_t i = 0; i < alloc_array_size; i++)
    {
        alloc_array[i].mark = false;
    }

    uintptr_t heap_start = (uintptr_t)heap;
    uintptr_t heap_end = heap_start + (uintptr_t)HEAP_CAPACITY;

    for (size_t i = 0; i < alloc_array_size; i++)
    {
        metadata_t current = alloc_array[i];
        if (current.usable_size <= sizeof(void *))
            continue;

        for (uintptr_t addr = (uintptr_t)current.data_ptr;
             addr <= (uintptr_t)current.data_ptr + current.usable_size - sizeof(void *);
             addr += sizeof(void *))
        {
            mark_if_points_to_heap(*(uintptr_t *)addr);
        }
    }
}

void collect_static()
{
    for (uintptr_t addr = (uintptr_t)__data_start;
         addr <= (uintptr_t)_edata - sizeof(void *);
         addr += sizeof(void *))
    {
        mark_if_points_to_heap(*(uintptr_t *)addr);
    }

    for (uintptr_t addr = (uintptr_t)__bss_start;
         addr <= (uintptr_t)_end - sizeof(void *);
         addr += sizeof(void *))
    {
        mark_if_points_to_heap(*(uintptr_t *)addr);
    }
}

void collect_thread_stack()
{
    collect_thread_info();

    for (size_t i = 0; i < thread_count; i++)
    {
        for (uintptr_t addr = (uintptr_t)thread_info_array[i].stack_base;
             addr <= (uintptr_t)thread_info_array[i].stack_end - sizeof(void *);
             addr += sizeof(void *))
        {
            mark_if_points_to_heap(*(uintptr_t *)addr);
        }
    }
}

void collect_libc_heap()
{
    void *libc_heap_start, *libc_heap_end;
    get_libc_heap_range(&libc_heap_start, &libc_heap_end);

    if (!libc_heap_start || !libc_heap_end)
        return;

    for (uintptr_t addr = (uintptr_t)libc_heap_start;
         addr <= (uintptr_t)libc_heap_end - sizeof(void *);
         addr += sizeof(void *))
    {
        mark_if_points_to_heap(*(uintptr_t *)addr);
    }
}

void sweep_unmarked()
{
    for (ssize_t i = alloc_array_size - 1; i >= 0; i--)
    {
        if (!alloc_array[i].mark)
        {
            add_into_free_array(
                alloc_array[i].chunk_ptr,
                alloc_array[i].data_ptr,
                alloc_array[i].prev_chunk_ptr,
                alloc_array[i].size,
                alloc_array[i].usable_size,
                alloc_array[i].current_alignment);
            remove_from_alloc_array(i);
        }
    }
    defragment_heap();
}

void garbage_collect()
{
    collect_on_personal_heap();
    collect_static();
    collect_thread_stack();
    collect_libc_heap();

    sweep_unmarked();
}

#undef MAX_PATH_LENGTH
#undef THREAD_SCAN_LIMIT
#undef MAX_FILE_LENGTH
#undef LINE_BUFFER_LENGTH

#endif

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

static void defragment_heap()
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
    free_array[0].alloc_type = ALLOC_TYPE_HEAP;

    init_bins();
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

    metadata_t *target_free_array;
    metadata_t *target_alloc_array;
    size_t *target_free_size;
    size_t *target_alloc_size;
    size_t target_capacity;
    allocation_type_t alloc_type;

    if (size <= BIN_8_SIZE)
    {
        target_free_array = free_bin_8;
        target_alloc_array = alloc_bin_8;
        target_free_size = &free_bin_8_size;
        target_alloc_size = &alloc_bin_8_size;
        target_capacity = BIN_8_CAPACITY;
        size = BIN_8_SIZE;
        alloc_type = ALLOC_TYPE_BIN_8;
    }
    else if (size <= BIN_16_SIZE)
    {
        target_free_array = free_bin_16;
        target_alloc_array = alloc_bin_16;
        target_free_size = &free_bin_16_size;
        target_alloc_size = &alloc_bin_16_size;
        target_capacity = BIN_16_CAPACITY;
        size = BIN_16_SIZE;
        alloc_type = ALLOC_TYPE_BIN_16;
    }
    else if (size <= BIN_32_SIZE)
    {
        target_free_array = free_bin_32;
        target_alloc_array = alloc_bin_32;
        target_free_size = &free_bin_32_size;
        target_alloc_size = &alloc_bin_32_size;
        target_capacity = BIN_32_CAPACITY;
        size = BIN_32_SIZE;
        alloc_type = ALLOC_TYPE_BIN_32;
    }
    else
    {
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
            free_array[free_array_size - 1].alloc_type = ALLOC_TYPE_HEAP;

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
            free_array[free_array_size - 1].alloc_type = ALLOC_TYPE_HEAP;
            chunk->size = size;
        }

        add_into_alloc_array(chunk->chunk_ptr, data_ptr,
                             chunk->prev_chunk_ptr, chunk->size,
                             chunk->size - padding, alignment);
        alloc_array[alloc_array_size - 1].alloc_type = ALLOC_TYPE_HEAP;
        remove_from_free_array(best_fit_index);

        return data_ptr;
    }

    if (*target_free_size == 0)
    {
        return NULL;
    }

    metadata_t chunk = target_free_array[0];

    size_t padding = ((alignment - (size_t)chunk.chunk_ptr) & (alignment - 1));
    void *data_ptr = (uint8_t *)chunk.chunk_ptr + padding;

    size_t aligned_size = size;
    if (alignment < size)
    {
        aligned_size = ((size + (size - 1)) / size) * size;
    }

    uint8_t *bin_start;
    uint8_t *bin_end;
    switch (alloc_type)
    {
    case ALLOC_TYPE_BIN_8:
        bin_start = bin_8;
        bin_end = bin_8 + (BIN_8_CAPACITY * BIN_8_SIZE);
        break;
    case ALLOC_TYPE_BIN_16:
        bin_start = bin_16;
        bin_end = bin_16 + (BIN_16_CAPACITY * BIN_16_SIZE);
        break;
    case ALLOC_TYPE_BIN_32:
        bin_start = bin_32;
        bin_end = bin_32 + (BIN_32_CAPACITY * BIN_32_SIZE);
        break;
    default:
        return NULL;
    }

    if ((uint8_t *)data_ptr + aligned_size > bin_end || (uint8_t *)data_ptr < bin_start)
    {
        return NULL;
    }

    metadata_t alloc_chunk = {
        .chunk_ptr = chunk.chunk_ptr,
        .data_ptr = data_ptr,
        .prev_chunk_ptr = chunk.prev_chunk_ptr,
        .size = aligned_size,
        .usable_size = aligned_size - padding,
        .current_alignment = alignment,
        .alloc_type = alloc_type};

    if (!add_into_array(alloc_chunk, target_alloc_array, target_alloc_size, target_capacity))
    {
        return NULL;
    }

    remove_from_array(0, target_free_array, target_free_size);

    return data_ptr;
}

static void init_bins()
{
    static bool has_run = false;
    if (has_run)
    {
        return;
    }

    has_run = true;

    free_bin_8_size = 0;
    for (size_t i = 0; i < BIN_8_CAPACITY; i++)
    {
        void *chunk_ptr = &bin_8[i * BIN_8_SIZE];
        metadata_t chunk = {
            .chunk_ptr = chunk_ptr,
            .data_ptr = chunk_ptr,
            .prev_chunk_ptr = i > 0 ? &bin_8[(i - 1) * BIN_8_SIZE] : NULL,
            .size = BIN_8_SIZE,
            .usable_size = BIN_8_SIZE,
            .current_alignment = MAX_ALIGNMENT,
            .alloc_type = ALLOC_TYPE_BIN_8};
        add_into_array(chunk, free_bin_8, &free_bin_8_size, BIN_8_CAPACITY);
    }

    free_bin_16_size = 0;
    for (size_t i = 0; i < BIN_16_CAPACITY; i++)
    {
        void *chunk_ptr = &bin_16[i * BIN_16_SIZE];
        metadata_t chunk = {
            .chunk_ptr = chunk_ptr,
            .data_ptr = chunk_ptr,
            .prev_chunk_ptr = i > 0 ? &bin_16[(i - 1) * BIN_16_SIZE] : NULL,
            .size = BIN_16_SIZE,
            .usable_size = BIN_16_SIZE,
            .current_alignment = MAX_ALIGNMENT,
            .alloc_type = ALLOC_TYPE_BIN_16};
        add_into_array(chunk, free_bin_16, &free_bin_16_size, BIN_16_CAPACITY);
    }

    free_bin_32_size = 0;
    for (size_t i = 0; i < BIN_32_CAPACITY; i++)
    {
        void *chunk_ptr = &bin_32[i * BIN_32_SIZE];
        metadata_t chunk = {
            .chunk_ptr = chunk_ptr,
            .data_ptr = chunk_ptr,
            .prev_chunk_ptr = i > 0 ? &bin_32[(i - 1) * BIN_32_SIZE] : NULL,
            .size = BIN_32_SIZE,
            .usable_size = BIN_32_SIZE,
            .current_alignment = MAX_ALIGNMENT,
            .alloc_type = ALLOC_TYPE_BIN_32};
        add_into_array(chunk, free_bin_32, &free_bin_32_size, BIN_32_CAPACITY);
    }
}

void heap_free(void *ptr)
{
    if (!ptr)
    {
        return;
    }

    metadata_t *chunk = NULL;
    metadata_t *source_alloc_array = NULL;
    metadata_t *target_free_array = NULL;
    size_t *source_alloc_size = NULL;
    size_t *target_free_size = NULL;
    size_t target_capacity = 0;
    ssize_t alloc_index = -1;

    uintptr_t heap_start = (uintptr_t)heap;
    uintptr_t heap_end = heap_start + HEAP_CAPACITY;
    uintptr_t bin_8_start = (uintptr_t)bin_8;
    uintptr_t bin_8_end = bin_8_start + BIN_8_CAPACITY * BIN_8_SIZE;
    uintptr_t bin_16_start = (uintptr_t)bin_16;
    uintptr_t bin_16_end = (uintptr_t)bin_16 + BIN_16_CAPACITY * BIN_16_SIZE;
    uintptr_t bin_32_start = (uintptr_t)bin_32;
    uintptr_t bin_32_end = bin_32_start + BIN_32_SIZE * BIN_32_CAPACITY;

    if ((uintptr_t)ptr >= bin_8_start && (uintptr_t)ptr < bin_8_end)
    {
        source_alloc_array = alloc_bin_8;
        target_free_array = free_bin_8;
        source_alloc_size = &alloc_bin_8_size;
        target_free_size = &free_bin_8_size;
        target_capacity = BIN_8_CAPACITY;
        alloc_index = search_by_ptr(ptr, alloc_bin_8, alloc_bin_8_size);
    }
    else if ((uintptr_t)ptr >= bin_16_start && (uintptr_t)ptr < bin_16_end)
    {
        source_alloc_array = alloc_bin_16;
        target_free_array = free_bin_16;
        source_alloc_size = &alloc_bin_16_size;
        target_free_size = &free_bin_16_size;
        target_capacity = BIN_16_CAPACITY;
        alloc_index = search_by_ptr(ptr, alloc_bin_16, alloc_bin_16_size);
    }
    else if ((uintptr_t)ptr >= bin_32_start && (uintptr_t)ptr < bin_32_end)
    {
        source_alloc_array = alloc_bin_32;
        target_free_array = free_bin_32;
        source_alloc_size = &alloc_bin_32_size;
        target_free_size = &free_bin_32_size;
        target_capacity = BIN_32_CAPACITY;
        alloc_index = search_by_ptr(ptr, alloc_bin_32, alloc_bin_32_size);
    }
    else if ((uintptr_t)ptr >= heap_start && (uintptr_t)ptr < heap_end)
    {
        alloc_index = search_by_ptr_in_alloc_array(ptr);
    }
    else
    {
        return;
    }

    if (source_alloc_array && alloc_index >= 0)
    {
        chunk = &source_alloc_array[alloc_index];
        metadata_t free_chunk = {
            .chunk_ptr = chunk->chunk_ptr,
            .data_ptr = chunk->data_ptr,
            .prev_chunk_ptr = chunk->prev_chunk_ptr,
            .size = chunk->size,
            .usable_size = chunk->usable_size,
            .current_alignment = chunk->current_alignment,
            .alloc_type = chunk->alloc_type};

        if (!add_into_array(free_chunk, target_free_array, target_free_size, target_capacity))
        {
            return;
        }
        remove_from_array(alloc_index, source_alloc_array, source_alloc_size);
        return;
    }

    if (alloc_index >= 0)
    {
        chunk = &alloc_array[alloc_index];
        add_into_free_array(
            chunk->chunk_ptr,
            chunk->data_ptr,
            chunk->prev_chunk_ptr,
            chunk->size,
            chunk->usable_size,
            chunk->current_alignment);
        free_array[free_array_size - 1].alloc_type = ALLOC_TYPE_HEAP;
        remove_from_alloc_array(alloc_index);
        if (!num_of_free_called_on_heap & (FREE_DEFRAG_CUTOFF - 1))
        {
            defragment_heap();
        }
    }
}

void *heap_realloc(void *ptr, size_t new_size, alignment_t new_alignment)
{
    if (!ptr)
    {
        return heap_alloc(new_size, new_alignment);
    }

    if (!new_size)
    {
        heap_free(ptr);
        return NULL;
    }

    if (((new_alignment) & (new_alignment - 1)) || (new_alignment > MAX_ALIGNMENT))
    {
        new_alignment = DEFAULT_ALIGNMENT;
    }

    ssize_t ptr_index = search_by_ptr_in_alloc_array(ptr);
    if (ptr_index < 0)
    {
        return NULL;
    }

    metadata_t *chunk = &alloc_array[ptr_index];

    if (new_size <= chunk->size)
    {
        if (new_alignment != chunk->current_alignment)
        {
            void *new_ptr = heap_alloc(new_size, new_alignment);
            if (!new_ptr)
            {
                return NULL;
            }

            size_t copy_size = new_size < chunk->usable_size ? new_size : chunk->usable_size;
            memcpy(new_ptr, ptr, copy_size);
            heap_free(ptr);
            return new_ptr;
        }

        // now if the alignment is same and the size is below
        size_t remaining = chunk->size - new_size;

        // only split if remaining space is above cutoff
        if (remaining >= SPLIT_CUTOFF)
        {
            void *new_chunk_ptr = (uint8_t *)chunk->chunk_ptr + new_size;
            add_into_free_array(new_chunk_ptr, new_chunk_ptr,
                                chunk->chunk_ptr, remaining, remaining,
                                calculate_alignment(new_chunk_ptr));
            chunk->size = new_size;
        }

        return ptr;
    }

    void *new_ptr = heap_alloc(new_size, new_alignment);
    if (!new_ptr)
    {
        return NULL;
    }

    memcpy(new_ptr, ptr, chunk->usable_size);
    heap_free(ptr);
    return new_ptr;
}

#endif // MEM_IMPLEMENTATION

#undef HEAP_CAPACITY
#undef FREE_CAPACITY
#undef ALLOC_CAPACITY

#undef MAX_ALIGNMENT
#undef MAX_ALIGNMENT_INT
#undef DEFAULT_ALIGNMENT // by default align on pointer size, this is enough for most platforms and architectures

#undef SPLIT_CUTOFF

#undef FREE_DEFRAG_CUTOFF // must be a power of 2

#undef THREAD_LOCAL

#undef BIN_8_SIZE
#undef BIN_16_SIZE
#undef BIN_32_SIZE
#undef BIN_8_CAPACITY
#undef BIN_16_CAPACITY
#undef BIN_32_CAPACITY

#endif /* D46AFE7A_7823_4C7A_A759_A5737B4A74D1 */
