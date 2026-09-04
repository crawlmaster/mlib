#ifndef MLIB_VECTOR_H
#define MLIB_VECTOR_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mlib_vector mlib_vector_t;

mlib_vector_t *mlib_vector_create(size_t elem_size, size_t initial_capacity,
				  mlib_free_fn free_fn);

mlib_status_t mlib_vector_push_back(mlib_vector_t *vect, const void *elem);
mlib_status_t mlib_vector_insert_at(mlib_vector_t *vect, size_t index,
				    const void *elem);

mlib_status_t mlib_vector_pop_back(mlib_vector_t *vect);
mlib_status_t mlib_vector_remove_at(mlib_vector_t *vect, size_t index);

void	     *mlib_vector_get(const mlib_vector_t *vect, size_t index);
mlib_status_t mlib_vector_set(mlib_vector_t *vect, size_t index,
			      const void *elem);

void *mlib_vector_front(const mlib_vector_t *vect);
void *mlib_vector_back(const mlib_vector_t *vect);

void  *mlib_vector_data(const mlib_vector_t *vec);
size_t mlib_vector_size(const mlib_vector_t *vec);
size_t mlib_vector_capacity(const mlib_vector_t *vec);

mlib_status_t mlib_vector_reserve(mlib_vector_t *vec, size_t new_capacity);
mlib_status_t mlib_vector_resize(mlib_vector_t *vec, size_t new_size);
mlib_status_t mlib_vector_shrink_to_fit(mlib_vector_t *vec);
void	      mlib_vector_clear(mlib_vector_t *vec);

mlib_status_t mlib_vector_find(const mlib_vector_t *vec, const void *target,
			       mlib_compar_fn comp, size_t *out_index);

mlib_status_t mlib_vector_foreach(mlib_vector_t *vec, mlib_callback_fn cb,
				  void *user_data);

void mlib_vector_destroy(mlib_vector_t **vect);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_VECTOR_H */
