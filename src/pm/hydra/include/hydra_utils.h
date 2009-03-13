/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_UTILS_H_INCLUDED
#define HYDRA_UTILS_H_INCLUDED

#include "hydra_base.h"

/* Environment utilities */
HYD_Status HYDU_Env_global_list(HYD_Env_t ** env_list);
HYD_Env_t *HYDU_Env_dup(HYD_Env_t env);
HYD_Env_t *HYDU_Env_listdup(HYD_Env_t * env);
HYD_Status HYDU_Env_create(HYD_Env_t ** env, char *env_name, char *env_value);
HYD_Status HYDU_Env_free(HYD_Env_t * env);
HYD_Status HYDU_Env_free_list(HYD_Env_t * env);
HYD_Env_t *HYDU_Env_found_in_list(HYD_Env_t * env_list, HYD_Env_t env);
HYD_Status HYDU_Env_add_to_list(HYD_Env_t ** env_list, HYD_Env_t env);
HYD_Status HYDU_Env_assign_form(HYD_Env_t env, char **env_str);
void HYDU_Env_putenv(char *env_str);


/* Launch utilities */
struct HYD_Partition_list {
    char *name;
    int proc_count;
    char **mapping;             /* Can be core IDs or something else */

    /*
     * The boot-strap server is expected to start a single executable
     * on the first possible node and return a single PID. This
     * executable could be a PM proxy that will launch the actual
     * application on the rest of the partition list.
     *
     * Possible hacks:
     *
     *   1. If the process manager needs more proxies within this same
     *      list, it can use different group IDs. Each group ID will
     *      have its own proxy.
     *
     *   2. If no proxy is needed, the PM can split this list into one
     *      element per process. The boot-strap server itself does not
     *      distinguish a proxy from the application executable, so it
     *      will not require any changes.
     *
     *   3. One proxy per physical node means that each partition will
     *      have a different group ID.
     */
    int group_id;               /* Assumed to be in ascending order */
    int group_rank;             /* Rank within the group */
    int pid;
    int out;
    int err;
    int exit_status;
    char *args[HYD_EXEC_ARGS];

    struct HYD_Partition_list *next;
};

HYD_Status HYDU_Append_env(HYD_Env_t * env_list, char **client_arg);
HYD_Status HYDU_Append_exec(char **exec, char **client_arg);
HYD_Status HYDU_Append_wdir(char **client_arg, char *wdir);
HYD_Status HYDU_Allocate_Partition(struct HYD_Partition_list **partition);
HYD_Status HYDU_Create_process(char **client_arg, int *in, int *out, int *err, int *pid);
HYD_Status HYDU_Dump_args(char **args);
HYD_Status HYDU_Get_base_path(char *execname, char **path);
HYD_Status HYDU_Chdir(const char *dir);


/* Memory utilities */
#define HYDU_NUM_JOIN_STR 100

#define HYDU_MALLOC(p, type, size, status)                              \
    {                                                                   \
        (p) = (type) MPIU_Malloc((size));                               \
        if ((p) == NULL) {                                              \
            HYDU_Error_printf("failed trying to allocate %d bytes\n", (size)); \
            (status) = HYD_NO_MEM;                                      \
            goto fn_fail;                                               \
        }                                                               \
    }

#define HYDU_FREE(p)                            \
    {                                           \
        MPIU_Free(p);                           \
    }

#define HYDU_STRDUP(src, dest, type, status)                            \
    {                                                                   \
        (dest) = (type) MPIU_Strdup((src));                             \
        if ((dest) == NULL) {                                           \
            HYDU_Error_printf("failed duping string %s\n", (src));      \
            (status) = HYD_INTERNAL_ERROR;                              \
            goto fn_fail;                                               \
        }                                                               \
    }

HYD_Status HYDU_String_alloc_and_join(char **strlist, char **strjoin);
HYD_Status HYDU_String_int_to_str(int x, char **str);


/* Signal utilities */
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

HYD_Status HYDU_Set_signal(int signum, void (*handler) (int));
HYD_Status HYDU_Set_common_signals(void (*handler) (int));


/* Timer utilities */
/* FIXME: HYD_Time should be OS specific */
#ifdef HAVE_TIME
#include <time.h>
#endif /* HAVE_TIME */
typedef struct timeval HYD_Time;
void HYDU_Time_set(HYD_Time * time, int *val);
int HYDU_Time_left(HYD_Time start, HYD_Time timeout);


/* Sock utilities */
#include <poll.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#if !defined size_t
#define size_t unsigned int
#endif /* size_t */

HYD_Status HYDU_Sock_listen(int *listen_fd, char *port_range, uint16_t * port);
HYD_Status HYDU_Sock_connect(const char *host, uint16_t port, int *fd);
HYD_Status HYDU_Sock_accept(int listen_fd, int *fd);
HYD_Status HYDU_Sock_readline(int fd, char *buf, int maxlen, int *linelen);
HYD_Status HYDU_Sock_read(int fd, void *buf, int maxlen, int *count);
HYD_Status HYDU_Sock_writeline(int fd, char *buf, int maxsize);
HYD_Status HYDU_Sock_write(int fd, void *buf, int maxsize);
HYD_Status HYDU_Sock_set_nonblock(int fd);
HYD_Status HYDU_Sock_set_cloexec(int fd);
HYD_Status HYDU_Sock_stdout_cb(int fd, HYD_Event_t events, int stdout_fd, int *closed);
HYD_Status HYDU_Sock_stdin_cb(int fd, HYD_Event_t events, char *buf, int *buf_count,
                              int *buf_offset, int *closed);

#endif /* HYDRA_UTILS_H_INCLUDED */
