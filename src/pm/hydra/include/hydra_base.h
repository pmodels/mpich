/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_BASE_H_INCLUDED
#define HYDRA_BASE_H_INCLUDED

#include <stdio.h>
#include "mpibase.h"
#include "hydra_config.h"

#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if defined HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if defined HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if defined HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */

#if defined HAVE_STDARG_H
#include <stdarg.h>
#endif /* HAVE_STDARG_H */

#if defined HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#include <errno.h>
/* FIXME: Is time.h available everywhere? We should probably have
 * multiple timer options like MPICH2. */
#include <time.h>
#include "mpimem.h"

#if defined MAXHOSTNAMELEN
#define MAX_HOSTNAME_LEN MAXHOSTNAMELEN
#else
#define MAX_HOSTNAME_LEN 256
#endif /* MAXHOSTNAMELEN */

#if defined MANUAL_EXTERN_ENVIRON
extern char **environ;
#endif /* MANUAL_EXTERN_ENVIRON */

#define HYD_DEFAULT_PROXY_PORT 9899

typedef enum {
    HYD_SUCCESS = 0,
    HYD_NO_MEM,
    HYD_SOCK_ERROR,
    HYD_INVALID_PARAM,
    HYD_INTERNAL_ERROR
} HYD_Status;

#define HYD_STDOUT  (1)
#define HYD_STDIN   (2)

typedef unsigned short HYD_Event_t;

#define HYD_TMPBUF_SIZE (64 * 1024)
#define HYD_EXEC_ARGS 200

typedef struct HYD_Env {
    char *env_name;
    char *env_value;
    struct HYD_Env *next;
} HYD_Env_t;

typedef enum {
    HYD_ENV_PROP_UNSET,
    HYD_ENV_PROP_ALL,
    HYD_ENV_PROP_NONE,
    HYD_ENV_PROP_LIST
} HYD_Env_prop_t;

#if !defined COMPILER_ACCEPTS_VA_ARGS
#define HYDU_Error_printf MPIU_Error_printf
#elif defined COMPILER_ACCEPTS_FUNC && defined __LINE__
#define HYDU_Error_printf(...)                            \
    {                                                     \
	fprintf(stderr, "%s (%d): ", __func__, __LINE__); \
	MPIU_Error_printf(__VA_ARGS__);                   \
    }
#elif defined __FILE__ && defined __LINE__
#define HYDU_Error_printf(...)                            \
    {                                                     \
	fprintf(stderr, "%s (%d): ", __FILE__, __LINE__); \
	MPIU_Error_printf(__VA_ARGS__);                   \
    }
#else
#define HYDU_Error_printf(...)                  \
    {                                           \
	MPIU_Error_printf(__VA_ARGS__);         \
    }
#endif

#if !defined COMPILER_ACCEPTS_VA_ARGS
#define HYDU_Debug if (handle.debug) printf
#else
#define HYDU_Debug(...)                                 \
    {                                                   \
        if (handle.debug)                               \
            printf(__VA_ARGS__);                        \
    }
#endif /* COMPILER_ACCEPTS_VA_ARGS */

#if !defined ENABLE_DEBUG
#define HYDU_FUNC_ENTER()
#define HYDU_FUNC_EXIT()
#elif defined COMPILER_ACCEPTS_FUNC
#define HYDU_FUNC_ENTER()                                               \
    {                                                                   \
	HYDU_Debug("Entering function %s\n", __func__);                 \
    }
#define HYDU_FUNC_EXIT()                                               \
    {                                                                  \
	HYDU_Debug("Exiting function %s\n", __func__);                 \
    }
#else
#define HYDU_FUNC_ENTER()                                               \
    {                                                                   \
    }
#define HYDU_FUNC_EXIT()                                               \
    {                                                                  \
    }
#endif

#endif /* HYDRA_BASE_H_INCLUDED */
