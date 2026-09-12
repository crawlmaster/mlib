/**
 * @file mlib_vector.h
 * @brief Contiguous dynamic array (vector) implementation with value semantics.
 *
 * This module implements an opaque, cache-friendly dynamic array storing elements
 * by value using fixed-size memory blocks (`elem_size`). Geometric buffer growth
 * guarantees amortized constant-time push operations. The container supports indexed
 * random access, shifting insertions/deletions, in-place sorting, element reversal,
 * and O(1) unordered deletions via swap-remove.
 */

#ifndef MLIB_VECTOR_H
#define MLIB_VECTOR_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct mlib_vector
 * @brief Main dynamic array container handle (opaque).
 */
typedef struct mlib_vector mlib_vector_t;

/* ========================================================================== */
/* Lifecycle Management                                                       */
/* ========================================================================== */

/**
 * @brief Allocates and initializes an empty dynamic vector.
 *
 * @pre `elem_size > 0`
 * @pre `initial_capacity <= SIZE_MAX / elem_size` (multiplication must not overflow).
 *
 * @param[in] elem_size        Size in bytes of each stored element. Must be non-zero.
 * @param[in] initial_capacity Initial element slot count to pre-allocate.
 *                             Pass `0` to enable lazy buffer allocation.
 * @param[in] free_fn          Optional destructor callback invoked on each element's
 *                             memory address before element destruction, truncation,
 *                             clearing, or container destruction. Pass `NULL` if elements
 *                             are plain-old-data (POD) or do not manage dynamic resources.
 *
 * @return Pointer to the allocated `mlib_vector_t` container on success,
 *         or `NULL` if `elem_size == 0`, total bytes overflow `SIZE_MAX`,
 *         or heap allocation fails.
 *
 * @note **Ownership:** The library allocates and owns both the container handle
 *       and the underlying contiguous data buffer.
 * @note **Complexity:** O(1) if `initial_capacity == 0`, otherwise O(initial_capacity).
 * @note **Thread Safety:** Thread-safe for invocation across distinct instances.
 */
mlib_vector_t *mlib_vector_create(size_t elem_size, size_t initial_capacity,
				  mlib_free_fn free_fn);

/**
 * @brief Removes and destroys all stored elements, resetting size to 0.
 *
 * If `free_fn != NULL`, it is invoked once for each active element from index 0
 * up to `size - 1`. The allocated buffer capacity is retained for reuse.
 *
 * @param[in,out] vec Pointer to the vector instance.
 *                    If `vec == NULL` or the vector is already empty, this is a safe no-op.
 *
 * @warning **Pointer Invalidation:** Any pointers previously returned by @ref mlib_vector_get,
 *          @ref mlib_vector_front, @ref mlib_vector_back, or @ref mlib_vector_data
 *          are invalidated.
 *
 * @note **Complexity:** O(N) if `free_fn != NULL`; O(1) if `free_fn == NULL`.
 * @note **Thread Safety:** Not thread-safe on concurrent access to the same instance.
 */
void mlib_vector_clear(mlib_vector_t *vec);

/**
 * @brief Deallocates all resources, destroys the vector, and nullifies the caller's handle.
 *
 * Invokes @ref mlib_vector_clear, frees the internal data buffer and container structure,
 * and sets `*vect = NULL` to guard against use-after-free.
 *
 * @param[in,out] vect Double pointer to the vector handle.
 *                     Safe no-op if `vect == NULL` or `*vect == NULL`.
 *
 * @warning **Pointer Invalidation:** `*vect` is set to `NULL`; all element pointers
 *          and the container reference itself are permanently invalidated.
 *
 * @note **Ownership:** Deallocates all memory owned by the vector container.
 * @note **Complexity:** O(N) if `free_fn != NULL`; O(1) otherwise.
 * @note **Thread Safety:** Not thread-safe.
 */
void mlib_vector_destroy(mlib_vector_t **vect);

/* ========================================================================== */
/* Insertion Operations                                                       */
/* ========================================================================== */

/**
 * @brief Appends a new element to the end of the vector.
 *
 * Copies `elem_size` bytes from `elem` into the slot at `size`.
 * If capacity is exhausted, the buffer doubles in size (or initializes to 8 slots).
 *
 * @pre If `elem != NULL`, it must point to at least `elem_size` readable bytes.
 *
 * @param[in,out] vect Pointer to the target vector instance.
 * @param[in]     elem Pointer to the payload to append.
 *                     If `NULL`, the appended slot is zero-initialized.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Element successfully appended.
 *         - @ref MLIB_ERR_NULL_PTR: `vect` was `NULL`.
 *         - @ref MLIB_ERR_ALLOC: Buffer expansion failed due to memory exhaustion
 *                                or integer overflow. Vector remains unmodified.
 *
 * @warning **Pointer Invalidation:** If geometric growth triggers buffer reallocation,
 *          all raw element pointers previously retrieved are invalidated.
 *
 * @note **Aliasing & Self-Insertion:** Safe for self-insertion. If `elem` points directly
 *       inside the vector's current buffer, the internal byte offset is preserved
 *       even if reallocation moves the buffer.
 * @note **Ownership:** Value semantics. Payload is copied; caller retains ownership of `elem`.
 * @note **Complexity:** Amortized O(1); worst-case O(N) when triggering buffer reallocation.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_push_back(mlib_vector_t *vect, const void *elem);

/**
 * @brief Inserts an element at a specific 0-based index, shifting subsequent elements right.
 *
 * Elements at `[index, size - 1]` are shifted right by one position via `memmove`.
 * Inserting at `index == size` is equivalent to @ref mlib_vector_push_back.
 *
 * @pre `index <= mlib_vector_size(vect)`
 * @pre If `elem != NULL`, it must point to at least `elem_size` readable bytes.
 *
 * @param[in,out] vect  Pointer to the target vector instance.
 * @param[in]     index 0-based insertion position (`0` through `size`).
 * @param[in]     elem  Pointer to the payload to insert.
 *                      If `NULL`, the slot at `index` is zero-initialized.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Element successfully inserted.
 *         - @ref MLIB_ERR_NULL_PTR: `vect` was `NULL`.
 *         - @ref MLIB_ERR_OUT_OF_BOUNDS: `index > size`.
 *         - @ref MLIB_ERR_ALLOC: Buffer reallocation failed. Vector remains unmodified.
 *
 * @warning **Pointer Invalidation:** Reallocation invalidates all existing element pointers.
 *          Even without reallocation, any pointers to elements at or after `index` are
 *          shifted and should not be accessed via stale references.
 *
 * @note **Aliasing & Self-Insertion:** Safe for self-insertion. If `elem` points inside
 *       the vector, its relative offset and post-shift address are adjusted automatically.
 * @note **Ownership:** Value semantics. Payload is copied into the vector.
 * @note **Complexity:** O(N - index) without reallocation; O(N) if reallocation occurs.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_insert_at(mlib_vector_t *vect, size_t index,
				    const void *elem);

/* ========================================================================== */
/* Removal Operations                                                         */
/* ========================================================================== */

/**
 * @brief Removes the last element from the vector.
 *
 * If `free_fn != NULL`, it is called on the popped element before decreasing `size`.
 *
 * @param[in,out] vect Pointer to the target vector instance.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Last element removed.
 *         - @ref MLIB_ERR_NULL_PTR: `vect` was `NULL`.
 *         - @ref MLIB_ERR_EMPTY: Vector contains zero elements.
 *
 * @warning **Pointer Invalidation:** Any pointer referencing the popped back element
 *          is invalidated.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_pop_back(mlib_vector_t *vect);

/**
 * @brief Removes the element at the specified 0-based index, shifting subsequent elements left.
 *
 * If configured, `free_fn` is invoked on the element at `index`. Subsequent elements
 * from `index + 1` to `size - 1` are shifted left by one position to maintain order.
 *
 * @pre `index < mlib_vector_size(vect)`
 *
 * @param[in,out] vect  Pointer to the target vector instance.
 * @param[in]     index 0-based position to remove (`0` through `size - 1`).
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Element removed and gap closed.
 *         - @ref MLIB_ERR_NULL_PTR: `vect` was `NULL`.
 *         - @ref MLIB_ERR_EMPTY: Vector contains zero elements.
 *         - @ref MLIB_ERR_OUT_OF_BOUNDS: `index >= size`.
 *
 * @warning **Pointer Invalidation:** Pointers to elements at or after `index` are
 *          invalidated or relocated.
 *
 * @note **Order:** Preserves relative element order. For an O(1) removal where order
 *       does not matter, use @ref mlib_vector_swap_remove.
 * @note **Complexity:** O(N - index) where N is `size`. O(1) if `index == size - 1`.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_remove_at(mlib_vector_t *vect, size_t index);

/**
 * @brief Removes the element at `index` in constant time by swapping with the last element.
 *
 * Invokes `free_fn` on the element at `index`, moves the element at `size - 1`
 * into `index`, and decrements `size`. Does not preserve original element ordering.
 *
 * @pre `index < mlib_vector_size(vect)`
 *
 * @param[in,out] vect  Pointer to the target vector instance.
 * @param[in]     index 0-based position to remove (`0` through `size - 1`).
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Element removed via swap.
 *         - @ref MLIB_ERR_NULL_PTR: `vect` was `NULL`.
 *         - @ref MLIB_ERR_EMPTY: Vector contains zero elements.
 *         - @ref MLIB_ERR_OUT_OF_BOUNDS: `index >= size`.
 *
 * @warning **Pointer Invalidation:** Pointers to the element at `index` and the element
 *          at `size - 1` are invalidated or refer to swapped contents.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_swap_remove(mlib_vector_t *vect, size_t index);

/* ========================================================================== */
/* Element Access & Modification                                              */
/* ========================================================================== */

/**
 * @brief Retrieves a direct pointer to the element at a given 0-based index.
 *
 * @pre `index < mlib_vector_size(vect)`
 *
 * @param[in] vect  Pointer to the vector instance.
 * @param[in] index 0-based element position.
 *
 * @return Direct pointer to the element payload in the buffer,
 *         or `NULL` if `vect == NULL` or `index >= size`.
 *
 * @warning **Pointer Invalidation & Lifetime:** The returned pointer references internal
 *          storage and remains valid ONLY until modifying calls that alter buffer capacity
 *          or element positions (@ref mlib_vector_push_back, @ref mlib_vector_insert_at,
 *          @ref mlib_vector_remove_at, @ref mlib_vector_reserve, etc.).
 *
 * @note **Ownership:** Caller must NOT free the returned pointer.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
void *mlib_vector_get(const mlib_vector_t *vect, size_t index);

/**
 * @brief Replaces the element payload at a specified 0-based index.
 *
 * If `set_ptr == elem` (self-assignment), no action is taken. Otherwise, invokes
 * `free_fn` on the existing element at `index` (if configured) and copies `elem_size`
 * bytes from `elem` into the slot.
 *
 * @pre `index < mlib_vector_size(vect)`
 * @pre If `elem != NULL`, it must point to at least `elem_size` readable bytes.
 *
 * @param[in,out] vect  Pointer to the vector instance.
 * @param[in]     index 0-based target element position.
 * @param[in]     elem  Pointer to new payload to write.
 *                      If `NULL`, the slot at `index` is zero-initialized.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Slot payload replaced.
 *         - @ref MLIB_ERR_NULL_PTR: `vect` was `NULL`.
 *         - @ref MLIB_ERR_OUT_OF_BOUNDS: `index >= size`.
 *
 * @note **Aliasing:** Safe if `elem` points directly to the target slot (`set_ptr == elem`).
 * @note **Ownership:** Value semantics. Payload is copied into the vector.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_set(mlib_vector_t *vect, size_t index,
			      const void *elem);

/**
 * @brief Retrieves a direct pointer to the first element in the vector.
 *
 * @param[in] vect Pointer to the vector instance.
 *
 * @return Direct pointer to element 0, or `NULL` if `vect == NULL` or empty.
 *
 * @warning **Pointer Invalidation:** Valid only until subsequent buffer modifications.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
void *mlib_vector_front(const mlib_vector_t *vect);

/**
 * @brief Retrieves a direct pointer to the last element in the vector.
 *
 * @param[in] vect Pointer to the vector instance.
 *
 * @return Direct pointer to element `size - 1`, or `NULL` if `vect == NULL` or empty.
 *
 * @warning **Pointer Invalidation:** Valid only until subsequent buffer modifications.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
void *mlib_vector_back(const mlib_vector_t *vect);

/**
 * @brief Retrieves a direct pointer to the raw contiguous data buffer.
 *
 * Allows direct low-level access or interop with external C APIs (e.g. `qsort`, `fwrite`).
 *
 * @param[in] vect Pointer to the vector instance. Can be `NULL`.
 *
 * @return Direct pointer to internal storage array, or `NULL` if `vect == NULL`
 *         or no buffer has been allocated yet (`capacity == 0`).
 *
 * @warning **Pointer Invalidation:** Invalidated on buffer reallocation.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
void *mlib_vector_data(const mlib_vector_t *vect);

/* ========================================================================== */
/* Size & Capacity Inspection                                                 */
/* ========================================================================== */

/**
 * @brief Retrieves the current number of elements stored in the vector.
 *
 * @param[in] vec Pointer to the vector instance. Can be `NULL`.
 *
 * @return Element count, or `0` if `vec == NULL`.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
size_t mlib_vector_size(const mlib_vector_t *vec);

/**
 * @brief Retrieves the total capacity (in elements) allocated in the buffer.
 *
 * @param[in] vec Pointer to the vector instance. Can be `NULL`.
 *
 * @return Allocated slot count, or `0` if `vec == NULL`.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
size_t mlib_vector_capacity(const mlib_vector_t *vec);

/* ========================================================================== */
/* Memory & Capacity Management                                               */
/* ========================================================================== */

/**
 * @brief Pre-allocates buffer storage for at least `new_capacity` elements.
 *
 * If `new_capacity <= current_capacity`, the function performs no allocation.
 *
 * @pre `new_capacity <= SIZE_MAX / elem_size`
 *
 * @param[in,out] vec          Pointer to the vector instance.
 * @param[in]     new_capacity Minimum required capacity slot count.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Buffer reserved or was already large enough.
 *         - @ref MLIB_ERR_NULL_PTR: `vec` was `NULL`.
 *         - @ref MLIB_ERR_ALLOC: Memory reallocation failed or integer overflow occurred.
 *
 * @warning **Pointer Invalidation:** Buffer reallocation invalidates all existing element pointers.
 *
 * @note **Complexity:** O(N) where N is current `size` if reallocation occurs; O(1) otherwise.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_reserve(mlib_vector_t *vec, size_t new_capacity);

/**
 * @brief Resizes the vector to contain exactly `new_size` elements.
 *
 * If `new_size < size`, elements beyond `new_size` are destroyed via `free_fn` (if configured)
 * and `size` is truncated.
 * If `new_size > size`, capacity is expanded as needed, and newly appended slots
 * are zero-initialized.
 *
 * @pre `new_size <= SIZE_MAX / elem_size`
 *
 * @param[in,out] vec      Pointer to the vector instance.
 * @param[in]     new_size Desired element count.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Vector resized.
 *         - @ref MLIB_ERR_NULL_PTR: `vec` was `NULL`.
 *         - @ref MLIB_ERR_ALLOC: Reallocation failed or byte calculation overflowed.
 *
 * @warning **Pointer Invalidation:** Buffer reallocation or truncation invalidates
 *          affected element pointers.
 *
 * @note **Complexity:** O(N) where N is `new_size` or old `size`.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_resize(mlib_vector_t *vec, size_t new_size);

/**
 * @brief Shrinks the allocated memory buffer to match `size` exactly.
 *
 * Releases unused capacity back to the system. If `size == 0`, the data buffer
 * is completely freed and capacity becomes 0.
 *
 * @param[in,out] vec Pointer to the vector instance.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Buffer resized to match size (or already matched).
 *         - @ref MLIB_ERR_NULL_PTR: `vec` was `NULL`.
 *         - @ref MLIB_ERR_ALLOC: Buffer reallocation failed (state remains unchanged).
 *
 * @warning **Pointer Invalidation:** Buffer reallocation invalidates all existing element pointers.
 *
 * @note **Complexity:** O(N) where N is current `size`.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_shrink_to_fit(mlib_vector_t *vec);

/* ========================================================================== */
/* Search & Traversal                                                         */
/* ========================================================================== */

/**
 * @brief Searches linearly for the first element matching `target` using a comparator.
 *
 * Evaluates `comp(element_ptr, target)` sequentially. A match is found when `comp` returns 0.
 *
 * @param[in]  vec       Pointer to the vector instance.
 * @param[in]  target    Pointer to key/payload to match against.
 * @param[in]  comp      Comparison callback returning 0 on match (`int (*)(const void *, const void *)`).
 *                       Must not be `NULL`.
 * @param[out] out_index Optional pointer where the 0-based matching index is stored.
 *                       If not found, receives `size`. Pass `NULL` if not needed.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Match found; `*out_index` populated if provided.
 *         - @ref MLIB_ERR_NULL_PTR: `vec` or `comp` was `NULL`.
 *         - @ref MLIB_ERR_NOT_FOUND: No matching element found; `*out_index` set to `size`.
 *
 * @note **Complexity:** O(N) comparisons where N is `size`.
 * @note **Thread Safety:** Safe for concurrent read-only access if `comp` is pure.
 */
mlib_status_t mlib_vector_find(const mlib_vector_t *vec, const void *target,
			       mlib_compar_fn comp, size_t *out_index);

/**
 * @brief Invokes a visitor callback for each element in the vector from index 0 to `size - 1`.
 *
 * @param[in,out] vec       Pointer to the vector instance.
 * @param[in]     cb        Callback invoked as `cb(element_ptr, user_data)`. Must not be `NULL`.
 * @param[in,out] user_data Arbitrary pointer forwarded to `cb`. Can be `NULL`.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Traversal completed across all elements.
 *         - @ref MLIB_ERR_NULL_PTR: `vec` or `cb` was `NULL`.
 *
 * @warning **Structural Mutation:** The callback `cb` must not invoke modifying operations
 *          that alter capacity or size on `vec`. In-place mutations to element values are allowed.
 *
 * @note **Complexity:** O(N) invocations where N is `size`.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_foreach(mlib_vector_t *vec, mlib_callback_fn cb,
				  void *user_data);

/* ========================================================================== */
/* Algorithms & Transformations                                               */
/* ========================================================================== */

/**
 * @brief Sorts a contiguous sub-range of elements in-place using standard `qsort`.
 *
 * Sorts `n` elements starting at index `start`.
 *
 * @pre `n <= mlib_vector_size(vect)`
 * @pre `start <= mlib_vector_size(vect) - n` (range `[start, start + n)` must stay within bounds).
 *
 * @param[in,out] vect  Pointer to the vector instance.
 * @param[in]     start 0-based starting index of the sub-range.
 * @param[in]     n     Number of elements in the sub-range. If `n < 2`, no action is taken.
 * @param[in]     comp  Comparison callback returning negative, zero, or positive.
 *                      Must not be `NULL`.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Range sorted (or range had fewer than 2 elements).
 *         - @ref MLIB_ERR_NULL_PTR: `vect` or `comp` was `NULL`.
 *         - @ref MLIB_ERR_OUT_OF_BOUNDS: Specified range extends beyond `size`.
 *
 * @warning **Pointer Invalidation:** While buffer capacity does not change, individual
 *          element positions within `[start, start + n)` are reordered.
 *
 * @note **Complexity:** O(n log n) comparisons.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_sort_range(mlib_vector_t *vect, size_t start,
				     size_t n, mlib_compar_fn comp);

/**
 * @brief Sorts all elements in the vector in-place using standard `qsort`.
 *
 * Equivalent to calling `mlib_vector_sort_range(vect, 0, vect->size, comp)`.
 *
 * @param[in,out] vect Pointer to the vector instance.
 * @param[in]     comp Comparison callback returning negative, zero, or positive.
 *                     Must not be `NULL`.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Entire vector sorted.
 *         - @ref MLIB_ERR_NULL_PTR: `vect` or `comp` was `NULL`.
 *
 * @note **Complexity:** O(N log N) comparisons where N is `size`.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_sort(mlib_vector_t *vect, mlib_compar_fn comp);

/**
 * @brief Reverses the order of all elements in the vector in-place.
 *
 * Performs two-pointer swapping from both ends toward the center.
 * Automatically utilizes optimized 64-bit and 32-bit register-swap fast paths
 * if `elem_size` and alignment match word boundaries (8 or 4 bytes).
 *
 * @param[in,out] vect Pointer to the vector instance.
 *                     If `size < 2`, performs no operations.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Vector order reversed.
 *         - @ref MLIB_ERR_NULL_PTR: `vect` was `NULL`.
 *         - @ref MLIB_ERR_ALLOC: Fallback scratch buffer allocation failed
 *                                (only for `elem_size > 256` bytes).
 *
 * @warning **Pointer Invalidation:** Element positions are mirrored; pointers to specific
 *          indices now observe elements from their symmetric positions.
 *
 * @note **Complexity:** O(N) swaps where N is `size`.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_vector_reverse(mlib_vector_t *vect);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_VECTOR_H */
