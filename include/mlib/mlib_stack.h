/**
 * @file mlib_stack.h
 * @brief Dynamic array-backed LIFO stack implementation with value semantics.
 *
 * This module implements an opaque, contiguously allocated Last-In-First-Out (LIFO)
 * stack container. Elements are stored by value using fixed-size memory blocks
 * (`elem_size`). Geometric growth guarantees amortized O(1) push operations while
 * maintaining spatial data locality.
 */

#ifndef MLIB_STACK_H
#define MLIB_STACK_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef mlib_stack_t
 * @brief Main LIFO stack container handle (opaque).
 */
typedef struct mlib_stack mlib_stack_t;

/* ========================================================================== */
/* Lifecycle Management                                                       */
/* ========================================================================== */

/**
 * @brief Allocates and initializes an empty LIFO stack instance.
 *
 * @pre `elem_size > 0`
 *
 * @param[in] elem_size        Size in bytes of each stored element. Must be strictly positive.
 * @param[in] initial_capacity Initial capacity slot count. `0` enables lazy buffer allocation.
 * @param[in] free_fn          Optional destructor callback invoked on each element slot address
 *                             before removal, clearing, or destruction. Pass `NULL` if elements
 *                             are plain-old-data (POD) or do not manage external resources.
 *
 * @return Pointer to the allocated `mlib_stack_t` container on success,
 *         or `NULL` if `elem_size == 0` or heap allocation fails.
 *
 * @note **Ownership:** The library allocates and owns the stack container and internal buffer.
 * @note **Complexity:** O(1) if `initial_capacity == 0`, otherwise O(initial_capacity).
 * @note **Thread Safety:** Thread-safe for invocation (independent instances).
 */
mlib_stack_t *mlib_stack_create(size_t elem_size, size_t initial_capacity,
                                mlib_free_fn free_fn);

/**
 * @brief Removes and cleans up all elements, resetting stack size to 0.
 *
 * If `free_fn != NULL`, it is invoked once for each active element from top to bottom.
 * The underlying buffer capacity remains allocated for reuse.
 *
 * @param[in,out] stack Pointer to the stack instance.
 *                      If `NULL`, this function is a safe no-op.
 *
 * @warning **Pointer Invalidation:** Any pointers previously returned by @ref mlib_stack_peek
 *          are rendered dangling and must not be dereferenced.
 *
 * @note **Complexity:** O(N) if `free_fn` is provided; O(1) if `free_fn == NULL`.
 * @note **Thread Safety:** Not thread-safe on concurrent access to the same instance.
 */
void mlib_stack_clear(mlib_stack_t *stack);

/**
 * @brief Deallocates all resources, destroys the stack, and nullifies the caller handle.
 *
 * Invokes @ref mlib_stack_clear, deallocates the internal contiguous buffer,
 * frees the `mlib_stack_t` structure, and writes `NULL` to `*stack`.
 *
 * @param[in,out] stack Double pointer to the stack handle.
 *                      Safe no-op if `stack == NULL` or `*stack == NULL`.
 *
 * @warning **Pointer Invalidation:** `*stack` is nullified; all references to the stack
 *          and its elements are permanently invalidated.
 *
 * @note **Ownership:** Deallocates all memory owned by the container.
 * @note **Complexity:** O(N) if `free_fn` is provided; O(1) otherwise.
 * @note **Thread Safety:** Not thread-safe.
 */
void mlib_stack_destroy(mlib_stack_t **stack);

/* ========================================================================== */
/* Data Operations (Push / Pop / Peek)                                        */
/* ========================================================================== */

/**
 * @brief Pushes a new element onto the top of the stack.
 *
 * Copies `elem_size` bytes from the memory location pointed to by `elem` into
 * the stack. If the internal buffer is full, capacity is automatically doubled.
 *
 * @pre If `elem != NULL`, it must point to at least `elem_size` readable bytes.
 *
 * @param[in,out] stack Pointer to the stack container.
 * @param[in]     elem  Pointer to the source payload to copy.
 *                      If `NULL`, the newly allocated element slot is filled with zero bytes.
 *
 * @return Status code indicating the outcome:
 *         - #MLIB_SUCCESS : Element successfully pushed.
 *         - #MLIB_ERR_NULL_PARAM : `stack` was `NULL`.
 *         - #MLIB_ERR_ALLOC : Internal buffer reallocation failed (stack remains unmodified).
 *
 * @warning **Pointer Invalidation:** If geometric growth triggers buffer reallocation,
 *          any pointer previously retrieved via @ref mlib_stack_peek is invalidated.
 * @warning **Aliasing:** `elem` must not point into the current internal buffer of `stack`.
 *
 * @note **Ownership:** Value semantics. Memory is copied internally; caller retains
 *       full ownership of the source buffer `elem`.
 * @note **Complexity:** Amortized O(1); worst-case O(N) when triggering buffer reallocation.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_stack_push(mlib_stack_t *stack, const void *elem);

/**
 * @brief Removes the top element from the stack.
 *
 * If configured, `free_fn` is invoked on the top element's address before
 * the logical stack size is decremented.
 *
 * @param[in,out] stack Pointer to the stack container.
 *
 * @return Status code indicating the outcome:
 *         - #MLIB_SUCCESS : Top element removed.
 *         - #MLIB_ERR_NULL_PARAM : `stack` was `NULL`.
 *         - #MLIB_ERR_EMPTY : Stack contains zero elements.
 *
 * @warning **Pointer Invalidation:** Pointers to the popped element previously
 *          obtained via @ref mlib_stack_peek become invalid.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_stack_pop(mlib_stack_t *stack);

/**
 * @brief Retrieves a direct pointer to the element currently at the top of the stack.
 *
 * Inspects the top element without removing it.
 *
 * @param[in] stack Pointer to the stack container.
 *
 * @return Pointer to top element's payload in the internal buffer,
 *         or `NULL` if `stack == NULL` or stack is empty.
 *
 * @warning **Pointer Invalidation & Lifetime:** The returned pointer references internal
 *          buffer storage. It remains valid ONLY until the next modifying operation
 *          (@ref mlib_stack_push, @ref mlib_stack_pop, @ref mlib_stack_clear,
 *           @ref mlib_stack_reserve, @ref mlib_stack_shrink_to_fit).
 *
 * @note **Ownership:** Caller must NOT invoke free() on the returned pointer.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent calls if no write operations occur.
 */
void *mlib_stack_peek(const mlib_stack_t *stack);

/* ========================================================================== */
/* Capacity & Size Inspection                                                 */
/* ========================================================================== */

/**
 * @brief Retrieves the current number of elements stored in the stack.
 *
 * @param[in] stack Pointer to the stack instance. Can be `NULL`.
 *
 * @return Active element count, or `0` if `stack == NULL`.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
size_t mlib_stack_size(const mlib_stack_t *stack);

/**
 * @brief Retrieves the total capacity (in elements) allocated in the buffer.
 *
 * @param[in] stack Pointer to the stack instance. Can be `NULL`.
 *
 * @return Total allocated element slots, or `0` if `stack == NULL`.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
size_t mlib_stack_capacity(const mlib_stack_t *stack);

/**
 * @brief Checks whether the stack contains zero elements.
 *
 * @param[in] stack Pointer to the stack instance. Can be `NULL`.
 *
 * @return `true` if `stack == NULL` or `size == 0`; `false` otherwise.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
bool mlib_stack_is_empty(const mlib_stack_t *stack);

/* ========================================================================== */
/* Memory & Capacity Management                                               */
/* ========================================================================== */

/**
 * @brief Pre-allocates buffer storage for at least `new_capacity` elements.
 *
 * If `new_capacity <= current_capacity`, the function performs no operation.
 * Prevents multiple reallocations when the upper bound is known in advance.
 *
 * @param[in,out] stack        Pointer to the stack instance.
 * @param[in]     new_capacity Minimum required slot capacity.
 *
 * @return Status code indicating the outcome:
 *         - #MLIB_SUCCESS : Buffer expanded or was already sufficient.
 *         - #MLIB_ERR_NULL_PARAM : `stack` was `NULL`.
 *         - #MLIB_ERR_ALLOC : Heap reallocation failed (state remains unchanged).
 *
 * @warning **Pointer Invalidation:** Reallocation invalidates existing pointers from @ref mlib_stack_peek.
 *
 * @note **Complexity:** O(N) where N is current element count.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_stack_reserve(mlib_stack_t *stack, size_t new_capacity);

/**
 * @brief Shrinks the internal buffer to fit the current element count exactly.
 *
 * Deallocates unused surplus capacity. If the stack is empty (`size == 0`),
 * internal buffer memory may be completely released.
 *
 * @param[in,out] stack Pointer to the stack instance.
 *
 * @return Status code indicating the outcome:
 *         - #MLIB_SUCCESS : Buffer resized to match size.
 *         - #MLIB_ERR_NULL_PARAM : `stack` was `NULL`.
 *         - #MLIB_ERR_ALLOC : Reallocation failed (buffer remains unchanged).
 *
 * @warning **Pointer Invalidation:** Reallocation invalidates existing pointers from @ref mlib_stack_peek.
 *
 * @note **Complexity:** O(N) where N is current element count.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_stack_shrink_to_fit(mlib_stack_t *stack);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_STACK_H */
