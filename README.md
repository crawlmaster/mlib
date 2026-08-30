# C Data Structures & Algorithms Library

A modular, generic, and memory-safe data structures library written in C11.

## Modules Roadmap
- [x] Singly Linked Lists
- [ ] Dynamic Vector
- [ ] Generic Stack / Queue
- [ ] Binary Heap & Priority Queue
- [ ] Binary Search Tree & AVL Tree
- [ ] Red-Black Tree
- [ ] Fibonacci Heap
- [ ] Graphs
- [ ] Graph Algorithms - Shortest Paths, MST, etc.

## Build & Test

```bash
# Compile library and run unit test suite
make test

# Run tests with AddressSanitizer and UBSan
make sanitize

# Run memory leak checks with Valgrind
make memcheck
```