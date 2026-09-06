#include <mlib/mlib_vector.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mlib_vector {
	void	    *data;
	size_t	     size;
	size_t	     elem_size;
	size_t	     capacity;
	mlib_free_fn free_fn;
};

/* Helper to check if doubling capacity or buffer byte size causes integer overflow */
static inline mlib_status_t mlib_vector_check_overflow(size_t  capacity,
						       size_t  elem_size,
						       size_t *out_capacity)
{
	size_t new_cap = capacity ? (capacity << 1) : 8;

	if (new_cap < capacity || new_cap > SIZE_MAX / elem_size)
		return MLIB_ERR_ALLOC;

	if (out_capacity)
		*out_capacity = new_cap;

	return MLIB_SUCCESS;
}

/* Helper to check if elem points inside the vector buffer and compute its byte offset */
static inline ptrdiff_t mlib_vector_ptr_offset(const void *elem,
					       const void *vect_data,
					       size_t size, size_t elem_size)
{
	if (!elem || !vect_data)
		return -1;

	const char *elem_byte = (const char *)elem;
	const char *data_start = (const char *)vect_data;
	const char *data_end = data_start + (size * elem_size);

	if (elem_byte >= data_start && elem_byte < data_end)
		return elem_byte - data_start;

	return -1;
}

mlib_vector_t *mlib_vector_create(size_t elem_size, size_t initial_capacity,
				  mlib_free_fn free_fn)
{
	if (unlikely(elem_size == 0))
		return NULL;

	if (unlikely(initial_capacity > SIZE_MAX / elem_size))
		return NULL;

	mlib_vector_t *vect = malloc(sizeof(*vect));
	if (unlikely(!vect))
		return NULL;

	if (initial_capacity > 0) {
		vect->data = malloc(initial_capacity * elem_size);
		if (unlikely(!vect->data)) {
			free(vect);
			return NULL;
		}
	} else
		vect->data = NULL;

	vect->elem_size = elem_size;
	vect->capacity = initial_capacity;
	vect->size = 0;
	vect->free_fn = free_fn;

	return vect;
}

mlib_status_t mlib_vector_push_back(mlib_vector_t *vect, const void *elem)
{
	if (unlikely(!vect))
		return MLIB_ERR_NULL_PTR;

	if (unlikely(vect->size == vect->capacity)) {
		ptrdiff_t offset = mlib_vector_ptr_offset(
			elem, vect->data, vect->size, vect->elem_size);

		size_t new_cap = 0;

		mlib_status_t status = mlib_vector_check_overflow(
			vect->capacity, vect->elem_size, &new_cap);
		if (status != MLIB_SUCCESS)
			return status;

		void *aux_data = realloc(vect->data, new_cap * vect->elem_size);
		if (unlikely(!aux_data))
			return MLIB_ERR_ALLOC;

		vect->data = aux_data;
		vect->capacity = new_cap;

		if (offset >= 0)
			elem = (const char *)vect->data + offset;
	}

	void *push_ptr = (char *)vect->data + (vect->size * vect->elem_size);
	if (elem)
		memcpy(push_ptr, elem, vect->elem_size);
	else
		memset(push_ptr, 0, vect->elem_size);

	++vect->size;
	return MLIB_SUCCESS;
}

mlib_status_t mlib_vector_insert_at(mlib_vector_t *vect, size_t index,
				    const void *elem)
{
	if (unlikely(!vect))
		return MLIB_ERR_NULL_PTR;
	if (unlikely(index > vect->size))
		return MLIB_ERR_OUT_OF_BOUNDS;
	if (index == vect->size)
		return mlib_vector_push_back(vect, elem);

	ptrdiff_t offset = -1;
	if (elem)
		offset = mlib_vector_ptr_offset(elem, vect->data, vect->size,
						vect->elem_size);

	if (unlikely(vect->size == vect->capacity)) {
		size_t	      new_capacity = 0;
		mlib_status_t status = mlib_vector_check_overflow(
			vect->capacity, vect->elem_size, &new_capacity);
		if (unlikely(status != MLIB_SUCCESS))
			return status;

		void *aux_data =
			realloc(vect->data, new_capacity * vect->elem_size);
		if (unlikely(!aux_data))
			return MLIB_ERR_ALLOC;

		vect->data = aux_data;
		vect->capacity = new_capacity;
	}

	if (offset >= 0)
		elem = (const char *)vect->data + offset;

	char *data_it = (char *)vect->data;
	memmove(data_it + ((index + 1) * vect->elem_size),
		data_it + (index * vect->elem_size),
		(vect->size - index) * vect->elem_size);

	if (offset >= (ptrdiff_t)(index * vect->elem_size))
		elem = (const char *)elem + vect->elem_size;

	if (elem)
		memcpy(data_it + (index * vect->elem_size), elem,
		       vect->elem_size);
	else
		memset(data_it + (index * vect->elem_size), 0, vect->elem_size);

	++vect->size;
	return MLIB_SUCCESS;
}

mlib_status_t mlib_vector_pop_back(mlib_vector_t *vect)
{
	if (unlikely(!vect))
		return MLIB_ERR_NULL_PTR;
	if (vect->size == 0)
		return MLIB_ERR_EMPTY;

	--vect->size;
	if (vect->free_fn) {
		void *free_ptr =
			(char *)vect->data + (vect->size * vect->elem_size);
		vect->free_fn(free_ptr);
	}

	return MLIB_SUCCESS;
}

mlib_status_t mlib_vector_remove_at(mlib_vector_t *vect, size_t index)
{
	if (unlikely(!vect))
		return MLIB_ERR_NULL_PTR;
	if (vect->size == 0)
		return MLIB_ERR_EMPTY;
	if (index >= vect->size)
		return MLIB_ERR_OUT_OF_BOUNDS;

	if (index == vect->size - 1)
		return mlib_vector_pop_back(vect);

	char *data_it = (char *)vect->data + (index * vect->elem_size);
	if (vect->free_fn)
		vect->free_fn(data_it);

	size_t shift_size = (vect->size - index - 1) * vect->elem_size;
	memmove(data_it, data_it + vect->elem_size, shift_size);
	--vect->size;

	return MLIB_SUCCESS;
}

void *mlib_vector_get(const mlib_vector_t *vect, size_t index)
{
	if (unlikely(!vect || index >= vect->size))
		return NULL;

	return (char *)vect->data + (index * vect->elem_size);
}

mlib_status_t mlib_vector_set(mlib_vector_t *vect, size_t index,
			      const void *elem)
{
	if (unlikely(!vect))
		return MLIB_ERR_NULL_PTR;
	if (index >= vect->size)
		return MLIB_ERR_OUT_OF_BOUNDS;

	void *set_ptr = (char *)vect->data + (index * vect->elem_size);
	if (set_ptr == elem)
		return MLIB_SUCCESS;

	if (vect->free_fn)
		vect->free_fn(set_ptr);

	if (elem)
		memcpy(set_ptr, elem, vect->elem_size);
	else
		memset(set_ptr, 0, vect->elem_size);

	return MLIB_SUCCESS;
}

void *mlib_vector_front(const mlib_vector_t *vect)
{
	if (unlikely(!vect || vect->size == 0))
		return NULL;

	return vect->data;
}

void *mlib_vector_back(const mlib_vector_t *vect)
{
	if (unlikely(!vect || vect->size == 0))
		return NULL;

	return (char *)vect->data + ((vect->size - 1) * vect->elem_size);
}

void *mlib_vector_data(const mlib_vector_t *vect)
{
	if (unlikely(!vect))
		return NULL;

	return vect->data;
}

size_t mlib_vector_size(const mlib_vector_t *vect)
{
	if (unlikely(!vect))
		return 0;

	return vect->size;
}

size_t mlib_vector_capacity(const mlib_vector_t *vect)
{
	if (unlikely(!vect))
		return 0;

	return vect->capacity;
}

mlib_status_t mlib_vector_reserve(mlib_vector_t *vect, size_t new_capacity)
{
	if (unlikely(!vect))
		return MLIB_ERR_NULL_PTR;
	if (vect->capacity >= new_capacity)
		return MLIB_SUCCESS;

	if (unlikely(new_capacity > SIZE_MAX / vect->elem_size))
		return MLIB_ERR_ALLOC;

	void *aux_data = realloc(vect->data, new_capacity * vect->elem_size);
	if (unlikely(!aux_data))
		return MLIB_ERR_ALLOC;

	vect->data = aux_data;
	vect->capacity = new_capacity;
	return MLIB_SUCCESS;
}

mlib_status_t mlib_vector_resize(mlib_vector_t *vect, size_t new_size)
{
	if (unlikely(!vect))
		return MLIB_ERR_NULL_PTR;

	if (new_size == vect->size)
		return MLIB_SUCCESS;

	if (new_size < vect->size) {
		if (vect->free_fn) {
			for (size_t i = new_size; i < vect->size; ++i) {
				void *free_ptr = (char *)vect->data +
						 (i * vect->elem_size);
				vect->free_fn(free_ptr);
			}
		}
		vect->size = new_size;
		return MLIB_SUCCESS;
	}

	if (new_size > vect->capacity) {
		if (unlikely(new_size > SIZE_MAX / vect->elem_size))
			return MLIB_ERR_ALLOC;

		void *aux_data =
			realloc(vect->data, new_size * vect->elem_size);
		if (unlikely(!aux_data))
			return MLIB_ERR_ALLOC;

		vect->data = aux_data;
		vect->capacity = new_size;
	}

	void *empty_space_ptr =
		(char *)vect->data + (vect->size * vect->elem_size);
	size_t empty_space_size = (new_size - vect->size) * vect->elem_size;
	memset(empty_space_ptr, 0, empty_space_size);

	vect->size = new_size;
	return MLIB_SUCCESS;
}

mlib_status_t mlib_vector_shrink_to_fit(mlib_vector_t *vect)
{
	if (unlikely(!vect))
		return MLIB_ERR_NULL_PTR;

	if (vect->size == vect->capacity)
		return MLIB_SUCCESS;

	if (vect->size == 0) {
		free(vect->data);
		vect->data = NULL;
		vect->capacity = 0;
		return MLIB_SUCCESS;
	}

	void *aux_data = realloc(vect->data, vect->size * vect->elem_size);
	if (unlikely(!aux_data))
		return MLIB_ERR_ALLOC;

	vect->data = aux_data;
	vect->capacity = vect->size;
	return MLIB_SUCCESS;
}

void mlib_vector_clear(mlib_vector_t *vect)
{
	if (unlikely(!vect || !vect->size))
		return;

	if (vect->free_fn) {
		for (size_t i = 0; i < vect->size; ++i) {
			void *free_ptr =
				(char *)vect->data + (i * vect->elem_size);
			vect->free_fn(free_ptr);
		}
	}

	vect->size = 0;
}

mlib_status_t mlib_vector_find(const mlib_vector_t *vect, const void *target,
			       mlib_compar_fn comp, size_t *out_index)
{
	if (unlikely(!vect || !comp))
		return MLIB_ERR_NULL_PTR;
	if (vect->size == 0)
		return MLIB_ERR_EMPTY;

	for (size_t i = 0; i < vect->size; ++i) {
		void *data_ptr = (char *)vect->data + (i * vect->elem_size);
		if (!comp(data_ptr, target)) {
			if (out_index)
				*out_index = i;
			return MLIB_SUCCESS;
		}
	}

	if (out_index)
		*out_index = vect->size;
	return MLIB_ERR_NOT_FOUND;
}

mlib_status_t mlib_vector_foreach(mlib_vector_t *vect, mlib_callback_fn cb,
				  void *user_data)
{
	if (unlikely(!vect || !cb))
		return MLIB_ERR_NULL_PTR;
	if (vect->size == 0)
		return MLIB_SUCCESS;

	for (size_t i = 0; i < vect->size; ++i) {
		void *data_ptr = (char *)vect->data + (i * vect->elem_size);
		cb(data_ptr, user_data);
	}

	return MLIB_SUCCESS;
}

void mlib_vector_destroy(mlib_vector_t **vect)
{
	if (unlikely(!vect || !*vect))
		return;

	mlib_vector_clear(*vect);
	free((*vect)->data);
	free(*vect);
	*vect = NULL;
}

mlib_status_t mlib_vector_sort_range(mlib_vector_t *vect, size_t start,
				     size_t n, mlib_compar_fn comp)
{
	if (unlikely(!vect || !comp))
		return MLIB_ERR_NULL_PTR;

	if (unlikely(n > vect->size || start > vect->size - n))
		return MLIB_ERR_OUT_OF_BOUNDS;

	if (n < 2)
		return MLIB_SUCCESS;

	void *data_start = (char *)vect->data + (start * vect->elem_size);
	qsort(data_start, n, vect->elem_size,
	      (int (*)(const void *, const void *))comp);

	return MLIB_SUCCESS;
}

mlib_status_t mlib_vector_sort(mlib_vector_t *vect, mlib_compar_fn comp)
{
	return mlib_vector_sort_range(vect, 0, vect ? vect->size : 0, comp);
}

mlib_status_t mlib_vector_reverse(mlib_vector_t *vect)
{
	if (unlikely(!vect))
		return MLIB_ERR_NULL_PTR;
	if (vect->size < 2)
		return MLIB_SUCCESS;

	char *left = (char *)vect->data;
	char *right = (char *)vect->data + ((vect->size - 1) * vect->elem_size);
	size_t elem_size = vect->elem_size;

	/* Fast-path pentru pointeri și primitive (4 și 8 octeți) */
	if (elem_size == 8) {
		while (left < right) {
			uint64_t temp = *(uint64_t *)left;
			*(uint64_t *)left = *(uint64_t *)right;
			*(uint64_t *)right = temp;
			left += 8;
			right -= 8;
		}
		return MLIB_SUCCESS;
	} else if (elem_size == 4) {
		while (left < right) {
			uint32_t temp = *(uint32_t *)left;
			*(uint32_t *)left = *(uint32_t *)right;
			*(uint32_t *)right = temp;
			left += 4;
			right -= 4;
		}
		return MLIB_SUCCESS;
	}

	/* Fallback pentru structuri complexe */
	char  stack_buf[256];
	char *aux_buf = stack_buf;

	if (unlikely(elem_size > sizeof(stack_buf))) {
		aux_buf = malloc(elem_size);
		if (unlikely(!aux_buf))
			return MLIB_ERR_ALLOC;
	}

	while (left < right) {
		memcpy(aux_buf, left, elem_size);
		memcpy(left, right, elem_size);
		memcpy(right, aux_buf, elem_size);

		left += elem_size;
		right -= elem_size;
	}

	if (aux_buf != stack_buf)
		free(aux_buf);

	return MLIB_SUCCESS;
}

#ifdef __cplusplus
}
#endif
