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

#if defined HAVE_TIME_H
#include <time.h>
#endif /* HAVE_TIME_H */

#if defined HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#include <errno.h>

#if !defined HAVE_GETTIMEOFDAY
#error "hydra requires gettimeofday support"
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

#if defined MANUAL_EXTERN_ENVIRON
extern char **environ;
#endif /* MANUAL_EXTERN_ENVIRON */

#define HYD_SILENT_ERROR(status) (((status) == HYD_GRACEFUL_ABORT) || ((status) == HYD_TIMED_OUT))

/* Status information */
typedef enum {
    HYD_SUCCESS = 0,

    /* Silent errors */
    HYD_GRACEFUL_ABORT,
    HYD_TIMED_OUT,

    /* Regular errors */
    HYD_NO_MEM,
    HYD_SOCK_ERROR,
    HYD_INVALID_PARAM,
    HYD_INTERNAL_ERROR
} HYD_status;

#if defined(HAVE_PUTENV) && defined(NEEDS_PUTENV_DECL)
extern int putenv(char *string);
#endif

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
    char *env_name;
    char *env_value;
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

    /* scratch space for the PM */
    void *pg_scratch;

    struct HYD_pg *next;
};

/* Information about the node itself */
struct HYD_node {
    char *hostname;
    int core_count;

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

    int *exit_status;
    int control_fd;

    /* scratch space for the PM */
    void *proxy_scratch;

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

    /* Other random parameters */
    int enablex;
    int debug;

    struct HYD_env_global global_env;
};

#endif /* HYDRA_BASE_H_INCLUDED */
