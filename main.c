#include <stdlib.h>
#include "mem_alloc.h"
#include <stdio.h>

int main(void)
{
    for (volatile size_t i = 0; i < 10000; i++)
    {
        void *ptr = heap_alloc(i, ALIGN_DEFAULT);
        printf("%p\n", ptr);
        heap_free(ptr);
    }
}