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

#define HYDU_ERR_POP(status, message)                                   \
    {                                                                   \
        if (status != HYD_SUCCESS && status != HYD_GRACEFUL_ABORT) {    \
            if (strlen(message))                                        \
                HYDU_error_printf(message);                             \
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
                HYDU_error_printf(message);                             \
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
                HYDU_error_printf(message, arg1);                       \
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
                HYDU_error_printf(message, arg1, arg2);                 \
            goto fn_fail;                                               \
        }                                                               \
        else if (status == HYD_GRACEFUL_ABORT) {                        \
            goto fn_exit;                                               \
        }                                                               \
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
void HYDU_init_pg(struct HYD_pg *pg);
HYD_status HYDU_alloc_pg(struct HYD_pg **pg);
void HYDU_free_pg_list(struct HYD_pg *pg_list);
HYD_status HYDU_alloc_proxy(struct HYD_proxy **proxy);
void HYDU_free_proxy_list(struct HYD_proxy *proxy_list);
HYD_status HYDU_alloc_proxy_exec(struct HYD_proxy_exec **exec);

/* args */
HYD_status HYDU_parse_array(char ***argv, struct HYD_arg_match_table *match_table);
HYD_status HYDU_set_str(char *arg, char ***argv, char **var, char *val);
HYD_status HYDU_set_str_and_incr(char *arg, char ***argv, char **var);
HYD_status HYDU_set_int(char *arg, char ***argv, int *var, int val);
HYD_status HYDU_set_int_and_incr(char *arg, char ***argv, int *var);
HYD_status HYDU_get_base_path(const char *execname, char *wdir, char **path);
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

/* env */
HYD_status HYDU_env_to_str(HYD_env_t * env, char **str);
HYD_status HYDU_str_to_env(char *str, HYD_env_t ** env);
HYD_status HYDU_list_inherited_env(HYD_env_t ** env_list);
HYD_env_t *HYDU_env_list_dup(HYD_env_t * env);
HYD_status HYDU_env_create(HYD_env_t ** env, const char *env_name, char *env_value);
HYD_status HYDU_env_free(HYD_env_t * env);
HYD_status HYDU_env_free_list(HYD_env_t * env);
HYD_env_t *HYDU_env_lookup(char *env_name, HYD_env_t * env_list);
HYD_status HYDU_append_env_to_list(HYD_env_t env, HYD_env_t ** env_list);
HYD_status HYDU_putenv(HYD_env_t * env, HYD_env_overwrite_t overwrite);
HYD_status HYDU_putenv_list(HYD_env_t * env_list, HYD_env_overwrite_t overwrite);
HYD_status HYDU_comma_list_to_env_list(char *str, HYD_env_t ** env_list);

/* launch */
HYD_status HYDU_create_process(char **client_arg, HYD_env_t * env_list,
                               int *in, int *out, int *err, int *pid, int os_index);
HYD_status HYDU_fork_and_exit(int os_index);

/* others */
int HYDU_local_to_global_id(int local_id, int start_pid, int core_count,
                            int global_core_count);
HYD_status HYDU_add_to_node_list(char *hostname, int num_procs, struct HYD_node **node_list);

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
    HYDU_SOCK_COMM_MSGWAIT = 0x01
};

HYD_status HYDU_sock_listen(int *listen_fd, char *port_range, uint16_t * port);
HYD_status HYDU_sock_connect(const char *host, uint16_t port, int *fd);
HYD_status HYDU_sock_accept(int listen_fd, int *fd);
HYD_status HYDU_sock_readline(int fd, char *buf, int maxlen, int *linelen);
HYD_status HYDU_sock_writeline(int fd, const char *buf, int maxsize);
HYD_status HYDU_sock_read(int fd, void *buf, int maxlen, int *count,
                          enum HYDU_sock_comm_flag flag);
HYD_status HYDU_sock_write(int fd, const void *buf, int maxsize);
HYD_status HYDU_sock_trywrite(int fd, const void *buf, int maxsize);
HYD_status HYDU_sock_forward_stdio(int in, int out, int *closed);

/* Memory utilities */
#include <ctype.h>

/* FIXME: This should eventually become MPL_malloc and friends */
#define HYDU_malloc malloc
#define HYDU_calloc calloc
#define HYDU_free   free

#define HYDU_snprintf MPL_snprintf
#define HYDU_strdup   MPL_strdup

#define HYDU_MALLOC(p, type, size, status)                              \
    {                                                                   \
        (p) = (type) HYDU_malloc((size));                               \
        if ((p) == NULL)                                                \
            HYDU_ERR_SETANDJUMP1((status), HYD_NO_MEM,                  \
                                 "failed to allocate %d bytes\n",       \
                                 (size));                               \
    }

#define HYDU_CALLOC(p, type, num, size, status)                         \
    {                                                                   \
        (p) = (type) HYDU_calloc((num), (size));                        \
        if ((p) == NULL)                                                \
            HYDU_ERR_SETANDJUMP1((status), HYD_NO_MEM,                  \
                                 "failed to allocate %d bytes\n",       \
                                 (size));                               \
    }

#define HYDU_FREE(p)                            \
    {                                           \
        HYDU_free(p);                           \
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



/*! \addtogroup demux Demultiplexing Engine
 * @{
 */

/**
 * \brief HYDU_dmx_register_fd - Register file descriptors for events
 *
 * \param[in]  num_fds  Number of file descriptors being provided
 * \param[in]  fd       Array of file descriptors
 * \param[in]  events   Events we are interested in
 * \param[in]  userp    Persistent user storage associated with a descriptor
 *                      (not interpreted by the demux engine)
 * \param[in]  callback Callback function to call when the event occurs
 *
 * This function registers all file descriptors to be waited on for an
 * event.
 */
HYD_status HYDU_dmx_register_fd(int num_fds, int *fd, HYD_event_t events, void *userp,
                                HYD_status(*callback) (int fd, HYD_event_t events,
                                                       void *userp));

/**
 * \brief HYDU_dmx_deregister_fd - Deregister file descriptor
 *
 * \param[in]   fd    File descriptor to be deregistered
 *
 * This function deregisters a file descriptor from the demux
 * engine. All registered fd's must be deregistered.
 */
HYD_status HYDU_dmx_deregister_fd(int fd);

/**
 * \brief HYDU_dmx_wait_for_event - Wait for event
 *
 * \param    time      Time to wait for in seconds (-1 means infinite)
 *
 * This function waits till either one of the registered fd's has had
 * one of its registered events, or till the timeout expires.
 */
HYD_status HYDU_dmx_wait_for_event(int time);

/**
 * \brief HYDU_dmx_finalize - Finalize demux engine
 *
 * This function cleans up any relevant state that the demux engine
 * maintained.
 */
HYD_status HYDU_dmx_finalize(void);

/**
 * \brief HYDU_dmx_query_fd_registration - Query if an fd is registered
 *
 * \param[in]  fd     File descriptor to check whether registered
 * \param[out] ret    1 if registered; 0 if not
 *
 * If a file descriptor is being registered and deregistered by
 * different layers, this function allows us to query what its state
 * is.
 */
HYD_status HYDU_dmx_query_fd_registration(int fd, int *ret);

/*!
 * @}
 */

#endif /* HYDRA_UTILS_H_INCLUDED */
