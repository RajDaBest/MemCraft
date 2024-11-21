#ifdef INLINE_ALLOCATOR
#define MEM_IMPLEMENTATION
#include "src/allocator_implementations/inline_allocator.h"

#else
#define MEM_IMPLEMENTATION
#define GC_COLLECT
#include "src/allocator_implementations/segmented_allocator.h"
#endif


