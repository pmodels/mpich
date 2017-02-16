/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIEXEC_H_INCLUDED
#define MPIEXEC_H_INCLUDED

#include "mpl_uthash.h"

#define MPIEXEC_USIZE__UNSET     (0)
#define MPIEXEC_USIZE__SYSTEM    (-1)
#define MPIEXEC_USIZE__INFINITE  (-2)

/* Interaction commands between the UI and the proxy */
struct mpiexec_cmd {
    enum {
        MPIEXEC_CMD_TYPE__SIGNAL
    } type;

    int signum;
};

struct mpiexec_params_s {
    char *rmk;
    char *launcher;
    char *launcher_exec;

    char *binding;
    char *mapping;
    char *membind;

    int debug;
    int usize;

    int tree_width;

    int auto_cleanup;

    char *base_path;
    char *port_range;
    char *nameserver;
    char *localhost;

    struct HYD_node *global_node_list;
    int global_node_count;
    int global_core_count;
    int *global_active_processes;

    /* Cleanup */
    int signal_pipe[2];

    int ppn;
    int print_all_exitcodes;

    int timeout;

    enum {
        MPIEXEC_ENVPROP__UNSET = 0,
        MPIEXEC_ENVPROP__ALL,
        MPIEXEC_ENVPROP__NONE,
        MPIEXEC_ENVPROP__LIST,
    } envprop;

    int envlist_count;
    char **envlist;

    /* primary envs are force-propagated.  secondary envs are never
     * overwritten. */
    struct {
        struct HYD_env *list;
        int serialized_buf_len;
        void *serialized_buf;
    } primary, secondary;

    char *prepend_pattern;
    char *outfile_pattern;
    char *errfile_pattern;
};

struct mpiexec_pg {
    int pgid;

    int node_count;
    struct HYD_node *node_list;

    int total_proc_count;
    struct HYD_exec *exec_list;

    int num_downstream;
    struct {
        int *fd_stdin;
        int *fd_stdout;
        int *fd_stderr;
        int *fd_control;
        int *proxy_id;
        int *pid;

        void **kvcache;
        int *kvcache_size;
        int *kvcache_num_blocks;
    } downstream;

    int barrier_count;

    char *pmi_process_mapping;

    MPL_UT_hash_handle hh;
};

extern struct mpiexec_pg *mpiexec_pg_hash;
extern struct mpiexec_params_s mpiexec_params;

HYD_status mpiexec_get_parameters(char **t_argv);
HYD_status mpiexec_pmi_barrier(struct mpiexec_pg *pg);
HYD_status mpiexec_alloc_pg(struct mpiexec_pg **pg, int pgid);
void mpiexec_free_params(void);
void mpiexec_print_params(void);
HYD_status mpiexec_stdout_cb(int pgid, int proxy_id, int rank, void *buf, int buflen);
HYD_status mpiexec_stderr_cb(int pgid, int proxy_id, int rank, void *buf, int buflen);

#endif /* MPIEXEC_H_INCLUDED */
