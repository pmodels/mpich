/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_UTILS_H_INCLUDED
#define HYDRA_UTILS_H_INCLUDED

#include "hydra_base.h"
#include "mpimem.h"

int HYDU_Error_printf_simple(const char *str, ...);

#if !defined COMPILER_ACCEPTS_VA_ARGS
#define HYDU_Error_printf HYDU_Error_printf_simple
#elif defined COMPILER_ACCEPTS_FUNC && defined __LINE__
#define HYDU_Error_printf(...)                            \
    {                                                     \
        fprintf(stderr, "%s (%d): ", __func__, __LINE__); \
        HYDU_Error_printf_simple(__VA_ARGS__);            \
    }
#elif defined __FILE__ && defined __LINE__
#define HYDU_Error_printf(...)                            \
    {                                                     \
        fprintf(stderr, "%s (%d): ", __FILE__, __LINE__); \
        HYDU_Error_printf_simple(__VA_ARGS__);            \
    }
#else
#define HYDU_Error_printf(...)                  \
    {                                           \
        HYDU_Error_printf_simple(__VA_ARGS__);  \
    }
#endif

#define HYDU_ERR_POP(status, message)                                   \
    {                                                                   \
        if (status != HYD_SUCCESS && status != HYD_GRACEFUL_ABORT) {    \
            if (strlen(message))                                        \
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
            if (strlen(message))                                        \
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

#define HYDU_ERR_CHKANDJUMP1(status, chk, error, message, arg1)         \
    {                                                                   \
        if ((chk))                                                      \
            HYDU_ERR_SETANDJUMP1(status, error, message, arg1);         \
    }

#define HYDU_ERR_SETANDJUMP1(status, error, message, arg1)              \
    {                                                                   \
        status = error;                                                 \
        if (status != HYD_SUCCESS && status != HYD_GRACEFUL_ABORT) {    \
            if (strlen(message))                                        \
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
            if (strlen(message))                                        \
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

#define HYDU_Dump printf
#if defined COMPILER_ACCEPTS_VA_ARGS
#define HYDU_Debug(debug, ...)                  \
    {                                           \
        if ((debug))                            \
            HYDU_Dump(__VA_ARGS__);             \
    }
#else
#define HYDU_Debug(...) {}
#endif

/* We need to add more information in here later */
#if !defined ENABLE_DEBUG
#define HYDU_FUNC_ENTER() {}
#define HYDU_FUNC_EXIT() {}
#elif defined COMPILER_ACCEPTS_FUNC
#define HYDU_FUNC_ENTER() {}
#define HYDU_FUNC_EXIT() {}
#else
#define HYDU_FUNC_ENTER() {}
#define HYDU_FUNC_EXIT() {}
#endif


/* args */
HYD_Status HYDU_find_in_path(char *execname, char **path);
HYD_Status HYDU_get_base_path(char *execname, char *wdir, char **path);


/* bind */
#if defined PROC_BINDING
HYD_Status HYDU_bind_init(char *user_bind_map);
HYD_Status HYDU_bind_process(int core);
int HYDU_bind_get_core_id(int id, HYD_Binding binding);
#else
#define HYDU_bind_init(...) HYD_SUCCESS
#define HYDU_bind_process(...) HYD_SUCCESS
#define HYDU_bind_get_core_id(...) (-1)
#endif /* PROC_BINDING */


/* env */
HYD_Env_t *HYDU_str_to_env(char *str);
HYD_Status HYDU_list_append_env_to_str(HYD_Env_t * env_list, char **str_list);
HYD_Status HYDU_list_global_env(HYD_Env_t ** env_list);
HYD_Env_t *HYDU_env_list_dup(HYD_Env_t * env);
HYD_Status HYDU_env_create(HYD_Env_t ** env, char *env_name, char *env_value);
HYD_Status HYDU_env_free(HYD_Env_t * env);
HYD_Status HYDU_env_free_list(HYD_Env_t * env);
HYD_Env_t *HYDU_env_lookup(HYD_Env_t env, HYD_Env_t * env_list);
HYD_Status HYDU_append_env_to_list(HYD_Env_t env, HYD_Env_t ** env_list);
HYD_Status HYDU_putenv(HYD_Env_t * env);
HYD_Status HYDU_putenv_list(HYD_Env_t * env_list);
HYD_Status HYDU_comma_list_to_env_list(char *str, HYD_Env_t ** env_list);


/* launch */
#if defined HAVE_THREAD_SUPPORT
struct HYD_Thread_context {
    pthread_t thread;
};
#endif /* HAVE_THREAD_SUPPORT */

HYD_Status HYDU_alloc_partition(struct HYD_Partition **partition);
void HYDU_free_partition_list(struct HYD_Partition *partition);
HYD_Status HYDU_alloc_exec_info(struct HYD_Exec_info **exec_info);
void HYDU_free_exec_info_list(struct HYD_Exec_info *exec_info_list);
HYD_Status HYDU_alloc_partition_segment(struct HYD_Partition_segment **segment);
HYD_Status HYDU_merge_partition_segment(char *name, struct HYD_Partition_segment *segment,
                                        struct HYD_Partition **partition_list);
HYD_Status HYDU_merge_partition_mapping(char *name, char *map, int num_procs,
                                        struct HYD_Partition **partition_list);
HYD_Status HYDU_alloc_partition_exec(struct HYD_Partition_exec **exec);
HYD_Status HYDU_create_node_list_from_file(char *host_file,
                                           struct HYD_Partition **partition_list);
HYD_Status HYDU_create_process(char **client_arg, HYD_Env_t * env_list,
                               int *in, int *out, int *err, int *pid, int core);
HYD_Status HYDU_fork_and_exit(int core);
#if defined HAVE_THREAD_SUPPORT
HYD_Status HYDU_create_thread(void *(*func) (void *), void *args,
                              struct HYD_Thread_context *ctxt);
HYD_Status HYDU_join_thread(struct HYD_Thread_context ctxt);
#endif /* HAVE_THREAD_SUPPORT */
int HYDU_local_to_global_id(int local_id, int partition_core_count,
                            struct HYD_Partition_segment *segment_list, int global_core_count);


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
enum HYDU_sock_comm_flag {
    HYDU_SOCK_COMM_MSGWAIT = 0x01
};

HYD_Status HYDU_sock_listen(int *listen_fd, char *port_range, uint16_t * port);
HYD_Status HYDU_sock_connect(const char *host, uint16_t port, int *fd);
HYD_Status HYDU_sock_accept(int listen_fd, int *fd);
HYD_Status HYDU_sock_readline(int fd, char *buf, int maxlen, int *linelen);
HYD_Status HYDU_sock_writeline(int fd, char *buf, int maxsize);
HYD_Status HYDU_sock_read(int fd, void *buf, int maxlen, int *count,
                          enum HYDU_sock_comm_flag flag);
HYD_Status HYDU_sock_write(int fd, void *buf, int maxsize);
HYD_Status HYDU_sock_trywrite(int fd, void *buf, int maxsize);
HYD_Status HYDU_sock_set_nonblock(int fd);
HYD_Status HYDU_sock_set_cloexec(int fd);
HYD_Status HYDU_sock_stdout_cb(int fd, HYD_Event_t events, int stdout_fd, int *closed);
HYD_Status HYDU_sock_stdin_cb(int fd, HYD_Event_t events, int stdin_fd, char *buf,
                              int *buf_count, int *buf_offset, int *closed);


/* Memory utilities */
#include <ctype.h>

#define HYDU_MALLOC(p, type, size, status)                              \
    {                                                                   \
        (p) = (type) MPIU_Malloc((size));                               \
        if ((p) == NULL)                                                \
            HYDU_ERR_SETANDJUMP1((status), HYD_NO_MEM,                  \
                                 "failed to allocate %d bytes\n",       \
                                 (size));                               \
    }

#define HYDU_CALLOC(p, type, num, size, status)                         \
    {                                                                   \
        (p) = (type) MPIU_Calloc((num), (size));                        \
        if ((p) == NULL)                                                \
            HYDU_ERR_SETANDJUMP1((status), HYD_NO_MEM,                  \
                                 "failed to allocate %d bytes\n",       \
                                 (size));                               \
    }

#define HYDU_FREE(p)                            \
    {                                           \
        MPIU_Free(p);                           \
    }

#define HYDU_snprintf MPIU_Snprintf
#define HYDU_strdup MPIU_Strdup

HYD_Status HYDU_list_append_strlist(char **exec, char **client_arg);
HYD_Status HYDU_print_strlist(char **args);
void HYDU_free_strlist(char **args);
HYD_Status HYDU_str_alloc_and_join(char **strlist, char **strjoin);
HYD_Status HYDU_strsplit(char *str, char **str1, char **str2, char sep);
HYD_Status HYDU_strdup_list(char *src[], char **dest[]);
char *HYDU_int_to_str(int x);
char *HYDU_strerror(int error);
int HYDU_strlist_lastidx(char **strlist);
char **HYDU_str_to_strlist(char *str);


/* Timer utilities */
/* FIXME: HYD_Time should be OS specific */
#ifdef HAVE_TIME
#include <time.h>
#endif /* HAVE_TIME */
typedef struct timeval HYD_Time;
void HYDU_time_set(HYD_Time * time, int *val);
int HYDU_time_left(HYD_Time start, HYD_Time timeout);

#endif /* HYDRA_UTILS_H_INCLUDED */
