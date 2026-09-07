#include <mlib/mlib_queue.h>
#include <stdlib.h>
#include <stdint.h>

struct mlib_queue {
	void	    *data;
	size_t	     head;
	size_t	     size;
	size_t	     elem_size;
	size_t	     capacity;
	size_t	     mask;
	mlib_free_fn free_fn;
};

/* Helper to check if doubling capacity or buffer byte size causes integer overflow */
static inline mlib_status_t mlib_queue_check_overflow(const size_t capacity,
						      const size_t elem_size,
						      size_t	  *out_capacity)
{
	size_t new_cap = capacity ? (capacity << 1) : 8;

	if (new_cap < capacity || new_cap > SIZE_MAX / elem_size)
		return MLIB_ERR_ALLOC;

	if (out_capacity)
		*out_capacity = new_cap;

	return MLIB_SUCCESS;
}

mlib_queue_t *mlib_queue_create(const size_t	   elem_size,
				const size_t	   initial_capacity,
				const mlib_free_fn free_fn)
{
	if (unlikely(elem_size == 0))
		return NULL;

	mlib_queue_t *queue = malloc(sizeof(*queue));
	if (unlikely(!queue))
		return NULL;

	if (likely(initial_capacity > 0)) {
		queue->data = malloc(elem_size * initial_capacity);
		if (unlikely(!queue->data)) {
			free(queue);
			return NULL;
		}
	} else
		queue->data = NULL;

	queue->size = 0;
	queue->head = 0;
	queue->mask = 0;
	queue->capacity = initial_capacity;
	queue->elem_size = elem_size;
	queue->free_fn = free_fn;
	return queue;
}

mlib_status_t mlib_queue_push(mlib_queue_t *queue, const void *elem)
{
	if (unlikely(!queue))
		return MLIB_ERR_NULL_PTR;

	if (unlikely(queue->capacity == queue->size)) {
		size_t new_capacity = 0;
		if (unlikely(mlib_queue_check_overflow(
				     queue->capacity, queue->elem_size,
				     &new_capacity) != MLIB_SUCCESS))
			return MLIB_ERR_ALLOC;
		void *aux_data =
			realloc(queue->data, new_capacity * queue->elem_size);
		if (unlikely(!aux_data))
			return MLIB_ERR_ALLOC;
	}
}

mlib_status_t mlib_queue_pop(mlib_queue_t *queue);

void *mlib_queue_peek(const mlib_queue_t *queue);

size_t mlib_queue_size(const mlib_queue_t *queue);

size_t mlib_queue_capacity(const mlib_queue_t *queue);

bool mlib_queue_is_empty(const mlib_queue_t *queue);

void mlib_queue_clear(mlib_queue_t *queue);

void mlib_queue_destroy(mlib_queue_t **queue);
