#ifndef MLIB_SLL_H
#define MLIB_SLL_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mlib_sll_node mlib_sll_node_t;
typedef struct mlib_sll	     mlib_sll_t;
typedef struct mlib_sll_iter mlib_sll_iter_t;

mlib_sll_t *mlib_sll_create(mlib_free_fn free_fn);

mlib_status_t mlib_sll_insert_head(mlib_sll_t *list, void *data);
mlib_status_t mlib_sll_insert_tail(mlib_sll_t *list, void *data);
mlib_status_t mlib_sll_insert_at(mlib_sll_t *list, size_t index, void *data);

mlib_status_t mlib_sll_remove_head(mlib_sll_t *list);
mlib_status_t mlib_sll_remove_tail(mlib_sll_t *list);
mlib_status_t mlib_sll_remove_at(mlib_sll_t *list, size_t index);

size_t	      mlib_sll_size(const mlib_sll_t *list);
mlib_status_t mlib_sll_find(const mlib_sll_t *list, const void *data,
			    mlib_compar_fn comp, size_t *out_index);
mlib_status_t mlib_sll_foreach(mlib_sll_t *list, mlib_callback_fn cb,
			       void *user_data);

mlib_sll_iter_t *mlib_sll_head(const mlib_sll_t *list);
mlib_sll_iter_t *mlib_sll_tail(const mlib_sll_t *list);
mlib_sll_iter_t *mlib_sll_at(const mlib_sll_t *list, size_t index);
void		*mlib_sll_iter_data(const mlib_sll_iter_t *iter);
void		 mlib_sll_iter_destroy(mlib_sll_iter_t **iter);

bool mlib_sll_has_next(const mlib_sll_iter_t *iter);
void mlib_sll_iter_next(mlib_sll_iter_t *iter);

void mlib_sll_clear(mlib_sll_t *list);
void mlib_sll_destroy(mlib_sll_t **list);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_SLL_H */
