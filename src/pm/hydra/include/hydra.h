/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_H_INCLUDED
#define HYDRA_H_INCLUDED

/* hydra_config.h must come first, otherwise feature macros like _USE_GNU that
 * were defined by AC_USE_SYSTEM_EXTENSIONS will not be defined yet when mpl.h
 * indirectly includes features.h.  This leads to a mismatch between the
 * behavior determined by configure and the behavior actually caused by
 * "#include"ing unistd.h, for example. */
#include "hydra_config.h"

#include "mpl.h"
#include "mpl_uthash.h"

extern char *HYD_dbg_prefix;

/* C89 headers can be included without a check */
#if defined STDC_HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#else
#error "STDC_HEADERS are assumed in the Hydra code"
#endif /* STDC_HEADERS */

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

#if defined HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */

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
#endif /* HAVE_SYS_SOCKET_H */

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif /* HAVE_SIGNAL_H */

#define HYD_POLLIN  (0x0001)
#define HYD_POLLOUT (0x0002)
#define HYD_POLLHUP (0x0004)

#define HYD_TMPBUF_SIZE (64 * 1024)
#define HYD_TMP_STRLEN  (16 * 1024)
#define HYD_NUM_TMP_STRINGS 1000

#define HYD_DEFAULT_RETRY_COUNT (10)
#define HYD_CONNECT_DELAY (10)

#define dprintf(...)

#ifndef ATTRIBUTE
#ifdef HAVE_GCC_ATTRIBUTE
#define ATTRIBUTE(a_) __attribute__(a_)
#else
#define ATTRIBUTE(a_)
#endif
#endif

#define HYD_DRAW_LINE(x)                                 \
    {                                                    \
        int i_;                                          \
        for (i_ = 0; i_ < (x); i_++)                     \
            printf("=");                                 \
        printf("\n");                                    \
    }

#define HYD_CONVERT_FALSE_TO_NULL(x)                                    \
    {                                                                   \
        if (!(x)) {                                                     \
        }                                                               \
        else if (!strcasecmp((x), "none") || !strcasecmp((x), "no") ||  \
                 !strcasecmp((x), "dummy") || !strcasecmp((x), "null") || \
                 !strcasecmp((x), "nil") || !strcasecmp((x), "false")) { \
            MPL_free((x));                                              \
            (x) = NULL;                                                 \
        }                                                               \
    }

#if defined MANUAL_EXTERN_ENVIRON
extern char **environ;
#endif /* MANUAL_EXTERN_ENVIRON */

#if defined NEEDS_HSTRERROR_DECL
const char *hstrerror(int err);
#endif /* NEEDS_HSTRERROR_DECL */

#if defined NEEDS_GETTIMEOFDAY_DECL
int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif /* NEEDS_GETTIMEOFDAY_DECL */

#if defined NEEDS_GETPGID_DECL
pid_t getpgid(pid_t pid);
#endif /* NEEDS_GETPGID_DECL */

#if defined NEEDS_KILLPG_DECL
int killpg(int pgrp, int sig);
#endif /* NEEDS_KILLPG_DECL */

#if defined(MPL_HAVE_PUTENV) && defined(MPL_NEEDS_PUTENV_DECL)
extern int putenv(char *string);
#endif

#define HYD_SILENT_ERROR(status) (((status) == HYD_GRACEFUL_ABORT) || ((status) == HYD_TIMED_OUT))

#define HYDRA_NAMESERVER_DEFAULT_PORT 6392

struct HYD_string_stash {
    char **strlist;
    int max_count;
    int cur_count;
};

#define HYD_STRING_STASH_INIT(stash)            \
    do {                                        \
        (stash).strlist = NULL;                 \
        (stash).max_count = 0;                  \
        (stash).cur_count = 0;                  \
    } while (0)

#define HYD_STRING_STASH(stash, str, status)                            \
    do {                                                                \
        if ((stash).cur_count >= (stash).max_count - 1) {               \
            HYDU_REALLOC_OR_JUMP((stash).strlist, char **,              \
                                 ((stash).max_count + HYD_NUM_TMP_STRINGS) * sizeof(char *), \
                                 (status));                             \
            (stash).max_count += HYD_NUM_TMP_STRINGS;                   \
        }                                                               \
        (stash).strlist[(stash).cur_count++] = (str);                   \
        (stash).strlist[(stash).cur_count] = NULL;                      \
    } while (0)

#define HYD_STRING_SPIT(stash, str, status)                             \
    do {                                                                \
        if ((stash).cur_count == 0) {                                   \
            (str) = MPL_strdup("");                                     \
        }                                                               \
        else {                                                          \
            (status) = HYDU_str_alloc_and_join((stash).strlist, &(str)); \
            HYDU_ERR_POP((status), "unable to join strings\n");         \
            HYDU_free_strlist((stash).strlist);                         \
            MPL_free((stash).strlist);                                  \
            HYD_STRING_STASH_INIT((stash));                             \
        }                                                               \
    } while (0)

#define HYD_STRING_STASH_FREE(stash)            \
    do {                                        \
        if ((stash).strlist == NULL)            \
            break;                              \
        HYDU_free_strlist((stash).strlist);     \
        MPL_free((stash).strlist);              \
        (stash).max_count = 0;                  \
        (stash).cur_count = 0;                  \
    } while (0)

enum HYD_bool {
    HYD_FALSE = 0,
    HYD_TRUE = 1
};

/* fd state */
enum HYD_fd_state {
    HYD_FD_UNSET = -1,
    HYD_FD_CLOSED = -2
};

/* Status information */
typedef enum {
    HYD_SUCCESS = 0,
    HYD_FAILURE,        /* general failure */

    /* Silent errors */
    HYD_GRACEFUL_ABORT,
    HYD_TIMED_OUT,

    /* Regular errors */
    HYD_NO_MEM,
    HYD_SOCK_ERROR,
    HYD_INVALID_PARAM,
    HYD_INTERNAL_ERROR
} HYD_status;

#define HYD_USIZE_UNSET     (0)
#define HYD_USIZE_SYSTEM    (-1)
#define HYD_USIZE_INFINITE  (-2)

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
    int proxy_count;
    int pg_process_count;
    int barrier_count;

    struct HYD_pg *spawner_pg;

    /* user-specified node-list */
    struct HYD_node *user_node_list;
    int pg_core_count;

    /* scratch space for the PM */
    void *pg_scratch;

    struct HYD_pg *next;
};

/* Information about the node itself */
struct HYD_node {
    /* information filled out by the RMK */
    char *username;
    char *hostname;
    int core_count;
    char *local_binding;

    /* information filled out once the proxies are setup */
    int active_processes;

    int node_id;

    struct HYD_node *next;
};

/* Proxy information */
struct HYD_proxy {
    struct HYD_node *node;

    struct HYD_pg *pg;          /* Back pointer to the PG */

    char **exec_launch_info;

    int proxy_id;

    int proxy_process_count;

    /* Filler processes that we are adding on this proxy */
    int filler_processes;

    struct HYD_exec *exec_list;

    int *pid;
    int *exit_status;
    int control_fd;

    struct HYD_proxy *next;

    MPL_UT_hash_handle hh;
};

/* Global user parameters */
struct HYD_user_global {
    /* RMK */
    char *rmk;

    /* Launcher */
    char *launcher;
    char *launcher_exec;

    /* Processor/Memory topology */
    char *topolib;
    char *binding;
    char *mapping;
    char *membind;

    /* Other random parameters */
    int enablex;
    int debug;
    int usize;

    int auto_cleanup;

    struct HYD_env_global global_env;
};

#define HYDU_dump_prefix(fp)                    \
    {                                           \
        fprintf(fp, "[%s] ", HYD_dbg_prefix);   \
        fflush(fp);                             \
    }

#define HYDU_dump_noprefix(fp, ...)             \
    {                                           \
        fprintf(fp, __VA_ARGS__);               \
        fflush(fp);                             \
    }

#define HYDU_dump(fp, ...)                      \
    {                                           \
        HYDU_dump_prefix(fp);                   \
        HYDU_dump_noprefix(fp, __VA_ARGS__);    \
    }

#if defined HAVE__FUNC__
#define HYDU_FUNC __func__
#elif defined HAVE_CAP__FUNC__
#define HYDU_FUNC __FUNC__
#elif defined HAVE__FUNCTION__
#define HYDU_FUNC __FUNCTION__
#endif

#if defined __FILE__ && defined HYDU_FUNC
#define HYDU_error_printf(...)                                          \
    {                                                                   \
        HYDU_dump_prefix(stderr);                                       \
        HYDU_dump_noprefix(stderr, "%s (%s:%d): ", HYDU_FUNC, __FILE__, __LINE__); \
        HYDU_dump_noprefix(stderr, __VA_ARGS__);                        \
    }
#elif defined __FILE__
#define HYDU_error_printf(...)                                          \
    {                                                                   \
        HYDU_dump_prefix(stderr);                                       \
        HYDU_dump_noprefix(stderr, "%s (%d): ", __FILE__, __LINE__);    \
        HYDU_dump_noprefix(stderr, __VA_ARGS__);                        \
    }
#else
#define HYDU_error_printf(...)                          \
    {                                                   \
        HYDU_dump_prefix(stderr);                       \
        HYDU_dump_noprefix(stderr, __VA_ARGS__);        \
    }
#endif

#define HYDU_ASSERT(x, status)                                  \
    {                                                           \
        if (!(x)) {                                             \
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,     \
                                "assert (%s) failed\n", #x);    \
        }                                                       \
    }

#define HYDU_IGNORE_TIMEOUT(status)             \
    {                                           \
        if ((status) == HYD_TIMED_OUT)          \
            (status) = HYD_SUCCESS;             \
    }

#define HYDU_ERR_POP(status, ...)                       \
    {                                                   \
        if (status && !HYD_SILENT_ERROR(status)) {      \
            HYDU_error_printf(__VA_ARGS__);             \
            goto fn_fail;                               \
        }                                               \
        else if (HYD_SILENT_ERROR(status)) {            \
            goto fn_exit;                               \
        }                                               \
    }

#define HYDU_ERR_SETANDJUMP(status, error, ...) \
    {                                           \
        status = error;                         \
        HYDU_ERR_POP(status, __VA_ARGS__);      \
    }

#define HYDU_ERR_CHKANDJUMP(status, chk, error, ...)            \
    {                                                           \
        if ((chk))                                              \
            HYDU_ERR_SETANDJUMP(status, error, __VA_ARGS__);    \
    }

#if defined ENABLE_WARNINGS
#define HYDU_warn_printf HYDU_error_printf
#else
#define HYDU_warn_printf(...)
#endif /* ENABLE_WARNINGS */

/* Disable for now; we might add something here in the future */
#define HYDU_FUNC_ENTER()   do {} while (0)
#define HYDU_FUNC_EXIT()    do {} while (0)


/* alloc */
void HYDU_init_user_global(struct HYD_user_global *user_global);
void HYDU_finalize_user_global(struct HYD_user_global *user_global);
void HYDU_init_global_env(struct HYD_env_global *global_env);
void HYDU_finalize_global_env(struct HYD_env_global *global_env);
HYD_status HYDU_alloc_node(struct HYD_node **node);
void HYDU_free_node_list(struct HYD_node *node_list);
void HYDU_init_pg(struct HYD_pg *pg, int pgid);
HYD_status HYDU_alloc_pg(struct HYD_pg **pg, int pgid);
void HYDU_free_pg_list(struct HYD_pg *pg_list);
void HYDU_free_proxy_list(struct HYD_proxy *proxy_list);
HYD_status HYDU_alloc_exec(struct HYD_exec **exec);
void HYDU_free_exec_list(struct HYD_exec *exec_list);
HYD_status HYDU_create_proxy_list(struct HYD_exec *exec_list, struct HYD_node *node_list,
                                  struct HYD_pg *pg);
HYD_status HYDU_correct_wdir(char **wdir);

/* args */
HYD_status HYDU_find_in_path(const char *execname, char **path);
HYD_status HYDU_parse_array(char ***argv, struct HYD_arg_match_table *match_table);
HYD_status HYDU_set_str(char *arg, char **var, const char *val);
HYD_status HYDU_set_int(char *arg, int *var, int val);
char *HYDU_getcwd(void);
HYD_status HYDU_process_mfile_token(char *token, int newline, struct HYD_node **node_list);
char *HYDU_get_abs_wd(const char *wd);
HYD_status HYDU_parse_hostfile(const char *hostfile, struct HYD_node **node_list,
                               HYD_status(*process_token) (char *token, int newline,
                                                           struct HYD_node ** node_list));
char *HYDU_find_full_path(const char *execname);
HYD_status HYDU_send_strlist(int fd, char **strlist);

/* debug */
HYD_status HYDU_dbg_init(const char *str);
void HYDU_dbg_finalize(void);

/* env */
HYD_status HYDU_env_to_str(struct HYD_env *env, char **str);
HYD_status HYDU_list_inherited_env(struct HYD_env **env_list);
struct HYD_env *HYDU_env_list_dup(struct HYD_env *env);
HYD_status HYDU_env_create(struct HYD_env **env, const char *env_name, const char *env_value);
HYD_status HYDU_env_free(struct HYD_env *env);
HYD_status HYDU_env_free_list(struct HYD_env *env);
struct HYD_env *HYDU_env_lookup(char *env_name, struct HYD_env *env_list);
HYD_status HYDU_append_env_to_list(const char *env_name, const char *env_value,
                                   struct HYD_env **env_list);
HYD_status HYDU_append_env_str_to_list(const char *str, struct HYD_env **env_list);
HYD_status HYDU_putenv(struct HYD_env *env, HYD_env_overwrite_t overwrite);
HYD_status HYDU_putenv_list(struct HYD_env *env_list, HYD_env_overwrite_t overwrite);
HYD_status HYDU_comma_list_to_env_list(char *str, struct HYD_env **env_list);

/* launch */
HYD_status HYDU_create_process(char **client_arg, struct HYD_env *env_list,
                               int *in, int *out, int *err, int *pid, int idx);

/* others */
int HYDU_dceil(int x, int y);
HYD_status HYDU_add_to_node_list(const char *hostname, int num_procs, struct HYD_node **node_list);
void HYDU_delay(unsigned long delay);

/* signals */
#ifdef NEEDS_POSIX_FOR_SIGACTION
#define _POSIX_SOURCE
#endif

#include <sys/wait.h>
#if defined(USE_SIGNAL) || defined(USE_SIGACTION)
#include <signal.h>
#else
#error no signal choice
#endif
#ifdef NEEDS_STRSIGNAL_DECL
extern char *strsignal(int);
#endif

HYD_status HYDU_set_signal(int signum, void (*handler) (int));
HYD_status HYDU_set_common_signals(void (*handler) (int));

/* Sock utilities */
enum HYDU_sock_comm_flag {
    HYDU_SOCK_COMM_NONE = 0,
    HYDU_SOCK_COMM_MSGWAIT = 1
};

HYD_status HYDU_sock_listen(int *listen_fd, char *port_range, uint16_t * port);

/* delay is in microseconds */
HYD_status HYDU_sock_connect(const char *host, uint16_t port, int *fd, int retries,
                             unsigned long delay);
HYD_status HYDU_sock_accept(int listen_fd, int *fd);
HYD_status HYDU_sock_read(int fd, void *buf, int maxlen, int *recvd, int *closed,
                          enum HYDU_sock_comm_flag flag);
HYD_status HYDU_sock_write(int fd, const void *buf, int maxlen, int *sent, int *closed,
                           enum HYDU_sock_comm_flag flag);
HYD_status HYDU_sock_set_nonblock(int fd);
HYD_status HYDU_sock_forward_stdio(int in, int out, int *closed);
void HYDU_sock_finalize(void);
HYD_status
HYDU_sock_create_and_listen_portstr(char *hostname, char *port_range,
                                    char **port_str,
                                    HYD_status(*callback) (int fd, HYD_event_t events,
                                                           void *userp), void *userp);
HYD_status HYDU_sock_cloexec(int fd);


/* Memory utilities */
#include <ctype.h>

#define HYDU_MALLOC_OR_JUMP(p, type, size, status)                      \
    {                                                                   \
        (p) = NULL; /* initialize p in case assert fails */             \
        HYDU_ASSERT(size, status);                                      \
        (p) = (type) MPL_malloc((size));                                \
        if ((p) == NULL)                                                \
            HYDU_ERR_SETANDJUMP((status), HYD_NO_MEM,                   \
                                "failed to allocate %d bytes\n",        \
                                (int) (size));                          \
    }

#define HYDU_REALLOC_OR_JUMP(p, type, size, status)                     \
    {                                                                   \
        HYDU_ASSERT(size, status);                                      \
        (p) = (type) MPL_realloc((p),(size));                           \
        if ((p) == NULL)                                                \
            HYDU_ERR_SETANDJUMP((status), HYD_NO_MEM,                   \
                                "failed to allocate %d bytes\n",        \
                                (int) (size));                          \
    }

HYD_status HYDU_list_append_strlist(char **exec, char **client_arg);
HYD_status HYDU_print_strlist(char **args);
void HYDU_free_strlist(char **args);
HYD_status HYDU_str_alloc_and_join(char **strlist, char **strjoin);
HYD_status HYDU_strsplit(char *str, char **str1, char **str2, char sep);
HYD_status HYDU_strdup_list(char *src[], char **dest[]);
char *HYDU_size_t_to_str(size_t x);
char *HYDU_int_to_str(int x);
char *HYDU_int_to_str_pad(int x, int maxlen);

#if defined HAVE_HERROR
#define HYDU_herror herror
#else
#define HYDU_herror HYDU_int_to_str
#endif /* HAVE_HERROR */

int HYDU_strlist_lastidx(char **strlist);
char **HYDU_str_to_strlist(char *str);

/*!
 * @}
 */

#endif /* HYDRA_H_INCLUDED */
