/**
 * @file mlib.h
 * @brief Master umbrella header for the Modular C Data Structures Library (mlib).
 *
 * Including this header provides single-point access to the entire mlib API:
 * foundational definitions, Singly Linked List (SLL), Dynamic Array (Vector),
 * LIFO Stack, and Ring Buffer FIFO Queue. It also defines semantic versioning
 * macros and runtime version query utilities.
 */

#ifndef MLIB_H
#define MLIB_H

/* ========================================================================== */
/* Core Primitives & Data Structure Modules                                   */
/* ========================================================================== */

#include "common.h"
#include "mlib_sll.h"
#include "mlib_vector.h"
#include "mlib_stack.h"
#include "mlib_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Version Metadata Macros                                                    */
/* ========================================================================== */

/** Major version component (breaking API/ABI changes). */
#define MLIB_VERSION_MAJOR 0

/** Minor version component (backwards-compatible functionality additions). */
#define MLIB_VERSION_MINOR 8

/** Patch version component (backwards-compatible bug fixes and optimizations). */
#define MLIB_VERSION_PATCH 0

/**
 * @def MLIB_VERSION_CHECK(major, minor, patch)
 * @brief Macro for compile-time version compatibility testing.
 *
 * Evaluates to a monotonically increasing integer that can be used in
 * preprocessor directives (e.g., `#if MLIB_VERSION >= MLIB_VERSION_CHECK(0, 5, 0)`).
 */
#define MLIB_VERSION_CHECK(major, minor, patch) \
        (((major) << 16) | ((minor) << 8) | (patch))

/** Current library semantic version encoded as an integer. */
#define MLIB_VERSION \
        MLIB_VERSION_CHECK(MLIB_VERSION_MAJOR, MLIB_VERSION_MINOR, MLIB_VERSION_PATCH)

/** String literal representation of the current library semantic version. */
#define MLIB_VERSION_STRING "0.5.4"

/* ========================================================================== */
/* Runtime Version Utilities                                                  */
/* ========================================================================== */

/**
 * @brief Retrieves the compiled library version string at runtime.
 *
 * Facilitates verification that the linked binary or shared object
 * matches the header declarations.
 *
 * @return Null-terminated string literal adhering to Semantic Versioning (e.g. "0.5.4").
 *
 * @note **Ownership:** The returned pointer references a static string literal;
 *       caller must NOT attempt to modify or free it.
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Thread-safe (reentrant, read-only data).
 */
static inline const char *mlib_version_string(void)
{
        return MLIB_VERSION_STRING;
}

/**
 * @brief Retrieves the encoded version integer of the compiled library at runtime.
 *
 * Can be compared directly against @ref MLIB_VERSION_CHECK.
 *
 * @return 32-bit integer encoding `(major << 16) | (minor << 8) | patch`.
 *
 * @note **Complexity:** O(1)
 * @note **Thread Safety:** Thread-safe.
 */
static inline unsigned int mlib_version_number(void)
{
        return MLIB_VERSION;
}

#ifdef __cplusplus
}
#endif

#endif /* MLIB_H */
