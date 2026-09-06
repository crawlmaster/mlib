#include <mlib/mlib_sll.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mlib_sll_node {
	void		     *data;
	struct mlib_sll_node *next;
};

struct mlib_sll {
	mlib_sll_node_t *sentinel;
	mlib_sll_node_t *tail;
	size_t		 size;
	mlib_free_fn	 free_fn;
};

struct mlib_sll_iter {
	mlib_sll_node_t	 *it;
	const mlib_sll_t *list;
};

mlib_sll_t *mlib_sll_create(mlib_free_fn free_fn)
{
	mlib_sll_t *list = malloc(sizeof(*list));
	if (unlikely(!list))
		return NULL;

	list->sentinel = malloc(sizeof(*list->sentinel));
	if (unlikely(!list->sentinel)) {
		free(list);
		return NULL;
	}

	list->sentinel->data = NULL;
	list->sentinel->next = NULL;
	list->tail = list->sentinel;
	list->size = 0;
	list->free_fn = free_fn;

	return list;
}

mlib_status_t mlib_sll_insert_head(mlib_sll_t *list, void *data)
{
	if (unlikely(!list))
		return MLIB_ERR_NULL_PTR;

	mlib_sll_node_t *new = malloc(sizeof(*new));
	if (unlikely(!new))
		return MLIB_ERR_ALLOC;

	new->data = data;
	new->next = list->sentinel->next;
	list->sentinel->next = new;

	if (list->tail == list->sentinel)
		list->tail = new;

	++list->size;
	return MLIB_SUCCESS;
}

mlib_status_t mlib_sll_insert_tail(mlib_sll_t *list, void *data)
{
	if (unlikely(!list))
		return MLIB_ERR_NULL_PTR;

	mlib_sll_node_t *new = malloc(sizeof(*new));
	if (unlikely(!new))
		return MLIB_ERR_ALLOC;

	new->data = data;
	new->next = NULL;

	list->tail->next = new;
	list->tail = new;

	++list->size;
	return MLIB_SUCCESS;
}

mlib_status_t mlib_sll_insert_at(mlib_sll_t *list, size_t index, void *data)
{
	if (unlikely(!list))
		return MLIB_ERR_NULL_PTR;

	if (index == 0)
		return mlib_sll_insert_head(list, data);
	if (unlikely(index >= list->size))
		return mlib_sll_insert_tail(list, data);

	mlib_sll_node_t *new = malloc(sizeof(*new));
	if (unlikely(!new))
		return MLIB_ERR_ALLOC;
	new->data = data;

	mlib_sll_node_t *iter = list->sentinel;
	for (size_t i = 0; i < index; ++i)
		iter = iter->next;

	new->next = iter->next;
	iter->next = new;
	++list->size;
	return MLIB_SUCCESS;
}

mlib_status_t mlib_sll_remove_head(mlib_sll_t *list)
{
	if (unlikely(!list))
		return MLIB_ERR_NULL_PTR;
	if (unlikely(list->size == 0))
		return MLIB_ERR_EMPTY;

	mlib_sll_node_t *head = list->sentinel->next;
	list->sentinel->next = head->next;

	if (list->tail == head)
		list->tail = list->sentinel;

	if (list->free_fn && head->data)
		list->free_fn(head->data);
	free(head);
	--list->size;
	return MLIB_SUCCESS;
}

mlib_status_t mlib_sll_remove_tail(mlib_sll_t *list)
{
	if (unlikely(!list))
		return MLIB_ERR_NULL_PTR;
	if (unlikely(list->size == 0))
		return MLIB_ERR_EMPTY;

	mlib_sll_node_t *iter = list->sentinel;
	while (iter->next != list->tail)
		iter = iter->next;

	if (list->free_fn && list->tail->data)
		list->free_fn(list->tail->data);
	free(list->tail);

	iter->next = NULL;
	list->tail = iter;
	--list->size;
	return MLIB_SUCCESS;
}

mlib_status_t mlib_sll_remove_at(mlib_sll_t *list, size_t index)
{
	if (unlikely(!list))
		return MLIB_ERR_NULL_PTR;
	if (unlikely(list->size == 0))
		return MLIB_ERR_EMPTY;
	if (unlikely(index >= list->size))
		return MLIB_ERR_OUT_OF_BOUNDS;
	if (index == 0)
		return mlib_sll_remove_head(list);
	if (index == list->size - 1)
		return mlib_sll_remove_tail(list);

	mlib_sll_node_t *iter = list->sentinel;
	for (size_t i = 0; i < index; ++i)
		iter = iter->next;

	mlib_sll_node_t *remove = iter->next;
	iter->next = remove->next;

	if (list->free_fn && remove->data)
		list->free_fn(remove->data);
	free(remove);
	--list->size;
	return MLIB_SUCCESS;
}

size_t mlib_sll_size(const mlib_sll_t *list)
{
	return list ? list->size : 0;
}

mlib_status_t mlib_sll_find(const mlib_sll_t *list, const void *data,
			    mlib_compar_fn comp, size_t *out_index)
{
	if (unlikely(!list || !comp))
		return MLIB_ERR_NULL_PTR;

	mlib_sll_node_t *iter = list->sentinel->next;
	for (size_t i = 0; i < list->size && iter != NULL;
	     ++i, iter = iter->next) {
		if (comp(data, iter->data) == 0) {
			if (out_index)
				*out_index = i;
			return MLIB_SUCCESS;
		}
	}

	return MLIB_ERR_NOT_FOUND;
}

mlib_status_t mlib_sll_foreach(mlib_sll_t *list, mlib_callback_fn cb,
			       void *user_data)
{
	if (unlikely(!list || !cb))
		return MLIB_ERR_NULL_PTR;

	mlib_sll_node_t *iter = list->sentinel->next;
	for (size_t i = 0; i < list->size && iter != NULL;
	     ++i, iter = iter->next)
		cb(&iter->data, user_data);

	return MLIB_SUCCESS;
}

mlib_sll_iter_t *mlib_sll_head(const mlib_sll_t *list)
{
	if (unlikely(!list))
		return NULL;

	mlib_sll_iter_t *iter = malloc(sizeof(*iter));
	if (unlikely(!iter))
		return NULL;

	iter->it = list->sentinel->next;
	iter->list = list;
	return iter;
}

mlib_sll_iter_t *mlib_sll_tail(const mlib_sll_t *list)
{
	if (unlikely(!list))
		return NULL;

	mlib_sll_iter_t *iter = malloc(sizeof(*iter));
	if (unlikely(!iter))
		return NULL;

	iter->it = (list->tail == list->sentinel) ? NULL : list->tail;
	iter->list = list;
	return iter;
}

mlib_sll_iter_t *mlib_sll_at(const mlib_sll_t *list, size_t index)
{
	if (unlikely(!list || index >= list->size))
		return NULL;

	if (index == 0)
		return mlib_sll_head(list);
	if (index == list->size - 1)
		return mlib_sll_tail(list);

	mlib_sll_iter_t *iter = malloc(sizeof(*iter));
	if (unlikely(!iter))
		return NULL;

	mlib_sll_node_t *iter_node = list->sentinel->next;
	for (size_t i = 0; i < index && iter_node != NULL; ++i)
		iter_node = iter_node->next;

	iter->it = iter_node;
	iter->list = list;
	return iter;
}

void *mlib_sll_iter_data(const mlib_sll_iter_t *iter)
{
	if (unlikely(!iter || !iter->list || !iter->it))
		return NULL;

	return iter->it->data;
}

void mlib_sll_iter_destroy(mlib_sll_iter_t **iter)
{
	if (unlikely(!iter || !*iter))
		return;

	free(*iter);
	*iter = NULL;
}

bool mlib_sll_has_next(const mlib_sll_iter_t *iter)
{
	if (!iter || !iter->list || !iter->it)
		return false;

	return iter->it != NULL;
}

void mlib_sll_iter_next(mlib_sll_iter_t *iter)
{
	if (!iter || !iter->list || !iter->it)
		return;

	iter->it = iter->it->next;
}

void mlib_sll_clear(mlib_sll_t *list)
{
	if (unlikely(!list))
		return;

	mlib_sll_node_t *current = list->sentinel->next;
	while (current != NULL) {
		mlib_sll_node_t *next = current->next;

		if (list->free_fn && current->data)
			list->free_fn(current->data);

		free(current);
		current = next;
	}

	list->sentinel->next = NULL;
	list->tail = list->sentinel;
	list->size = 0;
}

void mlib_sll_destroy(mlib_sll_t **list)
{
	if (unlikely(!list || !*list))
		return;

	mlib_sll_clear(*list);

	free((*list)->sentinel);
	free(*list);
	*list = NULL;
}

#ifdef __cplusplus
}
#endif
