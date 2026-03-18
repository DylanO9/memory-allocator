import block_structure from '../../../img/csapp/malloc_lab/block_structure.png';
import hdr_ftr_structure from '../../../img/csapp/malloc_lab/hdr_ftr_structure.png';
import heap_layout from '../../../img/csapp/malloc_lab/heap_layout.png';
import implicit_results from '../../../img/csapp/malloc_lab/implicit_free_list_results.png';

# Memory Allocator in C

## Overview
I implemented a dynamic memory allocator in C to understand how heap memory is managed at a low level and how allocator design impacts performance.

I first built an implicit free list allocator using boundary tags to support coalescing. After observing poor allocation throughput due to linear scans over all blocks, I redesigned the allocator as an explicit free list, reducing search cost by traversing only free blocks.

This project reinforced key systems concepts including heap layout, fragmentation, metadata encoding, and allocator tradeoffs between throughput and memory utilization.

---

## Key Takeaways
- Built both implicit and explicit free list allocators from scratch
- Implemented boundary-tag coalescing in constant time
- Analyzed internal vs external fragmentation
- Improved allocation performance from scanning all blocks to scanning only free blocks
- Developed intuition for how allocator design affects real program performance

---

## Why Memory Allocators Matter
Memory allocators sit on the critical path of many programs. Every call to malloc and free depends on how efficiently the allocator manages the heap.

Small design decisions—such as how free blocks are tracked or how memory is split—can significantly impact:
- Throughput (speed of allocations)
- Memory utilization (how efficiently space is used)

Understanding allocators enables building specialized memory managers tailored to specific workloads.

---

## Allocator Interface
```
mm_init()       // Initialize heap and metadata
mm_malloc(size) // Allocate memory
mm_free(ptr)    // Free memory
mm_realloc(ptr) // Resize allocation
```

---

# Design 1: Implicit Free List

## Heap Layout
<img src={heap_layout} alt="Heap layout" style={{maxWidth: '300px', width: '100%', height: 'auto'}} />

The heap is organized as a sequence of blocks:
- Prologue block: simplifies boundary conditions
- Regular blocks: allocated or free
- Epilogue block: marks the end of the heap

### Why prologue and epilogue?
They eliminate edge cases during coalescing and prevent invalid memory access at boundaries.

---

## Block Structure
<img src={block_structure} alt="Block structure" style={{maxWidth: '300px', width: '100%', height: 'auto'}} />

Each block consists of:
- Header: stores size and allocation status
- Payload: usable memory
- Footer: duplicate of header

### Why include a footer?
Footers enable constant-time coalescing with the previous block.

---

## Header/Footer Encoding
<img src={hdr_ftr_structure} alt="Header/Footer structure" style={{maxWidth: '300px', width: '100%', height: 'auto'}} />

Each header/footer stores:
- Block size (aligned to 8 bytes)
- Allocation bit (1 = allocated, 0 = free)

---

## Invariants
- All blocks are 8-byte aligned
- Header and footer must match
- Block traversal uses size
- Allocation bit reflects status

---

## Allocation Strategy
- Linear scan through the heap
- First-fit placement
- Split blocks when needed

---

## Coalescing
When freeing a block:
- Merge with adjacent free blocks
- Use footer for constant-time backward access

---

## Complexity
- mm_malloc: O(n)
- mm_free: O(1)

---

# Implicit Free List Results

<img src={implicit_results} alt="Implicit Free List Results" style={{maxWidth: '300px', width: '100%', height: 'auto'}} />

## Memory Utilization
High utilization due to minimal metadata overhead.

## Throughput
Low throughput due to scanning all blocks, including allocated ones.

---

## Key Insight
The allocator scans all blocks instead of only free blocks.

---

# Design 2: Explicit Free List

## Motivation
Avoid scanning allocated blocks by maintaining a list of only free blocks.

---

## Key Change
Free blocks now store:
- Previous pointer
- Next pointer

---

## Complexity Improvement
- mm_malloc: O(m) where m = free blocks
- mm_free: O(1)

---

## Tradeoffs
Pros:
- Faster allocation
- Less unnecessary traversal

Cons:
- More metadata
- Slightly lower utilization
- More complexity

---

# Lessons Learned

## Throughput vs Utilization
Implicit: better utilization, worse speed  
Explicit: better speed, slightly worse utilization

## Metadata Matters
Extra pointers impact memory usage.

## Data Structures Drive Performance
Allocator performance depends heavily on structure choice.

## Visualization
Understanding required reasoning about contiguous memory and pointer arithmetic.

---

# Future Improvements
- Segregated free lists
- Best-fit vs first-fit
- Remove footers for allocated blocks
- Optimize realloc
- Heap checker

---

# Final Reflection
Building a memory allocator transformed my understanding of memory from abstract concepts into concrete mechanics.

Concepts like fragmentation, coalescing, and heap traversal now feel intuitive.