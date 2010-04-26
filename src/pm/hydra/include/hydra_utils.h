/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_UTILS_H_INCLUDED
#define HYDRA_UTILS_H_INCLUDED

#include "hydra_base.h"
#include "mpl.h"

#if defined HAVE__FUNC__
#define HYDU_FUNC __func__
#elif defined HAVE_CAP__FUNC__
#define HYDU_FUNC __FUNC__
#elif defined HAVE__FUNCTION__
#define HYDU_FUNC __FUNCTION__
#endif

#if !defined COMPILER_ACCEPTS_VA_ARGS
#define HYDU_error_printf printf
#elif defined __FILE__ && defined HYDU_FUNC
#define HYDU_error_printf(...)                                          \
    {                                                                   \
        HYDU_dump_prefix(stderr);                                       \
        HYDU_dump_noprefix(stderr, "%s (%s:%d): ", __func__, __FILE__, __LINE__); \
        HYDU_dump_noprefix(stderr, __VA_ARGS__);                        \
    }
#elif defined __FILE__
#define HYDU_error_printf(...)                            \
    {                                                     \
        HYDU_dump_prefix(stderr);                                       \
        HYDU_dump_noprefix(stderr, "%s (%d): ", __FILE__, __LINE__);    \
        HYDU_dump_noprefix(stderr, __VA_ARGS__);                        \
    }
#else
#define HYDU_error_printf(...)                                          \
    {                                                                   \
        HYDU_dump_prefix(stderr);                                       \
        HYDU_dump_noprefix(stderr, __VA_ARGS__);                        \
    }
#endif

#define HYDU_ASSERT(x, status)                                          \
    {                                                                   \
        if (!(x)) {                                                     \
            HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,            \
                                 "assert (%s) failed\n", #x);           \
        }                                                               \
    }

#define HYDU_IGNORE_TIMEOUT(status)             \
    {                                           \
        if ((status) == HYD_TIMED_OUT)          \
            (status) = HYD_SUCCESS;             \
    }

#define HYDU_ERR_POP(status, message)                                   \
    {                                                                   \
        if (status && !HYD_SILENT_ERROR(status)) {                      \
            if (strlen(message))                                        \
                HYDU_error_printf(message);                             \
            goto fn_fail;                                               \
        }                                                               \
        else if (HYD_SILENT_ERROR(status)) {                            \
            goto fn_exit;                                               \
        }                                                               \
    }

#define HYDU_ERR_POP1(status, message, arg1)                            \
    {                                                                   \
        if (status && !HYD_SILENT_ERROR(status)) {                      \
            if (strlen(message))                                        \
                HYDU_error_printf(message, arg1);                       \
            goto fn_fail;                                               \
        }                                                               \
        else if (HYD_SILENT_ERROR(status)) {                            \
            goto fn_exit;                                               \
        }                                                               \
    }

#define HYDU_ERR_POP2(status, message, arg1, arg2)                      \
    {                                                                   \
        if (status && !HYD_SILENT_ERROR(status)) {                      \
            if (strlen(message))                                        \
                HYDU_error_printf(message, arg1, arg2);                 \
            goto fn_fail;                                               \
        }                                                               \
        else if (HYD_SILENT_ERROR(status)) {                            \
            goto fn_exit;                                               \
        }                                                               \
    }

#define HYDU_ERR_SETANDJUMP(status, error, message)                     \
    {                                                                   \
        status = error;                                                 \
        if (status && !HYD_SILENT_ERROR(status)) {                      \
            if (strlen(message))                                        \
                HYDU_error_printf(message);                             \
            goto fn_fail;                                               \
        }                                                               \
        else if (HYD_SILENT_ERROR(status)) {                            \
            goto fn_exit;                                               \
        }                                                               \
    }

#define HYDU_ERR_SETANDJUMP1(status, error, message, arg1)              \
    {                                                                   \
        status = error;                                                 \
        if (status && !HYD_SILENT_ERROR(status)) {                      \
            if (strlen(message))                                        \
                HYDU_error_printf(message, arg1);                       \
            goto fn_fail;                                               \
        }                                                               \
        else if (HYD_SILENT_ERROR(status)) {                            \
            goto fn_exit;                                               \
        }                                                               \
    }

#define HYDU_ERR_SETANDJUMP2(status, error, message, arg1, arg2)        \
    {                                                                   \
        status = error;                                                 \
        if (status && !HYD_SILENT_ERROR(status)) {                      \
            if (strlen(message))                                        \
                HYDU_error_printf(message, arg1, arg2);                 \
            goto fn_fail;                                               \
        }                                                               \
        else if (HYD_SILENT_ERROR(status)) {                            \
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

#if defined ENABLE_WARNINGS || !defined COMPILER_ACCEPTS_VA_ARGS
#define HYDU_warn_printf HYDU_error_printf
#else
#define HYDU_warn_printf(...)
#endif /* ENABLE_WARNINGS */

/* Disable for now; we might add something here in the future */
#define HYDU_FUNC_ENTER()   do {} while (0)
#define HYDU_FUNC_EXIT()    do {} while (0)


/* alloc */
void HYDU_init_user_global(struct HYD_user_global *user_global);
void HYDU_init_global_env(struct HYD_env_global *global_env);
HYD_status HYDU_alloc_node(struct HYD_node **node);
void HYDU_free_node_list(struct HYD_node *node_list);
void HYDU_init_pg(struct HYD_pg *pg, int pgid);
HYD_status HYDU_alloc_pg(struct HYD_pg **pg, int pgid);
void HYDU_free_pg_list(struct HYD_pg *pg_list);
HYD_status HYDU_alloc_proxy(struct HYD_proxy **proxy, struct HYD_pg *pg);
void HYDU_free_proxy_list(struct HYD_proxy *proxy_list);
HYD_status HYDU_alloc_exec(struct HYD_exec **exec);
void HYDU_free_exec_list(struct HYD_exec *exec_list);
HYD_status HYDU_create_proxy_list(struct HYD_exec *exec_list, struct HYD_node *node_list,
                                  struct HYD_pg *pg, int proc_offset);
HYD_status HYDU_correct_wdir(char **wdir);

/* args */
HYD_status HYDU_find_in_path(const char *execname, char **path);
HYD_status HYDU_parse_array(char ***argv, struct HYD_arg_match_table *match_table);
HYD_status HYDU_set_str(char *arg, char ***argv, char **var, const char *val);
HYD_status HYDU_set_str_and_incr(char *arg, char ***argv, char **var);
HYD_status HYDU_set_int(char *arg, char ***argv, int *var, int val);
HYD_status HYDU_set_int_and_incr(char *arg, char ***argv, int *var);
char *HYDU_getcwd(void);
HYD_status HYDU_parse_hostfile(char *hostfile,
                               HYD_status(*process_token) (char *token, int newline));
char *HYDU_find_full_path(const char *execname);
HYD_status HYDU_send_strlist(int fd, char **strlist);

/* debug */
HYD_status HYDU_dbg_init(const char *str);
void HYDU_dump_prefix(FILE * fp);
void HYDU_dump_noprefix(FILE * fp, const char *str, ...);
void HYDU_dump(FILE * fp, const char *str, ...);
void HYDU_dbg_finalize(void);

/* env */
HYD_status HYDU_env_to_str(struct HYD_env *env, char **str);
HYD_status HYDU_str_to_env(char *str, struct HYD_env **env);
HYD_status HYDU_list_inherited_env(struct HYD_env **env_list);
struct HYD_env *HYDU_env_list_dup(struct HYD_env *env);
HYD_status HYDU_env_create(struct HYD_env **env, const char *env_name, const char *env_value);
HYD_status HYDU_env_free(struct HYD_env *env);
HYD_status HYDU_env_free_list(struct HYD_env *env);
struct HYD_env *HYDU_env_lookup(char *env_name, struct HYD_env *env_list);
HYD_status HYDU_append_env_to_list(struct HYD_env env, struct HYD_env **env_list);
HYD_status HYDU_putenv(struct HYD_env *env, HYD_env_overwrite_t overwrite);
HYD_status HYDU_putenv_list(struct HYD_env *env_list, HYD_env_overwrite_t overwrite);
HYD_status HYDU_comma_list_to_env_list(char *str, struct HYD_env **env_list);
HYD_status HYDU_env_purge_existing(struct HYD_env **env_list);

/* launch */
HYD_status HYDU_create_process(char **client_arg, struct HYD_env *env_list,
                               int *in, int *out, int *err, int *pid, int os_index);

/* others */
int HYDU_local_to_global_id(int local_id, int start_pid, int core_count,
                            int global_core_count);
HYD_status HYDU_add_to_node_list(const char *hostname, int num_procs,
                                 struct HYD_node **node_list);
HYD_status HYDU_gethostname(char *hostname);

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
HYD_status HYDU_sock_connect(const char *host, uint16_t port, int *fd);
HYD_status HYDU_sock_accept(int listen_fd, int *fd);
HYD_status HYDU_sock_read(int fd, void *buf, int maxlen, int *recvd, int *closed,
                          enum HYDU_sock_comm_flag flag);
HYD_status HYDU_sock_write(int fd, const void *buf, int maxlen, int *sent, int *closed);
HYD_status HYDU_sock_forward_stdio(int in, int out, int *closed);
HYD_status HYDU_sock_get_iface_ip(char *iface, char **ip);
HYD_status
HYDU_sock_create_and_listen_portstr(char *iface, char *port_range, char **port_str,
                                    HYD_status(*callback) (int fd, HYD_event_t events,
                                                           void *userp), void *userp);
HYD_status HYDU_sock_cloexec(int fd);


/* Memory utilities */
#include <ctype.h>

#if defined USE_MEMORY_TRACING

#define HYDU_mem_init()  MPL_trinit(0)

#define HYDU_strdup(a) MPL_trstrdup(a,__LINE__,__FILE__)
#define strdup(a)      'Error use HYDU_strdup' :::

#define HYDU_malloc(a) MPL_trmalloc((unsigned)(a),__LINE__,__FILE__)
#define malloc(a)      'Error use HYDU_malloc' :::

#define HYDU_free(a) MPL_trfree(a,__LINE__,__FILE__)
#define free(a)      'Error use HYDU_free' :::

#else /* if !defined USE_MEMORY_TRACING */

#define HYDU_mem_init()
#define HYDU_strdup MPL_strdup
#define HYDU_malloc malloc
#define HYDU_free free

#endif /* USE_MEMORY_TRACING */

#define HYDU_snprintf MPL_snprintf

#define HYDU_MALLOC(p, type, size, status)                              \
    {                                                                   \
        (p) = (type) HYDU_malloc((size));                               \
        if ((p) == NULL)                                                \
            HYDU_ERR_SETANDJUMP1((status), HYD_NO_MEM,                  \
                                 "failed to allocate %d bytes\n",       \
                                 (size));                               \
    }

#define HYDU_FREE(p)                            \
    {                                           \
        HYDU_free((void *) p);                  \
    }

#define HYDU_STRLIST_CONSOLIDATE(strlist, i, status)                    \
    {                                                                   \
        char *out;                                                      \
        if ((i) >= (HYD_NUM_TMP_STRINGS / 2)) {                         \
            (strlist)[(i)] = NULL;                                      \
            (status) = HYDU_str_alloc_and_join((strlist), &out);        \
            HYDU_ERR_POP((status), "unable to join strings\n");         \
            HYDU_free_strlist((strlist));                               \
            strlist[0] = out;                                           \
            (i) = 1;                                                    \
        }                                                               \
    }

HYD_status HYDU_list_append_strlist(char **exec, char **client_arg);
HYD_status HYDU_print_strlist(char **args);
void HYDU_free_strlist(char **args);
HYD_status HYDU_str_alloc_and_join(char **strlist, char **strjoin);
HYD_status HYDU_strsplit(char *str, char **str1, char **str2, char sep);
HYD_status HYDU_strdup_list(char *src[], char **dest[]);
char *HYDU_int_to_str(int x);
char *HYDU_strerror(int error);
int HYDU_strlist_lastidx(char **strlist);
char **HYDU_str_to_strlist(char *str);

/*!
 * @}
 */

#endif /* HYDRA_UTILS_H_INCLUDED */
