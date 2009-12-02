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

#define HYD_DEFAULT_PROXY_PORT 9899

#define HYD_STDOUT  (1)
#define HYD_STDIN   (2)

#define HYD_TMPBUF_SIZE (64 * 1024)
#define HYD_TMP_STRLEN  1024
#define HYD_NUM_TMP_STRINGS 200

#define dprintf(...)

#ifndef ATTRIBUTE
#ifdef HAVE_GCC_ATTRIBUTE
#define ATTRIBUTE(a_) __attribute__(a_)
#else
#define ATTRIBUTE(a_)
#endif
#endif

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
    HYD_TIMED_OUT,
    HYD_INTERNAL_ERROR
} HYD_status;

#if defined(HAVE_PUTENV) && defined(NEEDS_PUTENV_DECL)
extern int putenv(char *string);
#endif

#if defined(NEEDS_GETHOSTNAME_DECL)
int gethostname(char *name, size_t len);
#endif

typedef unsigned short HYD_event_t;

/* Environment information */
typedef struct HYD_env {
    char *env_name;
    char *env_value;
    struct HYD_env *next;
} HYD_env_t;

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
    HYD_env_t *system;
    HYD_env_t *user;
    HYD_env_t *inherited;
    char *prop;
};

/* Executables on a proxy */
struct HYD_proxy_exec {
    char *exec[HYD_NUM_TMP_STRINGS];
    int proc_count;
    HYD_env_t *user_env;
    char *env_prop;

    struct HYD_proxy_exec *next;
};

/* Process group */
struct HYD_pg {
    int pgid;
    struct HYD_proxy *proxy_list;
    int pg_process_count;

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

    char **exec_launch_info;

    int proxy_id;

    int start_pid;
    int proxy_process_count;

    struct HYD_proxy_exec *exec_list;

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

    /* Other random parameters */
    int enablex;
    int debug;
    char *wdir;

    struct HYD_env_global global_env;
};

#endif /* HYDRA_BASE_H_INCLUDED */
