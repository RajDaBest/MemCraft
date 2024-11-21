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

void collect_thread_stack()
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

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Thread function that will be executed by each thread
void *thread_function(void *arg)
{
    int thread_num = *((int *)arg); // Get the thread number from the argument
    while (1)
    {
        printf("Thread %d is running indefinitely...\n", thread_num);
        sleep(1); // Sleep for 1 second to simulate some work
    }
    return NULL;
}

int main()
{
    int num_threads = 5;            // Number of threads to create
    pthread_t threads[num_threads]; // Array to hold thread IDs
    int thread_nums[num_threads];   // Array to hold thread arguments

    // Create the threads
    for (int i = 0; i < num_threads; i++)
    {
        thread_nums[i] = i + 1; // Set thread number
        if (pthread_create(&threads[i], NULL, thread_function, &thread_nums[i]) != 0)
        {
            perror("Failed to create thread");
            return 1;
        }
        printf("Created thread %d\n", i + 1);
    }

    collect_thread_stack();

    printf("pid: %zu\n", (size_t)getpid());
    // Keep the main program running indefinitely
    while (1)
    {
        // Main thread could do other work, but we'll just sleep here
        sleep(2);
        collect_thread_stack();
    }

    // Join threads (although this code will never reach here)
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
