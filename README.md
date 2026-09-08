# C Data Structures & Algorithms Library

A modular, generic, and memory-safe data structures library written in C11.

## Modules Roadmap
- [x] Singly Linked Lists
- [x] Dynamic Vector
- [ ] Generic Stack / Queue
- [ ] Binary Heap & Priority Queue
- [ ] Binary Search Tree & AVL Tree
- [ ] Red-Black Tree
- [ ] Trie & Radix-Trie
- [ ] Fibonacci Heap
- [ ] Graphs
- [ ] Graph Algorithms - Shortest Paths, MST, etc.

* A data structure is considered implemented and can be checked off when all the mandatory functions for that structure have been implemented. Additional useful functions can be implemented later in development.
## Build & Test

```bash
# Compile library and run unit test suite
make test

# Run tests with AddressSanitizer and UBSan
make sanitize

# Run memory leak checks with Valgrind
make memcheck
```

## Prerequisites & Build Tools

* **C Compiler:** GCC 9+ or Clang 11+ (C11 support required)
* **Build System:** **GNU Make 4.0+**

### macOS Notice
Apple bundles an outdated **GNU Make 3.81** with Xcode Command Line Tools. To avoid pattern rule resolution issues with unit test targets, install and use a modern GNU Make:

```bash
brew install make
```
