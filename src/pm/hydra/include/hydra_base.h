/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_BASE_H_INCLUDED
#define HYDRA_BASE_H_INCLUDED

#include <stdio.h>
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

#if defined HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#include <errno.h>

#if defined(HAVE_PUTENV) && defined(NEEDS_PUTENV_DECL)
extern int putenv(char *string);
#endif

#if defined(NEEDS_GETHOSTNAME_DECL)
int gethostname(char *name, size_t len);
#endif

#if defined HAVE_GETTIMEOFDAY
/* FIXME: Is time.h available everywhere? We should probably have
 * multiple timer options. */
#include <time.h>
#include <sys/time.h>
#endif /* HAVE_GETTIMEOFDAY */

#if defined MAXHOSTNAMELEN
#define MAX_HOSTNAME_LEN MAXHOSTNAMELEN
#else
#define MAX_HOSTNAME_LEN 256
#endif /* MAXHOSTNAMELEN */

#if defined MANUAL_EXTERN_ENVIRON
extern char **environ;
#endif /* MANUAL_EXTERN_ENVIRON */


/* sockets required headers */
#include <poll.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if !defined size_t
#define size_t unsigned int
#endif /* size_t */


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


/* Proxy type */
typedef enum {
    HYD_LAUNCH_UNSET,
    HYD_LAUNCH_RUNTIME,

    /* For persistent proxies */
    HYD_LAUNCH_BOOT,
    HYD_LAUNCH_BOOT_FOREGROUND,
    HYD_LAUNCH_SHUTDOWN,
    HYD_LAUNCH_PERSISTENT
} HYD_Launch_mode_t;


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

    int pgid;                   /* All executables with the same PGID belong to the same
                                 * job. */

    struct HYD_Partition_exec *next;
};

#if !defined HAVE_PTHREAD_H
#error "pthread.h needed"
#else
#include <pthread.h>
#endif

#define dprintf(...)

struct HYD_Thread_context {
    pthread_t thread;
};

struct HYD_Partition_base {
    char *name;
    char *exec_args[HYD_NUM_TMP_STRINGS];       /* Full argument list */
    char *proxy_args[HYD_NUM_TMP_STRINGS];      /* Proxy argument list */

    int partition_id;
    int active;

    int pid;
    int in;                     /* stdin is only valid for partition_id 0 */
    int out;
    int err;

    struct HYD_Partition_base *next;    /* Unused */
};

/* Partition information */
struct HYD_Partition {
    struct HYD_Partition_base *base;

    char *user_bind_map;
    int partition_core_count;

    /* Segment list will contain one-pass of the hosts file */
    struct HYD_Partition_segment *segment_list;
    struct HYD_Partition_exec *exec_list;

    /* Spawn information: each partition can have one or more
     * proxies. For the time being, we only support one proxy per
     * partition, but this can be easily extended later. We will also
     * need to give different ports for the proxies to listen on in
     * that case. */
    int *exit_status;
    int control_fd;

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
};

#endif /* HYDRA_BASE_H_INCLUDED */
