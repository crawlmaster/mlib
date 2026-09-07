#ifndef MLIB_QUEUE_H
#define MLIB_QUEUE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mlib_queue mlib_queue_t;

/**
 * @brief Allocates and initializes a contiguous ring buffer queue (FIFO).
 *
 * The queue automatically rounds up initial_capacity to the nearest power of two
 * to allow single-cycle bitwise masking (& (capacity - 1)) instead of modulo division.
 *
 * @param elem_size Size in bytes of each element (must be > 0).
 * @param initial_capacity Desired initial capacity (rounded up to power of two, minimum 8).
 * @param free_fn Optional destructor callback for releasing element memory, or NULL.
 * @return Pointer to newly allocated queue instance, or NULL on failure.
 */
mlib_queue_t *mlib_queue_create(size_t elem_size, size_t initial_capacity,
				mlib_free_fn free_fn);

/**
 * @brief Inserts an element at the back of the queue (enqueue).
 *
 * If the queue is full, its capacity doubles (maintaining power-of-two alignment)
 * and contiguous un-wrapping is handled automatically.
 *
 * @param queue Pointer to the queue instance.
 * @param elem Pointer to data to copy (if NULL, slot is zero-filled).
 * @return MLIB_SUCCESS, MLIB_ERR_NULL_PTR, or MLIB_ERR_ALLOC.
 */
mlib_status_t mlib_queue_push(mlib_queue_t *queue, const void *elem);

/**
 * @brief Removes the front element from the queue (dequeue) and invokes free_fn if set.
 *
 * @param queue Pointer to the queue instance.
 * @return MLIB_SUCCESS, MLIB_ERR_NULL_PTR, or MLIB_ERR_EMPTY.
 */
mlib_status_t mlib_queue_pop(mlib_queue_t *queue);

/**
 * @brief Retrieves a pointer to the front element without removing it.
 *
 * @param queue Pointer to the queue instance.
 * @return Pointer to the front element, or NULL if empty or invalid.
 */
void *mlib_queue_peek(const mlib_queue_t *queue);

/**
 * @brief Retrieves the current number of elements stored in the queue.
 */
size_t mlib_queue_size(const mlib_queue_t *queue);

/**
 * @brief Retrieves the current total allocated capacity of the ring buffer.
 */
size_t mlib_queue_capacity(const mlib_queue_t *queue);

/**
 * @brief Checks if the queue contains zero elements.
 */
bool mlib_queue_is_empty(const mlib_queue_t *queue);

/**
 * @brief Clears all elements from the queue, invoking free_fn on each element.
 */
void mlib_queue_clear(mlib_queue_t *queue);

/**
 * @brief Destroys the queue, frees buffer memory, and zeroes the reference pointer.
 */
void mlib_queue_destroy(mlib_queue_t **queue);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_QUEUE_H */
