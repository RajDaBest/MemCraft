#include "mem_alloc.h"
#include <stdio.h>

int main(void)
{
    void *ptr = heap_alloc(10, ALIGN_DEFAULT);
    printf("%p\n", ptr);
}