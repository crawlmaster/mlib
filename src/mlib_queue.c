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

static inline size_t mlib_queue_round_pow2(size_t n)
{
	if (n <= MLIB_QUEUE_MIN_CAPACITY)
		return MLIB_QUEUE_MIN_CAPACITY;

	if ((n & (n - 1)) == 0)
		return n;

#if defined(__GNUC__) || defined(__clang__)
	unsigned int leading_zeros = __builtin_clzll((unsigned long long)n);
	if (unlikely(leading_zeros == 0))
		return 0;
	return (size_t)1 << (64 - leading_zeros);
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

static inline mlib_status_t mlib_queue_check_overflow(const size_t capacity,
						      const size_t elem_size,
						      size_t	  *out_capacity)
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

static inline ptrdiff_t mlib_queue_ptr_offset(const void *elem,
					      const void *queue_data,
					      size_t capacity, size_t elem_size)
{
	if (unlikely(!elem || !queue_data))
		return -1;

	const char *elem_byte = (const char *)elem;
	const char *data_start = (const char *)queue_data;
	const char *data_end = data_start + (capacity * elem_size);

	if (elem_byte >= data_start && elem_byte < data_end)
		return elem_byte - data_start;

	return -1;
}

static inline void mlib_queue_reorder_after_realloc(mlib_queue_t *queue,
						    size_t	  old_capacity)
{
	if (queue->head == 0)
		return;

	char *data = (char *)queue->data;
	memmove(data + (old_capacity * queue->elem_size), data,
		queue->head * queue->elem_size);
}

/**
 * @brief Downsizes buffer to half capacity when size drops to a quarter.
 * Un-wraps elements linearly to base index 0 in the newly allocated buffer.
 */
static inline void mlib_queue_shrink_if_needed(mlib_queue_t *queue)
{
	if (queue->capacity <= MLIB_QUEUE_MIN_CAPACITY)
		return;

	if (queue->size > (queue->capacity >> 2))
		return;

	size_t new_capacity = queue->capacity >> 1;
	char  *new_data = malloc(new_capacity * queue->elem_size);
	if (unlikely(!new_data))
		return; /* Non-critical failure: retain current capacity */

	if (queue->size > 0) {
		size_t first_part = queue->capacity - queue->head;
		if (first_part >= queue->size) {
			memcpy(new_data,
			       (char *)queue->data +
				       (queue->head * queue->elem_size),
			       queue->size * queue->elem_size);
		} else {
			memcpy(new_data,
			       (char *)queue->data +
				       (queue->head * queue->elem_size),
			       first_part * queue->elem_size);
			memcpy(new_data + (first_part * queue->elem_size),
			       queue->data,
			       (queue->size - first_part) * queue->elem_size);
		}
	}

	free(queue->data);
	queue->data = new_data;
	queue->head = 0;
	queue->capacity = new_capacity;
	queue->mask = new_capacity - 1;
}

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
	} else {
		queue->data = NULL;
	}

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
		ptrdiff_t offset = mlib_queue_ptr_offset(
			elem, queue->data, queue->capacity, queue->elem_size);

		size_t	      new_cap = 0;
		mlib_status_t status = mlib_queue_check_overflow(
			queue->capacity, queue->elem_size, &new_cap);
		if (unlikely(status != MLIB_SUCCESS))
			return status;

		void *aux_data =
			realloc(queue->data, new_cap * queue->elem_size);
		if (unlikely(!aux_data))
			return MLIB_ERR_ALLOC;

		queue->data = aux_data;

		/* Re-adjust aliased pointer if memory reallocation shifted physical base */
		if (offset >= 0)
			elem = (const char *)queue->data + offset;

		size_t old_cap = queue->capacity;
		mlib_queue_reorder_after_realloc(queue, old_cap);

		/* If aliased element was in Segment B, it got relocated by reorder */
		if (offset >= 0 &&
		    offset < (ptrdiff_t)(queue->head * queue->elem_size))
			elem = (const char *)elem +
			       (old_cap * queue->elem_size);

		queue->capacity = new_cap;
		queue->mask = new_cap - 1;
	}

	size_t tail = (queue->head + queue->size) & queue->mask;
	char  *dest = (char *)queue->data + (tail * queue->elem_size);

	if (likely(elem != NULL))
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

	/* Shrink down with hysteresis to protect against thrashing */
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
	return likely(queue != NULL) ? queue->size : 0;
}

size_t mlib_queue_capacity(const mlib_queue_t *queue)
{
	return likely(queue != NULL) ? queue->capacity : 0;
}

bool mlib_queue_is_empty(const mlib_queue_t *queue)
{
	return likely(queue != NULL) ? (queue->size == 0) : true;
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
