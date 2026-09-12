/**
 * @file common.h
 * @brief Foundational definitions, error codes, compiler macros, and generic callbacks for mlib.
 *
 * This header provides core primitives shared across all container modules in the library,
 * including cross-compiler branch prediction hints, uniform operation status codes,
 * and canonical function pointer signatures for element lifecycle and traversal.
 */

#ifndef MLIB_COMMON_H
#define MLIB_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/* ========================================================================== */
/* Compiler Optimization & Utility Macros                                     */
/* ========================================================================== */

#if defined(__GNUC__) || defined(__clang__) || defined(DOXYGEN)
/**
 * @def likely(x)
 * @brief Branch prediction hint indicating that expression `x` is expected to evaluate to true.
 */
#define likely(x)   __builtin_expect(!!(x), 1)

/**
 * @def unlikely(x)
 * @brief Branch prediction hint indicating that expression `x` is expected to evaluate to false.
 */
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif

/**
 * @def MLIB_UNUSED(x)
 * @brief Suppresses unused parameter and variable compiler warnings across standards.
 */
#define MLIB_UNUSED(x) (void)(x)

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Status Codes                                                               */
/* ========================================================================== */

/**
 * @enum mlib_status_codes
 * @brief Standardized status codes returned by fallible mlib operations.
 */
enum mlib_status_codes {
        /** Operation completed successfully without errors. */
        MLIB_SUCCESS = 0,

        /** An illegal NULL pointer was supplied for a required handle or argument. */
        MLIB_ERR_NULL_PTR = 1,

        /** Alias for #MLIB_ERR_NULL_PTR ensuring API naming parity across headers. */
        MLIB_ERR_NULL_PARAM = 1,

        /** Memory allocation or reallocation via the system allocator failed. */
        MLIB_ERR_ALLOC = 2,

        /** Operation cannot proceed because the container contains zero elements. */
        MLIB_ERR_EMPTY = 3,

        /** An index or sub-range specification exceeded the container's valid bounds. */
        MLIB_ERR_OUT_OF_BOUNDS = 4,

        /** Requested element or lookup target was not located during search. */
        MLIB_ERR_NOT_FOUND = 5
} mlib_status_t;

/**
 * @typedef mlib_status_t
 * @brief Status code type representing the outcome of mlib operations.
 */
typedef enum mlib_status_codes mlib_status_t;

/* ========================================================================== */
/* Generic Callback Function Signatures                                       */
/* ========================================================================== */

/**
 * @brief Destructor callback signature for releasing dynamically allocated element payload.
 *
 * @param[in,out] data Pointer to the element memory to release.
 *                     In reference-based containers (e.g., SLL), this is the stored `void *` pointer.
 *                     In value-based containers (e.g., Vector, Stack, Queue), this points to the slot address.
 *
 * @note **Ownership:** The callback implementation is expected to free any nested heap resources
 *       referenced by `data`. It must not free the container's internal storage itself.
 * @note **Safety:** Implementations must be safe against receiving a `NULL` pointer if the container
 *       allows storing null references.
 */
typedef void (*mlib_free_fn)(void *data);

/**
 * @brief Comparison callback signature compatible with standard C library `qsort` semantics.
 *
 * @param[in] a Pointer to the first element to compare.
 * @param[in] b Pointer to the second element to compare.
 *
 * @return An integer indicating ordering:
 *         - `< 0`: Element `a` is strictly less than element `b`.
 *         - `== 0`: Element `a` is equivalent to element `b`.
 *         - `> 0`: Element `a` is strictly greater than element `b`.
 *
 * @note **Purity:** The comparator must be a pure function without side effects to maintain
 *       strict weak ordering invariants required by search and sort algorithms.
 */
typedef int (*mlib_compar_fn)(const void *a, const void *b);

/**
 * @brief Visitor callback signature executed during container traversal (e.g., `foreach`).
 *
 * @param[in,out] data      Pointer to the active element being visited.
 *                          In-place payload modifications are permitted.
 * @param[in,out] user_data Context pointer supplied by caller; forwarded unmodified.
 *                          Can be `NULL` if no external state is needed.
 *
 * @warning **Structural Invariance:** The callback must not invoke operations that alter
 *          container layout, length, or buffer capacity during traversal.
 */
typedef void (*mlib_callback_fn)(void *data, void *user_data);

/* ========================================================================== */
/* Generic Helper Functions                                                   */
/* ========================================================================== */

/**
 * @brief Helper destructor for value-based containers holding pointers to heap allocations.
 *
 * Designed specifically for containers storing pointer types by value (e.g., `mlib_vector_t`
 * storing `char *` or dynamically allocated structs). Dereferences `elem` as `void **`
 * and invokes `free(*elem)` if the dereferenced pointer is non-null.
 *
 * @param[in,out] elem Address of the memory slot containing the pointer to deallocate.
 *                     If `elem` or `*(void **)elem` is `NULL`, this is a safe no-op.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Safe if underlying heap allocations are not shared across threads.
 */
static inline void mlib_free_indirect(void *elem)
{
        if (elem && *(void **)elem)
                free(*(void **)elem);
}

#ifdef __cplusplus
}
#endif

#endif /* MLIB_COMMON_H */
