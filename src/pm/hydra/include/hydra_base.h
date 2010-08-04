/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_BASE_H_INCLUDED
#define HYDRA_BASE_H_INCLUDED

#include <stdio.h>
#include "hydra_config.h"

#if defined NEEDS_POSIX_FOR_SIGACTION
#define _POSIX_SOURCE
#endif /* NEEDS_POSIX_FOR_SIGACTION */

#if defined HAVE_WINDOWS_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif /* HAVE_WINDOWS_H */

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

#if defined HAVE_TIME_H
#include <time.h>
#endif /* HAVE_TIME_H */

#if defined HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#if defined HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif /* HAVE_IFADDRS_H */

#if defined HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */

#if defined HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if !defined HAVE_GETTIMEOFDAY
#error "hydra requires gettimeofday support"
#endif /* HAVE_GETTIMEOFDAY */

#if !defined HAVE_MACRO_VA_ARGS
#error "hydra requires VA_ARGS support"
#endif /* HAVE_MACRO_VA_ARGS */

#if defined MAXHOSTNAMELEN
#define MAX_HOSTNAME_LEN MAXHOSTNAMELEN
#else
#define MAX_HOSTNAME_LEN 256
#endif /* MAXHOSTNAMELEN */

#define HYDRA_MAX_PATH 4096

/* sockets required headers */
#ifdef HAVE_POLL_H
#include <poll.h>
#endif /* HAVE_POLL_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif /* HAVE_NETINET_TCP_H */

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#define HYD_POLLIN  (0x0001)
#define HYD_POLLOUT (0x0002)
#define HYD_POLLHUP (0x0004)

#define HYD_TMPBUF_SIZE (64 * 1024)
#define HYD_TMP_STRLEN  1024
#define HYD_NUM_TMP_STRINGS 1000

#define dprintf(...)

#ifndef ATTRIBUTE
#ifdef HAVE_GCC_ATTRIBUTE
#define ATTRIBUTE(a_) __attribute__(a_)
#else
#define ATTRIBUTE(a_)
#endif
#endif

#define HYD_IS_HELP(str) \
    ((!strcmp((str), "-h")) || (!strcmp((str), "-help")) || (!strcmp((str), "--help")))

#define HYD_DRAW_LINE(x)                                 \
    {                                                    \
        int i_;                                          \
        for (i_ = 0; i_ < (x); i_++)                     \
            printf("=");                                 \
        printf("\n");                                    \
    }

#define HYD_GET_ENV_STR_VAL(lvalue_, env_var_name_, default_val_)       \
    do {                                                       \
        if (lvalue_ == NULL) {                                 \
            const char *tmp_ = (default_val_);                 \
            MPL_env2str(env_var_name_, (const char **) &tmp_); \
            if (tmp_)                                          \
                lvalue_ = HYDU_strdup(tmp_);                   \
        }                                                      \
    } while (0)

#define HYD_CONVERT_FALSE_TO_NULL(x) \
    {                                                                   \
        if (!(x)) {                                                     \
        }                                                               \
        else if (!strcasecmp((x), "none") || !strcasecmp((x), "no") ||  \
                 !strcasecmp((x), "dummy") || !strcasecmp((x), "null") || \
                 !strcasecmp((x), "nil") || !strcasecmp((x), "false")) { \
            HYDU_FREE((x));                                             \
            (x) = NULL;                                                 \
        }                                                               \
    }

#if defined MANUAL_EXTERN_ENVIRON
extern char **environ;
#endif /* MANUAL_EXTERN_ENVIRON */

#define HYD_SILENT_ERROR(status) (((status) == HYD_GRACEFUL_ABORT) || ((status) == HYD_TIMED_OUT))

#define HYDRA_NAMESERVER_DEFAULT_PORT 6392

/* fd state */
enum HYD_fd_state {
    HYD_FD_UNSET = -1,
    HYD_FD_CLOSED = -2
};

/* Status information */
typedef enum {
    HYD_SUCCESS = 0,
    HYD_FAILURE, /* general failure */

    /* Silent errors */
    HYD_GRACEFUL_ABORT,
    HYD_TIMED_OUT,

    /* Regular errors */
    HYD_NO_MEM,
    HYD_SOCK_ERROR,
    HYD_INVALID_PARAM,
    HYD_INTERNAL_ERROR
} HYD_status;

#if defined(NEEDS_GETHOSTNAME_DECL)
int gethostname(char *name, size_t len);
#endif

typedef unsigned short HYD_event_t;

/* Argument matching functions */
struct HYD_arg_match_table {
    const char *arg;
     HYD_status(*handler_fn) (char *arg, char ***argv_p);
    void (*help_fn) (void);
};


/* Environment information */
struct HYD_env {
    const char *env_name;
    const char *env_value;
    struct HYD_env *next;
};

typedef enum HYD_env_overwrite {
    HYD_ENV_OVERWRITE_TRUE,
    HYD_ENV_OVERWRITE_FALSE
} HYD_env_overwrite_t;

typedef enum {
    HYD_ENV_PROP_UNSET,
    HYD_ENV_PROP_ALL,
    HYD_ENV_PROP_NONE,
    HYD_ENV_PROP_LIST
} HYD_env_prop_t;

struct HYD_env_global {
    struct HYD_env *system;
    struct HYD_env *user;
    struct HYD_env *inherited;
    char *prop;
};

/* Executable information */
struct HYD_exec {
    char *exec[HYD_NUM_TMP_STRINGS];
    char *wdir;

    int proc_count;
    struct HYD_env *user_env;
    char *env_prop;

    int appnum;

    struct HYD_exec *next;
};

/* Process group */
struct HYD_pg {
    int pgid;
    struct HYD_proxy *proxy_list;
    int pg_process_count;
    int barrier_count;

    struct HYD_pg *spawner_pg;

    /* scratch space for the PM */
    void *pg_scratch;

    struct HYD_pg *next;
};

/* Information about the node itself */
struct HYD_node {
    char *hostname;
    int core_count;

    /* Node-specific binding information */
    char *local_binding;

    struct HYD_node *next;
};

/* Proxy information */
struct HYD_proxy {
    struct HYD_node node;

    struct HYD_pg *pg;          /* Back pointer to the PG */

    char **exec_launch_info;

    int proxy_id;

    int start_pid;
    int proxy_process_count;

    struct HYD_exec *exec_list;

    int *pid;
    int *exit_status;
    int control_fd;

    struct HYD_proxy *next;
};

/* Global user parameters */
struct HYD_user_global {
    /* Bootstrap server */
    char *bootstrap;
    char *bootstrap_exec;

    /* Process binding */
    char *binding;
    char *bindlib;

    /* Checkpoint restart */
    char *ckpointlib;
    char *ckpoint_prefix;

    /* Demux engine */
    char *demux;

    /* Network interface */
    char *iface;

    /* Other random parameters */
    int prepend_rank;
    int enablex;
    int debug;

    int auto_cleanup;

    struct HYD_env_global global_env;
};

#endif /* HYDRA_BASE_H_INCLUDED */
