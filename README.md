# mlib - Modular C Data Structures & Algorithms Library

A high-performance, modular, and memory-safe data structures library written in C11. Built with value semantics for array-backed containers, strict branch-prediction optimizations, comprehensive Doxygen API contracts, and zero-leak memory verification.

[![Version](https://img.shields.io/badge/version-0.8.0-blue.svg)](https://github.com/)
[![Standard](https://img.shields.io/badge/c-c11-blue.svg)](https://en.cppreference.com/w/c/11)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## Modules Roadmap

- [x] **Singly Linked List (`sll`)** - Generic pointer-based list with cached tail and iterators
- [x] **Dynamic Vector (`vector`)** - Cache-friendly contiguous array with growth tracking and in-place sorting
- [x] **LIFO Stack (`stack`)** - Array-backed stack with value semantics and geometric resizing
- [x] **FIFO Queue (`queue`)** - Circular ring buffer with power-of-two capacity and bitwise index masking
- [ ] Binary Heap & Priority Queue
- [ ] Binary Search Tree & AVL Tree
- [ ] Red-Black Tree
- [ ] Trie & Radix-Trie
- [ ] Fibonacci Heap
- [ ] Graphs (Adjacency List & Matrix)
- [ ] Graph Algorithms (Dijkstra, Bellman-Ford, Prim, Kruskal, BFS/DFS)

> **Implementation Criteria:** A module is checked off once all mandatory container operations, edge cases, lifecycle functions, and automated test suites pass without memory leaks or undefined behavior.

---

## Prerequisites & Toolchain

* **C Compiler:** GCC 9+ or Clang 11+ (C11 support required)
* **Build System:** **GNU Make 4.0+**
* **Memory Analyzers (Optional):**
  * Linux / Windows: `valgrind`
  * macOS: Native `leaks` (bundled with Xcode Command Line Tools)
* **Documentation:** `doxygen` (for generating HTML/LaTeX reference manuals)
* **Formatting:** `clang-format`

### macOS Notice (Important)
Apple bundles an outdated **GNU Make 3.81** (released in 2006) with Xcode Command Line Tools. To prevent pattern rule resolution conflicts during test builds, install and use modern GNU Make:

```bash
brew install make
```
