#ifndef MLIB_STACK_H
#define MLIB_STACK_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mlib_stack mlib_stack_t;

/**
 * @brief Allocates and initializes a new LIFO stack instance.
 *
 * @param elem_size Size in bytes of each element (must be > 0).
 * @param initial_capacity Initial pre-allocated capacity (0 for lazy allocation).
 * @param free_fn Optional destructor callback for releasing element-held resources, or NULL.
 * @return Pointer to the newly allocated stack instance, or NULL on allocation failure or invalid parameters.
 */
mlib_stack_t *mlib_stack_create(size_t elem_size, size_t initial_capacity,
				mlib_free_fn free_fn);

/**
 * @brief Pushes an element onto the top of the stack.
 *
 * @param stack Pointer to the stack instance.
 * @param elem Pointer to the data to copy into the stack (if NULL, slot is zero-filled).
 * @return `MLIB_SUCCESS` on success, `MLIB_ERR_NULL_PTR` if stack is NULL, or `MLIB_ERR_ALLOC` on memory allocation failure.
 */
mlib_status_t mlib_stack_push(mlib_stack_t *stack, const void *elem);

/**
 * @brief Removes the top element from the stack, invoking the configured free_fn if present.
 *
 * @param stack Pointer to the stack instance.
 * @return `MLIB_SUCCESS` on success, `MLIB_ERR_NULL_PTR` if stack is NULL, or `MLIB_ERR_EMPTY` if the stack contains no elements.
 */
mlib_status_t mlib_stack_pop(mlib_stack_t *stack);

/**
 * @brief Retrieves a pointer to the element currently at the top of the stack without removing it.
 *
 * @param stack Pointer to the stack instance.
 * @return Pointer to the top element, or NULL if the stack is NULL or empty.
 */
void *mlib_stack_peek(const mlib_stack_t *stack);

/**
 * @brief Retrieves the current number of elements contained in the stack.
 *
 * @param stack Pointer to the stack instance.
 * @return Number of elements, or 0 if stack is NULL.
 */
size_t mlib_stack_size(const mlib_stack_t *stack);

/**
 * @brief Retrieves the current total allocated capacity of the stack.
 *
 * @param stack Pointer to the stack instance.
 * @return Total allocated capacity in elements, or 0 if stack is NULL.
 */
size_t mlib_stack_capacity(const mlib_stack_t *stack);

/**
 * @brief Checks whether the stack contains zero elements.
 *
 * @param stack Pointer to the stack instance.
 * @return true if the stack is NULL or empty, false otherwise.
 */
bool mlib_stack_is_empty(const mlib_stack_t *stack);

/**
 * @brief Pre-allocates storage for at least new_capacity elements.
 *
 * @param stack Pointer to the stack instance.
 * @param new_capacity Minimum required capacity.
 * @return `MLIB_SUCCESS` on success, `MLIB_ERR_NULL_PTR` if stack is NULL, or `MLIB_ERR_ALLOC` on allocation failure.
 */
mlib_status_t mlib_stack_reserve(mlib_stack_t *stack, size_t new_capacity);

/**
 * @brief Shrinks the allocated memory buffer down to match the exact number of elements in the stack.
 *
 * @param stack Pointer to the stack instance.
 * @return `MLIB_SUCCESS` on success, `MLIB_ERR_NULL_PTR` if stack is NULL, or `MLIB_ERR_ALLOC` on reallocation failure.
 */
mlib_status_t mlib_stack_shrink_to_fit(mlib_stack_t *stack);

/**
 * @brief Removes all elements from the stack, invoking free_fn on each element if configured.
 *
 * @param stack Pointer to the stack instance.
 */
void mlib_stack_clear(mlib_stack_t *stack);

/**
 * @brief Destroys the stack, frees all internal resources, and sets the passed pointer reference to NULL.
 *
 * @param stack Pointer to the stack reference to destroy.
 */
void mlib_stack_destroy(mlib_stack_t **stack);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_STACK_H */
