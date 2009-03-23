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

#if defined HAVE_GETTIMEOFDAY
/* FIXME: Is time.h available everywhere? We should probably have
 * multiple timer options like MPICH2. */
#include <time.h>
#include <sys/time.h>
#endif /* HAVE_GETTIMEOFDAY */

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

#define HYD_STDOUT  (1)
#define HYD_STDIN   (2)

typedef unsigned short HYD_Event_t;

#define HYD_TMPBUF_SIZE (64 * 1024)
#define HYD_TMP_STRLEN  1024
#define HYD_NUM_TMP_STRINGS 200


/* Status information */
typedef enum {
    HYD_SUCCESS = 0,
    HYD_GRACEFUL_ABORT,
    HYD_NO_MEM,
    HYD_SOCK_ERROR,
    HYD_INVALID_PARAM,
    HYD_INTERNAL_ERROR
} HYD_Status;


/* Environment information */
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

typedef enum {
    HYD_BIND_UNSET,
    HYD_BIND_NONE,
    HYD_BIND_RR,
    HYD_BIND_BUDDY,
    HYD_BIND_PACK,
    HYD_BIND_USER
} HYD_Binding;

/* List of contiguous segments of processes on a partition */
struct HYD_Partition_segment {
    int start_pid;
    int proc_count;
    char **mapping;
    struct HYD_Partition_segment *next;
};

/* Executables on a partition */
struct HYD_Partition_exec {
    char *exec[HYD_NUM_TMP_STRINGS];
    int proc_count;
    HYD_Env_prop_t prop;
    HYD_Env_t *prop_env;
    struct HYD_Partition_exec *next;
};

/* Partition information */
struct HYD_Partition {
    char *name;
    char *user_bind_map;
    int total_proc_count;

    /* Segment list will contain one-pass of the hosts file */
    struct HYD_Partition_segment *segment_list;
    struct HYD_Partition_exec *exec_list;

    /* Spawn information: each partition can have one or more
     * proxies. For the time being, we only support one proxy per
     * partition, but this can be easily extended later. We will also
     * need to give different ports for the proxies to listen on in
     * that case. */
    int pid;
    int out;
    int err;
    int exit_status;
    char *proxy_args[HYD_NUM_TMP_STRINGS];      /* Full argument list */

    struct HYD_Partition *next;
};

struct HYD_Exec_info {
    int exec_proc_count;
    char *exec[HYD_NUM_TMP_STRINGS];

    /* Local environment */
    HYD_Env_t *user_env;
    HYD_Env_prop_t prop;
    HYD_Env_t *prop_env;

    struct HYD_Exec_info *next;
} *exec_info;


#define HYDU_ERR_POP(status, message)                                   \
    {                                                                   \
        if (status != HYD_SUCCESS && status != HYD_GRACEFUL_ABORT) {    \
            if (strcmp(message, ""))                                    \
                HYDU_Error_printf(message);                             \
            goto fn_fail;                                               \
        }                                                               \
        else if (status == HYD_GRACEFUL_ABORT) {                        \
            goto fn_exit;                                               \
        }                                                               \
    }

#define HYDU_ERR_SETANDJUMP(status, error, message)                     \
    {                                                                   \
        status = error;                                                 \
        if (status != HYD_SUCCESS && status != HYD_GRACEFUL_ABORT) {    \
            if (strcmp(message, ""))                                    \
                HYDU_Error_printf(message);                             \
            goto fn_fail;                                               \
        }                                                               \
        else if (status == HYD_GRACEFUL_ABORT) {                        \
            goto fn_exit;                                               \
        }                                                               \
    }

#define HYDU_ERR_CHKANDJUMP(status, chk, error, message)                \
    {                                                                   \
        if ((chk))                                                      \
            HYDU_ERR_SETANDJUMP(status, error, message);                \
    }

#define HYDU_ERR_SETANDJUMP1(status, error, message, arg1)              \
    {                                                                   \
        status = error;                                                 \
        if (status != HYD_SUCCESS && status != HYD_GRACEFUL_ABORT) {    \
            if (strcmp(message, ""))                                    \
                HYDU_Error_printf(message, arg1);                       \
            goto fn_fail;                                               \
        }                                                               \
        else if (status == HYD_GRACEFUL_ABORT) {                        \
            goto fn_exit;                                               \
        }                                                               \
    }

#define HYDU_ERR_SETANDJUMP2(status, error, message, arg1, arg2)        \
    {                                                                   \
        status = error;                                                 \
        if (status != HYD_SUCCESS && status != HYD_GRACEFUL_ABORT) {    \
            if (strcmp(message, ""))                                    \
                HYDU_Error_printf(message, arg1, arg2);                 \
            goto fn_fail;                                               \
        }                                                               \
        else if (status == HYD_GRACEFUL_ABORT) {                        \
            goto fn_exit;                                               \
        }                                                               \
    }


#if defined ENABLE_WARNINGS
#define HYDU_Warn_printf HYDU_Error_printf
#else
#define HYDU_Warn_printf(...) {}
#endif /* ENABLE_WARNINGS */

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
