/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef _MPID_UCC_H_
#define _MPID_UCC_H_

#include "mpidimpl.h"
#include "mpid_ucc_pre.h"

#ifndef MPIDI_COMMON_UCC_COMM_MIN_SIZE
#define MPIDI_COMMON_UCC_COMM_MIN_SIZE 2
#endif

/* Condition statement for excluding communicators due to their type: */
#define MPIDI_COMMON_UCC_COMM_EXCL_COND_TYPE (comm_ptr->comm_kind != MPIR_COMM_KIND__INTRACOMM)
/* Condition statement for excluding communicators due to their size: */
#define MPIDI_COMMON_UCC_COMM_EXCL_COND_SIZE (comm_ptr->local_size < MPIDI_COMMON_UCC_COMM_MIN_SIZE)

/* Additional exclusion statement given by the device? */
#ifndef MPIDI_COMMON_UCC_COMM_EXCL_COND_DEV
/* If not, set this condition statement to false: */
#define MPIDI_COMMON_UCC_COMM_EXCL_COND_DEV (0)
#endif

#define MPIDI_COMMON_UCC_OUTPUT_FORMAT "%s: (%s:%d)"
#define MPIDI_COMMON_UCC_OUTPUT_PARAMS __FILE__, __FUNCTION__, __LINE__
#define MPIDI_COMMON_UCC_OUTPUT_STRINGIFY(_x) #_x

#define MPIDI_COMMON_UCC_VERBOSE_LEVELS(LEVEL) \
    LEVEL(NONE)                                \
    LEVEL(ERROR)                               \
    LEVEL(WARNING)                             \
    LEVEL(BASIC)                               \
    LEVEL(COMM)                                \
    LEVEL(DTYPE)                               \
    LEVEL(COLLOP)                              \
    LEVEL(FNCALL)                              \
    LEVEL(ALL)

#define MPIDI_COMMON_UCC_VERBOSE_ENUM(ENUM) MPIDI_COMMON_UCC_VERBOSE_LEVEL_ ## ENUM,
typedef enum {
    MPIDI_COMMON_UCC_VERBOSE_LEVELS(MPIDI_COMMON_UCC_VERBOSE_ENUM)
} MPIDI_common_ucc_verbose_levels_t;

extern const char *MPIDI_COMMON_UCC_VERBOSE_LEVEL_TO_STRING[];

#define MPIDI_COMMON_UCC_VERBOSE_STRING_TO_LEVEL(_str, _num) do {       \
        int __len = strlen(_str);                                       \
        int __exm = 0;                                                  \
        if (_str[__len ? __len - 1 : 0] == '!') {                       \
            __exm = 1;                                                  \
            __len--;                                                    \
        }                                                               \
        for (int __idx = 0; __idx <= MPIDI_COMMON_UCC_VERBOSE_LEVEL_ALL;\
             __idx++) {                                                 \
            if (strncasecmp(_str,                                       \
                       MPIDI_COMMON_UCC_VERBOSE_LEVEL_TO_STRING[__idx], \
                       __len) == 0) {                                   \
                _num = __exm ? (-1) * __idx : __idx;                    \
                break;                                                  \
            }                                                           \
        }                                                               \
    } while (0)

#define MPIDI_COMMON_UCC_ERROR(_format, ...) do {                       \
        mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_ERROR;                  \
        if (abs(MPIDI_common_ucc_priv.verbose_level) >=                 \
            MPIDI_COMMON_UCC_VERBOSE_LEVEL_ERROR) {                     \
            fprintf(stderr, "==%u== UCC ERROR in "                      \
                    MPIDI_COMMON_UCC_OUTPUT_FORMAT " | "                \
                    _format "\n", getpid(),                             \
                    MPIDI_COMMON_UCC_OUTPUT_PARAMS, ## __VA_ARGS__);    \
            mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_ERROR;              \
            (void)mpidi_ucc_err;                                        \
        }                                                               \
    } while (0)

#define MPIDI_COMMON_UCC_WARNING(_format, ...) do {                     \
        if (abs(MPIDI_common_ucc_priv.verbose_level) >=                 \
            MPIDI_COMMON_UCC_VERBOSE_LEVEL_WARNING) {                   \
            fprintf(stderr, "==%u== UCC WARNING in "                    \
                    MPIDI_COMMON_UCC_OUTPUT_FORMAT " | "                \
                    _format "\n", getpid(),                             \
                    MPIDI_COMMON_UCC_OUTPUT_PARAMS, ## __VA_ARGS__);    \
        }                                                               \
    } while (0)

#define MPIDI_COMMON_UCC_VERBOSE(_level, _format, ...) do {             \
        if ((MPIDI_common_ucc_priv.verbose_level >= _level) ||          \
            (MPIDI_common_ucc_priv.verbose_level == _level * (-1))) {   \
            fprintf(stderr, "==%u== UCC INFO %s in "                    \
                    MPIDI_COMMON_UCC_OUTPUT_FORMAT " | "                \
                    _format "\n", getpid(),                             \
                    MPIDI_COMMON_UCC_VERBOSE_LEVEL_TO_STRING[_level],   \
                    MPIDI_COMMON_UCC_OUTPUT_PARAMS, ## __VA_ARGS__);    \
        }                                                               \
    } while (0)

#ifndef MPIDI_COMMON_UCC_DEBUG
#define MPIDI_COMMON_UCC_DEBUG(...)
#endif

typedef struct {
    int ucc_enabled;            /* flag set during `MPIDI_common_ucc_enable()` to activate the UCC support in general */
    int ucc_initialized;        /* flag set when the UCC library has been initialized successfully */
    int verbose_level;          /* verbosity level of the UCC wrappers; set during `MPIDI_common_ucc_enable()` */
    int verbose_debug;          /* flag for activating the very verbose debugging mode; set during `MPIDI_common_ucc_enable()` */
    int progress_hook_id;       /* id as set by `MPIR_Progress_hook_register()` and needed for deregistering it again later */
    int comm_world_initialized; /* flag indicating whether UCC support has been initialized already for MPI_COMM_WORLD */
    ucc_lib_h ucc_lib;          /* handle for the UCC library itself after its initialization */
    ucc_lib_attr_t ucc_lib_attr;        /* handle for the attributes used when initializing the UCC library */
    ucc_context_h ucc_context;  /* handle for the single UCC context that we're currently using for all MPI communicators */
} MPIDI_common_ucc_priv_t;

extern MPIDI_common_ucc_priv_t MPIDI_common_ucc_priv;

#endif /*_MPID_UCC_H_*/
