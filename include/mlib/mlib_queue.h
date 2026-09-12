/**
 * @file mlib_queue.h
 * @brief Ring buffer FIFO queue implementation with power-of-two capacity.
 *
 * This module implements an opaque First-In-First-Out (FIFO) queue container backed
 * by a contiguous circular array. Elements are copied by value (`elem_size`).
 * The queue strictly enforces power-of-two buffer capacities, enabling wrap-around
 * index calculations via single-cycle bitwise masking (`idx & (cap - 1)`) rather
 * than costly integer division (`%`).
 */

#ifndef MLIB_QUEUE_H
#define MLIB_QUEUE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef mlib_queue_t
 * @brief Main FIFO ring buffer queue container handle (opaque).
 */
typedef struct mlib_queue mlib_queue_t;

/* ========================================================================== */
/* Lifecycle Management                                                       */
/* ========================================================================== */

/**
 * @brief Allocates and initializes a new contiguous ring buffer FIFO queue.
 *
 * Automatically rounds up `initial_capacity` to the nearest power of two
 * (enforcing an architectural minimum, e.g. 8 slots) for bitwise indexing.
 *
 * @pre `elem_size > 0`
 *
 * @param[in] elem_size        Size in bytes of each stored element. Must be strictly positive.
 * @param[in] initial_capacity Desired initial capacity (rounded up to nearest power of two).
 * @param[in] free_fn          Optional destructor callback invoked on each element slot address
 *                             before removal, clear, or destroy. Pass `NULL` if elements
 *                             do not own dynamic heap allocations.
 *
 * @return Pointer to the allocated `mlib_queue_t` container on success,
 *         or `NULL` if `elem_size == 0` or heap allocation fails.
 *
 * @note **Ownership:** Library allocates and owns the container and ring buffer memory.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Thread-safe for invocation (independent instances).
 */
mlib_queue_t *mlib_queue_create(size_t elem_size, size_t initial_capacity,
                                mlib_free_fn free_fn);

/**
 * @brief Removes and cleans up all stored elements, resetting size to 0.
 *
 * Invokes `free_fn` for each logically stored element from front to back,
 * correctly traversing circular buffer wrap-around boundaries.
 * Buffer capacity and allocated storage remain intact.
 *
 * @param[in,out] queue Pointer to the queue instance.
 *                      If `NULL`, this function is a safe no-op.
 *
 * @warning **Pointer Invalidation:** Any pointers previously returned by @ref mlib_queue_peek
 *          are rendered dangling and must not be dereferenced.
 *
 * @note **Complexity:** O(N) if `free_fn` is provided; O(1) if `free_fn == NULL`.
 * @note **Thread Safety:** Not thread-safe on concurrent access.
 */
void mlib_queue_clear(mlib_queue_t *queue);

/**
 * @brief Deallocates all resources, destroys the queue, and nullifies the caller handle.
 *
 * Calls @ref mlib_queue_clear, deallocates the contiguous circular buffer,
 * frees the `mlib_queue_t` handle, and sets `*queue = NULL`.
 *
 * @param[in,out] queue Double pointer to the queue handle.
 *                      Safe no-op if `queue == NULL` or `*queue == NULL`.
 *
 * @warning **Pointer Invalidation:** `*queue` is nullified; all references to the queue
 *          and elements inside become invalid.
 *
 * @note **Ownership:** Releases all memory owned by the queue container.
 * @note **Complexity:** O(N) if `free_fn` is provided; O(1) otherwise.
 * @note **Thread Safety:** Not thread-safe.
 */
void mlib_queue_destroy(mlib_queue_t **queue);

/* ========================================================================== */
/* Data Operations (Enqueue / Dequeue / Peek)                                 */
/* ========================================================================== */

/**
 * @brief Inserts a new element at the back of the queue (enqueue).
 *
 * Copies `elem_size` bytes from `elem` into the slot indexed by the write cursor.
 * When the queue is full, capacity doubles (preserving power-of-two alignment)
 * and split circular segments are realigned contiguously.
 *
 * @pre If `elem != NULL`, it must point to at least `elem_size` readable bytes.
 *
 * @param[in,out] queue Pointer to the target queue container.
 * @param[in]     elem  Pointer to source payload to copy.
 *                      If `NULL`, the newly allocated slot is zero-initialized.
 *
 * @return Status code indicating the outcome:
 *         - #MLIB_SUCCESS : Element appended to back.
 *         - #MLIB_ERR_NULL_PARAM : `queue` pointer was `NULL`.
 *         - #MLIB_ERR_ALLOC : Buffer expansion/realignment failed (state remains unchanged).
 *
 * @warning **Pointer Invalidation:** Buffer reallocation invalidates any raw pointers
 *          previously retrieved via @ref mlib_queue_peek.
 * @warning **Aliasing:** `elem` must not point into the internal buffer of `queue`.
 *
 * @note **Ownership:** Value semantics. Payload is copied into the queue; caller retains
 *       ownership of `elem`.
 * @note **Complexity:** Amortized O(1); worst-case O(N) when triggering buffer doubling.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_queue_push(mlib_queue_t *queue, const void *elem);

/**
 * @brief Removes the front element from the queue (dequeue).
 *
 * Invokes `free_fn` on the front element's slot address (if configured)
 * and advances the front cursor using bitwise mask wrapping.
 *
 * @param[in,out] queue Pointer to the target queue container.
 *
 * @return Status code indicating the outcome:
 *         - #MLIB_SUCCESS : Front element removed.
 *         - #MLIB_ERR_NULL_PARAM : `queue` pointer was `NULL`.
 *         - #MLIB_ERR_EMPTY : Queue contains zero elements.
 *
 * @warning **Pointer Invalidation:** Pointers to the dequeued element previously
 *          obtained via @ref mlib_queue_peek become invalid.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_queue_pop(mlib_queue_t *queue);

/**
 * @brief Retrieves a direct pointer to the element currently at the front of the queue.
 *
 * Inspects the oldest enqueued element without removing it.
 *
 * @param[in] queue Pointer to the queue container.
 *
 * @return Direct pointer to front element payload inside the ring buffer,
 *         or `NULL` if `queue == NULL` or empty.
 *
 * @warning **Pointer Invalidation & Lifetime:** The returned pointer references internal
 *          circular buffer storage. It remains valid ONLY until modifying calls
 *          (@ref mlib_queue_push, @ref mlib_queue_pop, @ref mlib_queue_clear).
 *
 * @note **Ownership:** Caller must NOT free the returned pointer.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access if no write occurs.
 */
void *mlib_queue_peek(const mlib_queue_t *queue);

/* ========================================================================== */
/* Capacity & State Inspection                                                */
/* ========================================================================== */

/**
 * @brief Retrieves the current number of elements stored in the queue.
 *
 * @param[in] queue Pointer to the queue instance. Can be `NULL`.
 *
 * @return Element count, or `0` if `queue == NULL`.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
size_t mlib_queue_size(const mlib_queue_t *queue);

/**
 * @brief Retrieves the total capacity (in elements) of the allocated ring buffer.
 *
 * Returns a value guaranteed to be a power of two (or 0 if uninitialized/NULL).
 *
 * @param[in] queue Pointer to the queue instance. Can be `NULL`.
 *
 * @return Total allocated slot count, or `0` if `queue == NULL`.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
size_t mlib_queue_capacity(const mlib_queue_t *queue);

/**
 * @brief Checks whether the queue currently contains zero elements.
 *
 * @param[in] queue Pointer to the queue instance. Can be `NULL`.
 *
 * @return `true` if `queue == NULL` or `size == 0`; `false` otherwise.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
bool mlib_queue_is_empty(const mlib_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_QUEUE_H */
