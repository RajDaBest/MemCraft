#include <stdio.h>
#include "mem_alloc.h"
#include <stdlib.h>

typedef struct Node
{
    int value;
    struct Node *next;
} Node;

Node *global_list = NULL;

Node *create_mixed_list()
{
    Node *head = heap_alloc(sizeof(Node), ALIGN_DEFAULT);
    head->value = 42;

    Node *current = head;
    for (int i = 0; i < 5; i++)
    {
        current->next = heap_alloc(sizeof(Node), ALIGN_DEFAULT);
        current = current->next;
        current->value = i;
        current->next = NULL;
    }

    Node *unreachable = heap_alloc(sizeof(Node), ALIGN_DEFAULT);
    unreachable->value = 999;
    unreachable->next = NULL;

    return head;
}

void print_list(Node *head)
{
    printf("List contents: ");
    while (head != NULL)
    {
        printf("%d ", head->value);
        head = head->next;
    }
    printf("\n");
}

int main()
{
    heap_init();
    global_list = create_mixed_list();
    gc_register_root(global_list);

    printf("Before GC:\n");
    print_list(global_list);

    printf("\nPerforming Garbage Collection...\n");
    gc_collect();

    printf("\nAfter GC:\n");
    print_list(global_list);

    Node *temp = global_list->next->next;
    global_list->next->next = NULL;

    printf("\nAfter breaking links:\n");
    print_list(global_list);

    printf("\nPerforming Second Garbage Collection...\n");
    gc_collect();

    printf("\nFinal list state:\n");
    print_list(global_list);

    return 0;
}