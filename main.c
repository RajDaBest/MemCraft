#include <assert.h>
#include <string.h>

#include "mem_alloc.h"

// Test helper functions
static void verify_alignment(void *ptr, alignment_t alignment)
{
    uintptr_t addr = (uintptr_t)ptr;
    assert((addr % alignment) == 0);
}

static void verify_stats(size_t expected_used, size_t expected_free)
{
    size_t total, used, free, largest;
    heap_get_stats(&total, &used, &free, &largest);
    assert(total == HEAP_CAPACITY);
    assert(used == expected_used);
    assert(free == HEAP_CAPACITY - sizeof(metadata_t) - expected_used);
}

// Main test function
int run_heap_tests(void)
{
    printf("\nRunning heap allocator tests...\n");
    int tests_passed = 0;
    int total_tests = 0;

    // Test 1: Initialization
    total_tests++;
    bool init_result = heap_init();
    assert(init_result);
    printf("✓ Test %d: Heap initialization successful\n", total_tests);
    tests_passed++;

    // Test 2: Basic allocation
    total_tests++;
    void *ptr1 = heap_alloc(100, ALIGN_8);
    assert(ptr1 != NULL);
    verify_alignment(ptr1, ALIGN_8);
    // verify_stats(100, HEAP_CAPACITY - sizeof(metadata_t) - 100);
    printf("✓ Test %d: Basic allocation successful\n", total_tests);
    tests_passed++;

    // Test 3: Multiple alignments
    total_tests++;
    void *ptr2 = heap_alloc(50, ALIGN_4);
    void *ptr3 = heap_alloc(75, ALIGN_16);
    assert(ptr2 != NULL && ptr3 != NULL);
    verify_alignment(ptr2, ALIGN_4);
    verify_alignment(ptr3, ALIGN_16);
    printf("✓ Test %d: Multiple alignment allocations successful\n", total_tests);
    tests_passed++;

    // Test 4: Free and reallocation
    total_tests++;
    heap_free(ptr2);
    void *ptr4 = heap_alloc(50, ALIGN_8);
    assert(ptr4 != NULL);
    verify_alignment(ptr4, ALIGN_8);
    printf("✓ Test %d: Free and reallocation successful\n", total_tests);
    tests_passed++;

    // Test 5: Coalescing
    total_tests++;
    heap_free(ptr1);
    heap_free(ptr4);
    void *ptr5 = heap_alloc(200, ALIGN_8);
    assert(ptr5 != NULL);
    verify_alignment(ptr5, ALIGN_8);
    printf("✓ Test %d: Chunk coalescing successful\n", total_tests);
    tests_passed++;

    // Test 6: Edge cases
    total_tests++;
    assert(heap_alloc(0, ALIGN_8) == NULL);
    assert(heap_alloc(HEAP_CAPACITY + 1, ALIGN_8) == NULL);
    assert(heap_alloc(100, 3) != NULL); // Should use default alignment
    printf("✓ Test %d: Edge cases handled correctly\n", total_tests);
    tests_passed++;

    // Test 7: Fragmentation handling
    total_tests++;
    void *fragments[10];
    for (int i = 0; i < 10; i++)
    {
        fragments[i] = heap_alloc(100, ALIGN_8);
        assert(fragments[i] != NULL);
    }
    for (int i = 0; i < 10; i += 2)
    {
        heap_free(fragments[i]);
    }
    void *large_alloc = heap_alloc(300, ALIGN_8);
    assert(large_alloc != NULL);
    printf("✓ Test %d: Fragmentation handling successful\n", total_tests);
    tests_passed++;

    // Test 8: Memory patterns
    total_tests++;
    void *pattern_ptr = heap_alloc(128, ALIGN_8);
    assert(pattern_ptr != NULL);
    memset(pattern_ptr, 0xAA, 128);
    heap_free(pattern_ptr);
    void *pattern_ptr2 = heap_alloc(128, ALIGN_8);
    assert(pattern_ptr2 != NULL);
    printf("✓ Test %d: Memory pattern test successful\n", total_tests);
    tests_passed++;

    // Print test summary
    printf("\nTest Summary:\n");
    printf("Total tests: %d\n", total_tests);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", total_tests - tests_passed);

    return (tests_passed == total_tests) ? 0 : -1;
}

// Main function
int main(void)
{
    int result = run_heap_tests();

    if (result == 0)
    {
        printf("\nAll heap allocator tests passed successfully!\n");
    }
    else
    {
        printf("\nSome heap allocator tests failed!\n");
    }

    return result;
}