/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_UTILS_H_INCLUDED
#define HYDRA_UTILS_H_INCLUDED

#include "hydra_base.h"

/* args */
HYD_Status HYDU_get_base_path(char *execname, char *wdir, char **path);


/* bind */
#if defined PROC_BINDING
#include "plpa.h"
#include "plpa_internal.h"
HYD_Status HYDU_bind_process(int core);
#else
#define HYDU_bind_process(...) HYD_SUCCESS
#endif /* PROC_BINDING */


/* env */
HYD_Status HYDU_list_append_env_to_str(HYD_Env_t * env_list, char **str_list);
HYD_Status HYDU_list_global_env(HYD_Env_t ** env_list);
HYD_Env_t *HYDU_env_list_dup(HYD_Env_t * env);
HYD_Status HYDU_env_create(HYD_Env_t ** env, char *env_name, char *env_value);
HYD_Status HYDU_env_free(HYD_Env_t * env);
HYD_Status HYDU_env_free_list(HYD_Env_t * env);
HYD_Env_t *HYDU_env_lookup(HYD_Env_t env, HYD_Env_t * env_list);
HYD_Status HYDU_append_env_to_list(HYD_Env_t env, HYD_Env_t ** env_list);
void HYDU_putenv(char *env_str);


/* launch */
struct HYD_Partition_list {
    char *name;
    int proc_count;
    char **mapping;             /* Can be core IDs or something else */

    int group_id;               /* Assumed to be in ascending order */
    int group_rank;             /* Rank within the group */
    int pid;
    int out;
    int err;
    int exit_status;
    char *args[HYD_EXEC_ARGS];

    struct HYD_Partition_list *next;
};

HYD_Status HYDU_alloc_partition(struct HYD_Partition_list **partition);
void HYDU_free_partition_list(struct HYD_Partition_list *partition);
HYD_Status HYDU_create_process(char **client_arg, int *in, int *out, int *err,
                               int *pid, int core);


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

HYD_Status HYDU_set_signal(int signum, void (*handler) (int));
HYD_Status HYDU_set_common_signals(void (*handler) (int));


/* Sock utilities */
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

HYD_Status HYDU_sock_listen(int *listen_fd, char *port_range, uint16_t * port);
HYD_Status HYDU_sock_connect(const char *host, uint16_t port, int *fd);
HYD_Status HYDU_sock_accept(int listen_fd, int *fd);
HYD_Status HYDU_sock_readline(int fd, char *buf, int maxlen, int *linelen);
HYD_Status HYDU_sock_read(int fd, void *buf, int maxlen, int *count);
HYD_Status HYDU_sock_writeline(int fd, char *buf, int maxsize);
HYD_Status HYDU_sock_write(int fd, void *buf, int maxsize);
HYD_Status HYDU_sock_set_nonblock(int fd);
HYD_Status HYDU_sock_set_cloexec(int fd);
HYD_Status HYDU_sock_stdout_cb(int fd, HYD_Event_t events, int stdout_fd, int *closed);
HYD_Status HYDU_sock_stdin_cb(int fd, HYD_Event_t events, char *buf, int *buf_count,
                              int *buf_offset, int *closed);


/* Memory utilities */
#define HYDU_NUM_JOIN_STR 100

#define HYDU_MALLOC(p, type, size, status)                              \
    {                                                                   \
        (p) = (type) MPIU_Malloc((size));                               \
        if ((p) == NULL)                                                \
            HYDU_ERR_SETANDJUMP1((status), HYD_NO_MEM,                  \
                                 "failed to allocate %d bytes\n",       \
                                 (size));                               \
    }

#define HYDU_FREE(p)                            \
    {                                           \
        MPIU_Free(p);                           \
    }

#define HYDU_STRDUP(src, dest, type, status)                            \
    {                                                                   \
        (dest) = (type) MPIU_Strdup((src));                             \
        if ((dest) == NULL)                                             \
            HYDU_ERR_SETANDJUMP1((status), HYD_INTERNAL_ERROR,          \
                             "failed duping string %s\n", (src));       \
    }

HYD_Status HYDU_list_append_strlist(char **exec, char **client_arg);
HYD_Status HYDU_print_strlist(char **args);
void HYDU_free_strlist(char **args);
HYD_Status HYDU_str_alloc_and_join(char **strlist, char **strjoin);
HYD_Status HYDU_strsplit(char *str, char **str1, char **str2, char sep);
HYD_Status HYDU_int_to_str(int x, char **str);
char *HYDU_strerror(int error);


/* Timer utilities */
/* FIXME: HYD_Time should be OS specific */
#ifdef HAVE_TIME
#include <time.h>
#endif /* HAVE_TIME */
typedef struct timeval HYD_Time;
void HYDU_time_set(HYD_Time * time, int *val);
int HYDU_time_left(HYD_Time start, HYD_Time timeout);

#endif /* HYDRA_UTILS_H_INCLUDED */
