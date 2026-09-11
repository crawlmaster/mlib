#ifndef MLIB_COMMON_H
#define MLIB_COMMON_H

#if defined(__GNUC__) || defined(__clang__)
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif

#define MLIB_UNUSED(x) (void)(x)

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum mlib_status_codes {
	MLIB_SUCCESS = 0,
	MLIB_ERR_NULL_PTR,
	MLIB_ERR_ALLOC,
	MLIB_ERR_EMPTY,
	MLIB_ERR_OUT_OF_BOUNDS,
	MLIB_ERR_NOT_FOUND
} mlib_status_t;

// Generic callback for freeing internal memory of a structure
typedef void (*mlib_free_fn)(void *data);

// Generic comparison callback function
typedef int (*mlib_compar_fn)(const void *a, const void *b);

// Generic callback function used in foreach
typedef void (*mlib_callback_fn)(void *data, void *user_data);

// Generic indirect freeing function for heap addresses stored in memory
static inline void mlib_free_indirect(void *elem)
{
	if (elem && *(void **)elem)
		free(*(void **)elem);
}

#ifdef __cplusplus
}
#endif

#endif /* MLIB_COMMON_H */
