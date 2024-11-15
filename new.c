#include <stdio.h>
#include "mem.h"

int main()
{
    heap_init();

    void *ptr1 = heap_alloc(10, ALIGN_16);
    void *ptr2 = heap_alloc(20, DEFAULT_ALIGNMENT);
    printf("%p\n", ptr1);
    printf("%p\n", ptr2);

    heap_free(ptr1);
    printf("%p\n", ptr2);
}