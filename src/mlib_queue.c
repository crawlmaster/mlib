#include <mlib/mlib_queue.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MLIB_QUEUE_MIN_CAPACITY 8

struct mlib_queue {
	void	    *data;
	size_t	     head;
	size_t	     size;
	size_t	     capacity;
	size_t	     mask;
	size_t	     elem_size;
	mlib_free_fn free_fn;
};

/** Helper function that returns the next highest power of 2 of n
 * @exception if n is already a power of 2, the function returns n
 */
static inline size_t mlib_queue_round_pow2(size_t n)
{
	if (n <= MLIB_QUEUE_MIN_CAPACITY)
		return MLIB_QUEUE_MIN_CAPACITY;

	if ((n & (n - 1)) == 0)
		return n;

#if defined(__GNUC__) || defined(__clang__)
	int leading_zeros = __builtin_clzll((unsigned long long)n);
	if (unlikely(leading_zeros == 0))
		return 0;
	return (size_t)1 << (__LLONG_WIDTH__ - leading_zeros);
#else
	--n;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;
	return n + 1;
#endif
}

/**
 * @brief Helper function that checks if resizing a queue would overflow the memory
 * given its capacity and element size. It expects the capacity to be a power of two
 * @param capacity the queue's current capacity
 * @param elem_size the size of an element that the queue holds
 * @param out_capacity pointer that will store the new capacity if resizing is possible
 * @return `MLIB_ERR_ALLOC` if the queue cannot be resized;
 *         `MLIB_SUCCESS` if the queue can be resized
 */
static inline mlib_status_t mlib_queue_check_overflow(size_t  capacity,
						      size_t  elem_size,
						      size_t *out_capacity)
{
	size_t new_cap = 0;

	if (unlikely(capacity == 0))
		new_cap = MLIB_QUEUE_MIN_CAPACITY;
	else {
		new_cap = capacity << 1;
		if (unlikely(new_cap <= capacity))
			return MLIB_ERR_ALLOC;
	}

	if (unlikely(new_cap > SIZE_MAX / elem_size))
		return MLIB_ERR_ALLOC;

	if (out_capacity)
		*out_capacity = new_cap;

	return MLIB_SUCCESS;
}

/**
 * @brief Calculates wether the address stored in `elem` exists inside the
 * same memory space as the queue's data, thus being a part of the queue
 * @param queue a valid queue
 * @param elem the pointer to be checked
 * @return Returns the index of elem relative to the queue's internal layout
 * or -1 if elem does not belong inside the queue
 */
static inline ptrdiff_t mlib_queue_ptr_offset(const mlib_queue_t *queue,
					      const void	 *elem)
{
	if (unlikely(!elem || !queue->data))
		return -1;

	uintptr_t elem_byte = (uintptr_t)elem;
	uintptr_t data_start = (uintptr_t)queue->data;
	uintptr_t data_end = data_start + (queue->capacity * queue->elem_size);

	if (elem_byte >= data_start && elem_byte < data_end)
		return (ptrdiff_t)(elem_byte - data_start);

	return -1;
}

static inline void mlib_queue_reorder_after_realloc(mlib_queue_t *queue,
						    size_t	  old_capacity)
{
	if (queue->head == 0)
		return;

	char *data = (char *)queue->data;

	// we turn [E3 E4 | head-> E1 E2 | <> <> <> <> ]
	// into [?? ?? | head-> E1 E2 E3 E4 | <> <>]
	memmove(data + (old_capacity * queue->elem_size), data,
		queue->head * queue->elem_size);
}

/**
 * @brief Downsizes the queue's data buffer to half capacity when size drops to a quarter
 */
static inline void mlib_queue_shrink_if_needed(mlib_queue_t *queue)
{
	if (queue->capacity <= MLIB_QUEUE_MIN_CAPACITY)
		return;

	if (likely(queue->size > (queue->capacity >> 2)))
		return;

	size_t new_cap = queue->capacity >> 1;
	char  *smaller_data = malloc(new_cap * queue->elem_size);
	if (unlikely(!smaller_data))
		return;

	if (queue->size > 0) {
		size_t first_part = queue->capacity - queue->head;
		if (first_part >= queue->size) {
			memcpy(smaller_data,
			       (char *)queue->data +
				       (queue->head * queue->elem_size),
			       queue->size * queue->elem_size);
		} else {
			memcpy(smaller_data,
			       (char *)queue->data +
				       (queue->head * queue->elem_size),
			       first_part * queue->elem_size);
			memcpy(smaller_data + (first_part * queue->elem_size),
			       queue->data,
			       (queue->size - first_part) * queue->elem_size);
		}
	}

	free(queue->data);
	queue->data = smaller_data;
	queue->head = 0;
	queue->capacity = new_cap;
	queue->mask = new_cap - 1;
}

/**
 * @brief Creates a new queue by allocating memory for it on the heap
 * @param elem_size the size of a single element that will be stored
 * @param initial_capacity the initial queue capacity
 * @param free_fn an `mlib_free_fn` that will be called when freeing queue data
 * @return the address of the queue in memory, or `NULL` if the allocation fails
 * @important the `initial_capacity` will be rounded to the next highest power of 2
 * @important if the `initial_capacity` is too large, the function will return `NULL`
 */
mlib_queue_t *mlib_queue_create(size_t elem_size, size_t initial_capacity,
				mlib_free_fn free_fn)
{
	if (unlikely(elem_size == 0))
		return NULL;

	mlib_queue_t *queue = malloc(sizeof(*queue));
	if (unlikely(!queue))
		return NULL;

	size_t cap = 0;
	if (initial_capacity > 0) {
		cap = mlib_queue_round_pow2(initial_capacity);
		if (unlikely(cap == 0 || cap > SIZE_MAX / elem_size)) {
			free(queue);
			return NULL;
		}

		queue->data = malloc(cap * elem_size);
		if (unlikely(!queue->data)) {
			free(queue);
			return NULL;
		}
	} else
		queue->data = NULL;

	queue->head = 0;
	queue->size = 0;
	queue->capacity = cap;
	queue->mask = cap ? (cap - 1) : 0;
	queue->elem_size = elem_size;
	queue->free_fn = free_fn;

	return queue;
}

mlib_status_t mlib_queue_push(mlib_queue_t *queue, const void *elem)
{
	if (unlikely(!queue))
		return MLIB_ERR_NULL_PTR;

	if (unlikely(queue->size == queue->capacity)) {
		size_t	      new_cap = 0;
		mlib_status_t status = mlib_queue_check_overflow(
			queue->capacity, queue->elem_size, &new_cap);
		if (unlikely(status != MLIB_SUCCESS))
			return status;

		ptrdiff_t offset = mlib_queue_ptr_offset(queue, elem);

		void *bigger_data =
			realloc(queue->data, new_cap * queue->elem_size);
		if (unlikely(!bigger_data))
			return MLIB_ERR_ALLOC;

		queue->data = bigger_data;

		size_t old_cap = queue->capacity;
		mlib_queue_reorder_after_realloc(queue, old_cap);

		if (unlikely(offset >= 0)) {
			uintptr_t head_bytes = queue->head * queue->elem_size;
			char	 *base = (char *)queue->data;

			if ((uintptr_t)offset < head_bytes)
				elem = base + (old_cap * queue->elem_size) +
				       offset;
			else
				elem = base + offset;
		}

		queue->capacity = new_cap;
		queue->mask = new_cap - 1;
	}

	size_t tail = (queue->head + queue->size) & queue->mask;
	char  *dest = (char *)queue->data + (tail * queue->elem_size);

	if (likely(elem))
		memcpy(dest, elem, queue->elem_size);
	else
		memset(dest, 0, queue->elem_size);

	++queue->size;
	return MLIB_SUCCESS;
}

mlib_status_t mlib_queue_pop(mlib_queue_t *queue)
{
	if (unlikely(!queue))
		return MLIB_ERR_NULL_PTR;
	if (unlikely(queue->size == 0))
		return MLIB_ERR_EMPTY;

	if (queue->free_fn) {
		void *front_elem =
			(char *)queue->data + (queue->head * queue->elem_size);
		queue->free_fn(front_elem);
	}

	queue->head = (queue->head + 1) & queue->mask;
	--queue->size;

	mlib_queue_shrink_if_needed(queue);

	return MLIB_SUCCESS;
}

void *mlib_queue_peek(const mlib_queue_t *queue)
{
	if (unlikely(!queue || queue->size == 0))
		return NULL;

	return (char *)queue->data + (queue->head * queue->elem_size);
}

size_t mlib_queue_size(const mlib_queue_t *queue)
{
	return likely(queue) ? queue->size : 0;
}

size_t mlib_queue_capacity(const mlib_queue_t *queue)
{
	return likely(queue) ? queue->capacity : 0;
}

bool mlib_queue_is_empty(const mlib_queue_t *queue)
{
	return likely(queue) ? (queue->size == 0) : true;
}

void mlib_queue_clear(mlib_queue_t *queue)
{
	if (unlikely(!queue || queue->size == 0))
		return;

	if (queue->free_fn) {
		for (size_t i = 0; i < queue->size; ++i) {
			size_t idx = (queue->head + i) & queue->mask;
			void  *elem_ptr =
				(char *)queue->data + (idx * queue->elem_size);
			queue->free_fn(elem_ptr);
		}
	}

	queue->head = 0;
	queue->size = 0;
}

void mlib_queue_destroy(mlib_queue_t **queue)
{
	if (unlikely(!queue || !*queue))
		return;

	mlib_queue_clear(*queue);
	free((*queue)->data);
	free(*queue);
	*queue = NULL;
}

#ifdef __cplusplus
}
#endif
