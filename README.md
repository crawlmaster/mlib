# C Data Structures & Algorithms Library

A modular, generic, and memory-safe data structures library written in C11.

## Modules Roadmap
- [ ] Singly & Doubly Linked Lists
- [ ] Generic Stack / Queue
- [ ] Dynamic Vector
- [ ] Binary Heap & Priority Queue
- [ ] Binary Search Tree & AVL Tree
- [ ] Red-Black Tree
- [ ] Fibonacci Heap

## Build & Test

```bash
# Compile library and run unit test suite
make test

# Run tests with AddressSanitizer and UBSan
make sanitize

# Run memory leak checks with Valgrind
make memcheck
