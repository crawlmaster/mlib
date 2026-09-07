#include <mlib/mlib_stack.h>
#include <mlib/mlib_vector.h>
#include <stdlib.h>

struct mlib_stack {
	mlib_vector_t *vect;
};

mlib_stack_t *mlib_stack_create(size_t elem_size, size_t initial_capacity,
				mlib_free_fn free_fn)
{
	if (unlikely(elem_size == 0))
		return NULL;

	mlib_stack_t *stack = malloc(sizeof(*stack));
	if (unlikely(!stack))
		return NULL;

	stack->vect = mlib_vector_create(elem_size, initial_capacity, free_fn);
	if (unlikely(!stack->vect)) {
		free(stack);
		return NULL;
	}

	return stack;
}

mlib_status_t mlib_stack_push(mlib_stack_t *stack, const void *elem)
{
	return likely(stack) ? mlib_vector_push_back(stack->vect, elem) :
			       MLIB_ERR_NULL_PTR;
}

mlib_status_t mlib_stack_pop(mlib_stack_t *stack)
{
	return likely(stack) ? mlib_vector_pop_back(stack->vect) :
			       MLIB_ERR_NULL_PTR;
}

void *mlib_stack_peek(const mlib_stack_t *stack)
{
	return likely(stack) ? mlib_vector_back(stack->vect) : NULL;
}

size_t mlib_stack_size(const mlib_stack_t *stack)
{
	return likely(stack) ? mlib_vector_size(stack->vect) : 0;
}

size_t mlib_stack_capacity(const mlib_stack_t *stack)
{
	return likely(stack) ? mlib_vector_capacity(stack->vect) : 0;
}

bool mlib_stack_is_empty(const mlib_stack_t *stack)
{
	return likely(stack) ? mlib_vector_size(stack->vect) == 0 : true;
}

mlib_status_t mlib_stack_reserve(mlib_stack_t *stack, size_t new_capacity)
{
	return likely(stack) ? mlib_vector_reserve(stack->vect, new_capacity) :
			       MLIB_ERR_NULL_PTR;
}

mlib_status_t mlib_stack_shrink_to_fit(mlib_stack_t *stack)
{
	return likely(stack) ? mlib_vector_shrink_to_fit(stack->vect) :
			       MLIB_ERR_NULL_PTR;
}

void mlib_stack_clear(mlib_stack_t *stack)
{
	if (likely(stack))
		mlib_vector_clear(stack->vect);
}

void mlib_stack_destroy(mlib_stack_t **stack)
{
	if (unlikely(!stack || !*stack))
		return;

	mlib_vector_destroy(&((*stack)->vect));
	free(*stack);
	*stack = NULL;
}
