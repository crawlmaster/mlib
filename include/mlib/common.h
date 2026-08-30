#ifndef MLIB_COMMON_H
#define MLIB_COMMON_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
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
typedef void (*mlib_callback_fn)(void **data, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* MLIB_COMMON_H */
