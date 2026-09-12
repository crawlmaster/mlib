/**
 * @file mlib_sll.h
 * @brief Singly Linked List (SLL) container with generic pointer storage.
 *
 * This module implements an opaque singly linked list storing payload elements
 * by reference (`void *`). Node links are unidirectional (head to tail).
 * Caches both head and tail pointers to guarantee constant-time push operations
 * at both ends.
 */

#ifndef MLIB_SLL_H
#define MLIB_SLL_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct mlib_sll_node
 * @brief Internal node representation holding pointer payload and next reference (opaque).
 */
typedef struct mlib_sll_node mlib_sll_node_t;

/**
 * @struct mlib_sll
 * @brief Singly linked list container handle (opaque).
 */
typedef struct mlib_sll mlib_sll_t;

/**
 * @struct mlib_sll_iter
 * @brief Forward traversal iterator referencing an active list node (opaque).
 */
typedef struct mlib_sll_iter mlib_sll_iter_t;

/* ========================================================================== */
/* Lifecycle Management                                                       */
/* ========================================================================== */

/**
 * @brief Allocates and initializes an empty singly linked list.
 *
 * @param[in] free_fn Optional destructor callback for releasing dynamic resources
 *                    held by payload pointers upon node removal or container destruction.
 *                    Pass `NULL` if elements are static or managed externally.
 *
 * @return Pointer to the allocated `mlib_sll_t` container on success,
 *         or `NULL` if heap allocation fails.
 *
 * @note **Ownership:** The library allocates and owns the `mlib_sll_t` header.
 *       Payload pointers stored later are owned collaboratively based on `free_fn`.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Thread-safe for invocation (independent instances).
 */
mlib_sll_t *mlib_sll_create(mlib_free_fn free_fn);

/**
 * @brief Removes and frees all nodes in the list, resetting size to 0.
 *
 * Iterates through every node, invokes `free_fn(node->data)` if `free_fn != NULL`,
 * deallocates internal node structures, and sets head and tail to `NULL`.
 *
 * @param[in,out] list Pointer to the target list container.
 *                     If `NULL`, this function is a safe no-op.
 *
 * @warning **Pointer Invalidation:** Any active iterators (`mlib_sll_iter_t`)
 *          referencing nodes in this list become invalid immediately.
 *
 * @note **Complexity:** O(N) where N is the current element count.
 * @note **Thread Safety:** Not thread-safe on concurrent access to the same list.
 */
void mlib_sll_clear(mlib_sll_t *list);

/**
 * @brief Deallocates all nodes, destroys the list container, and resets its handle.
 *
 * Calls @ref mlib_sll_clear to drain nodes and execute destructors, frees the
 * `mlib_sll_t` structure, and writes `NULL` to `*list`.
 *
 * @param[in,out] list Double pointer to the list container handle.
 *                     Safe no-op if `list == NULL` or `*list == NULL`.
 *
 * @warning **Pointer Invalidation:** The container handle `*list` is nullified;
 *          all node pointers, iterators, and the list handle become invalid.
 *
 * @note **Ownership:** Releases container memory owned by the library.
 * @note **Complexity:** O(N) where N is the element count.
 * @note **Thread Safety:** Not thread-safe on concurrent access.
 */
void mlib_sll_destroy(mlib_sll_t **list);

/* ========================================================================== */
/* Insertion Operations                                                       */
/* ========================================================================== */

/**
 * @brief Inserts a generic data pointer at the front (head) of the list.
 *
 * @param[in,out] list Pointer to the target list container.
 * @param[in]     data Generic payload pointer to store. `NULL` is allowed.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Element inserted successfully.
 *         - @ref MLIB_ERR_NULL_PARAM: `list` pointer was `NULL`.
 *         - @ref MLIB_ERR_ALLOC: Internal node allocation failed.
 *
 * @note **Ownership:** The list stores the raw `data` pointer. Caller retains
 *       ownership unless `free_fn` was supplied at list creation.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_sll_insert_head(mlib_sll_t *list, void *data);

/**
 * @brief Appends a generic data pointer to the end (tail) of the list.
 *
 * @param[in,out] list Pointer to the target list container.
 * @param[in]     data Generic payload pointer to store. `NULL` is allowed.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Element appended successfully.
 *         - @ref MLIB_ERR_NULL_PARAM: `list` pointer was `NULL`.
 *         - @ref MLIB_ERR_ALLOC: Internal node allocation failed.
 *
 * @note **Ownership:** Reference semantics. The raw pointer is stored as-is.
 * @note **Complexity:** O(1) due to tail pointer tracking.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_sll_insert_tail(mlib_sll_t *list, void *data);

/**
 * @brief Inserts a generic data pointer at a specified 0-based index.
 *
 * Inserting at index `0` delegates to @ref mlib_sll_insert_head.
 * Inserting at index equal to `size` delegates to @ref mlib_sll_insert_tail.
 *
 * @pre `index <= mlib_sll_size(list)`
 *
 * @param[in,out] list  Pointer to the target list container.
 * @param[in]     index 0-based insertion index (`0` through `size`).
 * @param[in]     data  Generic payload pointer to store. `NULL` is allowed.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Element placed at `index`.
 *         - @ref MLIB_ERR_NULL_PARAM: `list` pointer was `NULL`.
 *         - @ref MLIB_ERR_OUT_OF_BOUNDS: `index > size`.
 *         - @ref MLIB_ERR_ALLOC: Internal node allocation failed.
 *
 * @note **Complexity:** O(1) for `index == 0` or `index == size`; O(index) otherwise.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_sll_insert_at(mlib_sll_t *list, size_t index, void *data);

/* ========================================================================== */
/* Removal Operations                                                         */
/* ========================================================================== */

/**
 * @brief Removes the first element (head) from the list.
 *
 * Unlinks the head node, invokes `free_fn(head->data)` if configured,
 * and deallocates the unlinked node.
 *
 * @param[in,out] list Pointer to the target list container.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Head element removed.
 *         - @ref MLIB_ERR_NULL_PARAM: `list` pointer was `NULL`.
 *         - @ref MLIB_ERR_EMPTY: List contains zero elements.
 *
 * @warning **Pointer Invalidation:** Iterators referencing the removed head node
 *          become invalid immediately.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_sll_remove_head(mlib_sll_t *list);

/**
 * @brief Removes the last element (tail) from the list.
 *
 * In a singly linked list, unlinking the tail requires traversing to find
 * the node immediately preceding the tail.
 *
 * @param[in,out] list Pointer to the target list container.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Tail element removed.
 *         - @ref MLIB_ERR_NULL_PARAM: `list` pointer was `NULL`.
 *         - @ref MLIB_ERR_EMPTY: List contains zero elements.
 *
 * @warning **Pointer Invalidation:** Iterators referencing the removed tail node
 *          become invalid immediately.
 *
 * @note **Complexity:** O(N) where N is the current element count.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_sll_remove_tail(mlib_sll_t *list);

/**
 * @brief Removes the element at the specified 0-based index.
 *
 * Traverses to `index`, unlinks the node, calls `free_fn(node->data)` if set,
 * and deallocates node memory.
 *
 * @pre `index < mlib_sll_size(list)`
 *
 * @param[in,out] list  Pointer to the target list container.
 * @param[in]     index 0-based position to remove (`0` through `size - 1`).
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Node at `index` removed.
 *         - @ref MLIB_ERR_NULL_PARAM: `list` pointer was `NULL`.
 *         - @ref MLIB_ERR_EMPTY: List is empty.
 *         - @ref MLIB_ERR_OUT_OF_BOUNDS: `index >= size`.
 *
 * @warning **Pointer Invalidation:** Any iterator referencing the node at `index`
 *          is invalidated.
 *
 * @note **Complexity:** O(1) for `index == 0`; O(index) otherwise.
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_sll_remove_at(mlib_sll_t *list, size_t index);

/* ========================================================================== */
/* Inspection & Traversal                                                     */
/* ========================================================================== */

/**
 * @brief Retrieves the current number of elements stored in the list.
 *
 * @param[in] list Pointer to the list instance. Can be `NULL`.
 *
 * @return Number of elements stored, or `0` if `list == NULL`.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
size_t mlib_sll_size(const mlib_sll_t *list);

/**
 * @brief Searches for the first element matching `data` using a comparator callback.
 *
 * Linearly inspects stored pointers. For each element, evaluates `comp(node->data, data)`.
 * A match is established when `comp` returns `0`.
 *
 * @param[in]  list      Pointer to the list instance.
 * @param[in]  data      Key or payload pointer passed as the second argument to `comp`.
 * @param[in]  comp      Comparison callback (`int (*)(const void *, const void *)`).
 *                       Must not be `NULL`.
 * @param[out] out_index Optional pointer where the 0-based index of the matching node
 *                       is written. Pass `NULL` if only status check is desired.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Match found; `*out_index` populated if non-null.
 *         - @ref MLIB_ERR_NULL_PARAM: `list` or `comp` was `NULL`.
 *         - @ref MLIB_ERR_NOT_FOUND: Traversal completed without matching elements.
 *
 * @note **Complexity:** O(N) where N is the element count.
 * @note **Thread Safety:** Safe for concurrent read-only access if `comp` is pure.
 */
mlib_status_t mlib_sll_find(const mlib_sll_t *list, const void *data,
			    mlib_compar_fn comp, size_t *out_index);

/**
 * @brief Applies a visitor callback to each element payload in sequence from head to tail.
 *
 * @param[in,out] list      Pointer to the list instance.
 * @param[in]     cb        Callback invoked as `cb(node->data, user_data)`.
 *                          Must not be `NULL`.
 * @param[in,out] user_data Context pointer forwarded to `cb`. Can be `NULL`.
 *
 * @return Status code indicating the outcome:
 *         - @ref MLIB_SUCCESS: Callback executed over all nodes.
 *         - @ref MLIB_ERR_NULL_PARAM: `list` or `cb` was `NULL`.
 *
 * @warning **Structural Mutation:** The callback `cb` must not invoke structural
 *          mutation functions (`insert_*`, `remove_*`, `clear`) on `list`.
 *
 * @note **Complexity:** O(N)
 * @note **Thread Safety:** Not thread-safe.
 */
mlib_status_t mlib_sll_foreach(mlib_sll_t *list, mlib_callback_fn cb,
			       void *user_data);

/* ========================================================================== */
/* Iterator Interface                                                         */
/* ========================================================================== */

/**
 * @brief Creates a forward iterator positioned at the list head.
 *
 * @param[in] list Pointer to the list container.
 *
 * @return Allocated iterator referencing the head node, or `NULL` if `list`
 *         is `NULL`, empty, or heap allocation fails.
 *
 * @note **Ownership:** Caller owns the iterator and must free it via @ref mlib_sll_iter_destroy.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Thread-safe for distinct iterator handles.
 */
mlib_sll_iter_t *mlib_sll_head(const mlib_sll_t *list);

/**
 * @brief Creates an iterator positioned at the list tail.
 *
 * @param[in] list Pointer to the list container.
 *
 * @return Allocated iterator referencing the tail node, or `NULL` if `list`
 *         is `NULL`, empty, or heap allocation fails.
 *
 * @note **Ownership:** Caller owns the iterator and must free it via @ref mlib_sll_iter_destroy.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Thread-safe for distinct iterator handles.
 */
mlib_sll_iter_t *mlib_sll_tail(const mlib_sll_t *list);

/**
 * @brief Creates an iterator positioned at a given 0-based index.
 *
 * @pre `index < mlib_sll_size(list)`
 *
 * @param[in] list  Pointer to the list container.
 * @param[in] index 0-based target index position.
 *
 * @return Allocated iterator referencing node at `index`, or `NULL` if `list`
 *         is `NULL`, `index >= size`, or allocation fails.
 *
 * @note **Ownership:** Caller owns the iterator and must free it via @ref mlib_sll_iter_destroy.
 * @note **Complexity:** O(index)
 * @note **Thread Safety:** Thread-safe for distinct iterator handles.
 */
mlib_sll_iter_t *mlib_sll_at(const mlib_sll_t *list, size_t index);

/**
 * @brief Retrieves the generic data pointer stored at the current iterator position.
 *
 * @param[in] iter Pointer to the iterator instance.
 *
 * @return Raw pointer stored in the node, or `NULL` if `iter == NULL` or invalid.
 *
 * @note **Ownership:** Returns unmanaged pointer; caller must not free payload
 *       unless `free_fn` was `NULL`.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
void *mlib_sll_iter_data(const mlib_sll_iter_t *iter);

/**
 * @brief Tests whether the node referenced by the iterator has a successor.
 *
 * @param[in] iter Pointer to the iterator instance.
 *
 * @return `true` if `iter != NULL` and current node has a following node; `false` otherwise.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe for concurrent read-only access.
 */
bool mlib_sll_has_next(const mlib_sll_iter_t *iter);

/**
 * @brief Advances the iterator forward to the next node.
 *
 * If the iterator is at the tail node, calling this invalidates its position
 * (subsequent @ref mlib_sll_has_next returns `false`).
 *
 * @param[in,out] iter Pointer to the iterator instance. Safe no-op if `iter == NULL`.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Not thread-safe on concurrent access to the same iterator.
 */
void mlib_sll_iter_next(mlib_sll_iter_t *iter);

/**
 * @brief Destroys an iterator instance and nullifies its handle.
 *
 * Does not alter or free list nodes or payload data.
 *
 * @param[in,out] iter Double pointer to the iterator handle.
 *                     Safe no-op if `iter == NULL` or `*iter == NULL`.
 *
 * @note **Ownership:** Releases memory allocated for iterator structure.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Not thread-safe on concurrent access.
 */
void mlib_sll_iter_destroy(mlib_sll_iter_t **iter);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_SLL_H */
