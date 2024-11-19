# MemCraft

A modular, high-performance heap memory allocator implementation in C with support for multiple allocation strategies and memory integrity verification.

## Features

- **Multiple Allocation Strategies**
  - Segmented Allocator: Optimized for different size classes with separate memory bins and a traditional heap for general allocations with memory coalescing
  - Inline Allocator: Traditional inline metadata approach with memory coalescing
  
- **Memory Alignment**
  - Configurable alignment support (4, 8, 16, 32 bytes)
  - Automatic alignment detection and optimization
  
- **Memory Safety**
  - Integrity checking using checksums (XXH32 or CRC32)
  - Metadata validation for corruption detection
  - Boundary checks and alignment verification

- **Performance Optimizations**
  - Binary search for efficient block location
  - Smart splitting and coalescing strategies
  - Size-specific memory pools for common allocation sizes

- **Debug Support**
  - Detailed allocation/deallocation logging
  - Memory statistics tracking
  - Corruption detection and reporting

## Project Structure

```
MemCraft/
├── src/
│   ├── allocator_implementations/
│   │   ├── segmented_allocator.h
│   │   └── inline_allocator.h
│   └── checksum_implementations/
│       ├── xxh32.h
│       └── crc32.h
├── mem_alloc.h
└── README.md
```

## Usage

### Basic Integration

```c
#include "mem_alloc.h"

// Initialize the heap (must be called before any allocations)
heap_init();

// Allocate memory with specific alignment
void* ptr = heap_alloc(1024, ALIGN_16);

// Reallocate memory with new size and alignment
ptr = heap_realloc(ptr, 2048, ALIGN_32);

// Free allocated memory
heap_free(ptr);
```

### Choosing Allocator Implementation

The project supports two allocator implementations that can be selected at compile time:

```c
// Use inline allocator
#define INLINE_ALLOCATOR
#include "mem_alloc.h"

// Or use segmented allocator (default)
#include "mem_alloc.h"
```

### Memory Statistics

```c
size_t total_size, used_size, free_size, largest_block;
heap_get_stats(&total_size, &used_size, &free_size, &largest_block);
```

## Configuration

### Heap Settings

```c
#define HEAP_CAPACITY (65536)    // 64KB heap size
#define SPLIT_THRESHOLD (16)     // Minimum size for block splitting
```

### Alignment Options

```c
typedef enum {
    ALIGN_4 = 4,
    ALIGN_8 = 8,
    ALIGN_16 = 16,
    ALIGN_32 = 32,
    NO_ALIGNMENT = 0,
} alignment_t;
```

### Debug Settings

```c
#define DEBUG_LOGGING (1)        // Enable/disable debug logging
```

## Implementation Details

### Segmented Allocator

The segmented allocator implements a multi-bin strategy with:
- **MultiThread-Safe**
- Fixed-size bins (8, 16, 32 bytes) for common allocation sizes
- Standard Heap allocation for allocation above 32 bytes
- Binary search for efficient block location
- Separate free lists for different size classes
- Automatic defragmentation of freed blocks

### Inline Allocator

The inline allocator uses:
- Inline metadata storage with checksums
- Best-fit allocation strategy
- Automatic block coalescing
- Smart splitting of large blocks
- Memory corruption detection

## Memory Safety

Both allocator implementations include several safety features:

1. Metadata Integrity
   - Checksum verification of block metadata
   - Detection of corrupted blocks
   - Validation of block boundaries

2. Memory Protection
   - Alignment enforcement
   - Boundary checking
   - Pointer validation

3. Error Detection
   - Invalid pointer detection
   - Memory corruption detection
   - Out-of-bounds access detection

## Performance Considerations

### Segmented Allocator
- Best for applications with many small allocations
- Reduced fragmentation for common size classes
- Fast allocation for small blocks
- Higher memory overhead due to bin structure

### Inline Allocator
- Better memory utilization for large blocks
- Lower memory overhead
- More predictable performance
- Better suited for varied allocation sizes

## Building and Testing

The project is header-only and can be integrated directly into your C project. To use:

1. Copy the project files to your source directory
2. Include `mem_alloc.h` in your code
3. Choose your allocator implementation
4. Configure heap settings as needed

## License

This project is available under the MIT License. See the LICENSE file for more details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or create issues for bugs and feature requests.

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Open a Pull Request

## Author

[Raj Shukla]
