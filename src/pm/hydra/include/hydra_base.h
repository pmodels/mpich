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

#define HYDRA_MAX_PATH 4096

/* sockets required headers */
#include <poll.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#define HYD_DEFAULT_PROXY_PORT 9899

#define HYD_STDOUT  (1)
#define HYD_STDIN   (2)

#define HYD_TMPBUF_SIZE (64 * 1024)
#define HYD_TMP_STRLEN  1024
#define HYD_NUM_TMP_STRINGS 200

#if !defined HAVE_PTHREAD_H
#error "pthread.h needed"
#else
#include <pthread.h>
#endif

#define dprintf(...)

#ifndef ATTRIBUTE
#ifdef HAVE_GCC_ATTRIBUTE
#define ATTRIBUTE(a_) __attribute__(a_)
#else
#define ATTRIBUTE(a_)
#endif
#endif

#define PROXY_IS_ACTIVE(proxy, total_procs) \
    ((proxy)->segment_list->start_pid <= (total_procs))

#define FORALL_ACTIVE_PROXIES(proxy, proxy_list)    \
    for ((proxy) = (proxy_list); (proxy) && (proxy)->active; (proxy) = (proxy)->next)

#define FORALL_PROXIES(proxy, proxy_list)    \
    for ((proxy) = (proxy_list); (proxy); (proxy) = (proxy)->next)

#if defined MANUAL_EXTERN_ENVIRON
extern char **environ;
#endif /* MANUAL_EXTERN_ENVIRON */

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

#if defined(HAVE_PUTENV) && defined(NEEDS_PUTENV_DECL)
extern int putenv(char *string);
#endif

#if defined(NEEDS_GETHOSTNAME_DECL)
int gethostname(char *name, size_t len);
#endif

typedef unsigned short HYD_Event_t;

/* Environment information */
typedef struct HYD_Env {
    char *env_name;
    char *env_value;
    struct HYD_Env *next;
} HYD_Env_t;

typedef enum HYD_Env_overwrite {
    HYD_ENV_OVERWRITE_TRUE,
    HYD_ENV_OVERWRITE_FALSE
} HYD_Env_overwrite_t;

typedef enum {
    HYD_ENV_PROP_UNSET,
    HYD_ENV_PROP_ALL,
    HYD_ENV_PROP_NONE,
    HYD_ENV_PROP_LIST
} HYD_Env_prop_t;

struct HYD_Env_global {
    HYD_Env_t *system;
    HYD_Env_t *user;
    HYD_Env_t *inherited;
    char      *prop;
};

/* List of contiguous segments of processes on a proxy */
struct HYD_Proxy_segment {
    int start_pid;
    int proc_count;
    struct HYD_Proxy_segment *next;
};

/* Executables on a proxy */
struct HYD_Proxy_exec {
    char *exec[HYD_NUM_TMP_STRINGS];
    int proc_count;
    HYD_Env_t *user_env;
    char *env_prop;

    int pgid;                   /* All executables with the same PGID belong to the same
                                 * job. */

    struct HYD_Proxy_exec *next;
};

/* Proxy information */
struct HYD_Proxy {
    char *hostname;
    char **exec_args;

    int  proxy_id;
    int  active;

    int  pid;
    int  in;  /* stdin is only valid for proxy_id 0 */
    int  out;
    int  err;

    int proxy_core_count;

    /* Segment list will contain one-pass of the hosts file */
    struct HYD_Proxy_segment *segment_list;
    struct HYD_Proxy_exec *exec_list;

    int *exit_status;
    int control_fd;

    struct HYD_Proxy *next;
};

struct HYD_Exec_info {
    int process_count;
    char *exec[HYD_NUM_TMP_STRINGS];

    /* Local environment */
    HYD_Env_t *user_env;
    char *env_prop;

    struct HYD_Exec_info *next;
};

/* Global user parameters */
struct HYD_User_global {
    /* Bootstrap server */
    char *bootstrap;
    char *bootstrap_exec;

    /* Process binding */
    char *binding;
    char *bindlib;

    /* Checkpoint restart */
    char *ckpointlib;
    char *ckpoint_prefix;
    int ckpoint_restart;

    /* Other random parameters */
    int enablex;
    int debug;
    char *wdir;
    HYD_Launch_mode_t launch_mode;
    struct HYD_Env_global global_env;
};

#endif /* HYDRA_BASE_H_INCLUDED */
