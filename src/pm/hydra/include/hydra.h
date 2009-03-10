/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_H_INCLUDED
#define HYDRA_H_INCLUDED

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

typedef enum {
    HYD_ENV_STATIC,
    HYD_ENV_AUTOINC
} HYD_Env_type_t;

typedef struct HYD_Env {
    char *env_name;
    char *env_value;

    /* Auto-incrementing environment variables can only be integers */
    HYD_Env_type_t env_type;
    int start_val;
    struct HYD_Env *next;
} HYD_Env_t;

typedef enum {
    HYD_ENV_PROP_UNSET,
    HYD_ENV_PROP_ALL,
    HYD_ENV_PROP_NONE,
    HYD_ENV_PROP_LIST
} HYD_Env_prop_t;

struct HYD_Handle_ {
    int debug;
    int enablex;
    char *wdir;

    char *host_file;

    /* Global environment */
    HYD_Env_t *global_env;
    HYD_Env_t *system_env;
    HYD_Env_t *user_env;
    HYD_Env_prop_t prop;
    HYD_Env_t *prop_env;

    int in;
     HYD_Status(*stdin_cb) (int fd, HYD_Event_t events);

    /* Start time and timeout. These are filled in by the launcher,
     * but are utilized by the demux engine and the boot-strap server
     * to decide how long we need to wait for. */
    struct timeval start;
    struct timeval timeout;

    /* Each structure will contain all hosts/cores that use the same
     * executable and environment. */
    struct HYD_Proc_params {
        int exec_proc_count;
        char *exec[HYD_EXEC_ARGS];

        struct HYD_Partition_list {
            char *name;
            int proc_count;
            char **mapping;     /* Can be core IDs or something else */

            /*
             * The boot-strap server is expected to start a single
             * executable on the first possible node and return a
             * single PID. This executable could be a PM proxy that
             * will launch the actual application on the rest of the
             * partition list.
             *
             * Possible hacks:
             *
             *   1. If the process manager needs more proxies within
             *      this same list, it can use different group
             *      IDs. Each group ID will have its own proxy.
             *
             *   2. If no proxy is needed, the PM can split this list
             *      into one element per process. The boot-strap
             *      server itself does not distinguish a proxy from
             *      the application executable, so it will not require
             *      any changes.
             *
             *   3. One proxy per physical node means that each
             *      partition will have a different group ID.
             */
            int group_id;       /* Assumed to be in ascending order */
            int group_rank;     /* Rank within the group */
            int pid;
            int out;
            int err;
            int exit_status;
            char *args[HYD_EXEC_ARGS];

            struct HYD_Partition_list *next;
        } *partition;

        /* Local environment */
        HYD_Env_t *user_env;
        HYD_Env_prop_t prop;
        HYD_Env_t *prop_env;

        /* Callback functions for the stdout/stderr events. These can
         * be the same. */
         HYD_Status(*stdout_cb) (int fd, HYD_Event_t events);
         HYD_Status(*stderr_cb) (int fd, HYD_Event_t events);

        struct HYD_Proc_params *next;
    } *proc_params;

    /* Random parameters used for internal code */
    int func_depth;
    char stdin_tmp_buf[HYD_TMPBUF_SIZE];
    int stdin_buf_offset;
    int stdin_buf_count;
};

typedef struct HYD_Handle_ HYD_Handle;

/* We'll use this as the central handle that has most of the
 * information needed by everyone. All data to be written has to be
 * done before the HYD_CSI_Wait_for_completion() function is called,
 * except for two exceptions:
 *
 * 1. The timeout value is initially added by the launcher before the
 * HYD_CSI_Wait_for_completion() function is called, but can be edited
 * by the control system within this call. There's no guarantee on
 * what value it will contain for the other layers.
 *
 * 2. There is no guarantee on what the exit status will contain till
 * the HYD_CSI_Wait_for_completion() function returns (where the
 * bootstrap server can fill out these values).
 */
extern HYD_Handle handle;

#if !defined COMPILER_ACCEPTS_VA_ARGS
#define HYDU_Print printf
#else
#define HYDU_Print(...)                         \
    {                                           \
        int i;                                  \
        for (i = 0; i < HYDU_Dbg_depth; i++)    \
            printf("  ");                       \
        printf(__VA_ARGS__);                    \
    }
#endif /* COMPILER_ACCEPTS_VA_ARGS */

#if !defined ENABLE_DEBUG
#define HYDU_FUNC_ENTER()
#define HYDU_FUNC_EXIT()
#elif defined COMPILER_ACCEPTS_FUNC
#define HYDU_FUNC_ENTER()                                               \
    {                                                                   \
	HYDU_Print("Entering function %s\n", __func__);                 \
        HYDU_Dbg_depth++;                                               \
    }
#define HYDU_FUNC_EXIT()                                               \
    {                                                                  \
	HYDU_Print("Exiting function %s\n", __func__);                 \
        HYDU_Dbg_depth--;                                              \
    }
#else
#define HYDU_FUNC_ENTER()                                               \
    {                                                                   \
        HYDU_Dbg_depth++;                                               \
    }
#define HYDU_FUNC_EXIT()                                               \
    {                                                                  \
        HYDU_Dbg_depth--;                                              \
    }
#endif

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

#endif /* HYDRA_H_INCLUDED */
