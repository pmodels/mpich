/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PROXY_H_INCLUDED
#define PROXY_H_INCLUDED

#include "hydra.h"

struct proxy_kv_hash {
    char *key;
    char *val;
    UT_hash_handle hh;
};

struct proxy_params {
    int debug;
    char *cwd;
    int usize;
    int tree_width;

    /* root: this proxy */
    struct {
        int proxy_id;
        int node_id;
        int upstream_fd;
        const char *hostname;
        int subtree_size;
        int pid_ref_count;
        int exitcode_ref_count;
    } root;

    /* immediate children */
    struct {
        struct {
            int num_children;
            struct HYD_int_hash *fd_control_hash;
            struct HYD_int_hash *fd_stdin_hash;
            struct HYD_int_hash *fd_stdout_hash;
            struct HYD_int_hash *fd_stderr_hash;
            struct HYD_int_hash *pid_hash;
            int *block_start;
            int *block_size;

            void **kvcache;
            int *kvcache_size;
            int *kvcache_num_blocks;
        } proxy;

        struct {
            int num_children;
            struct HYD_int_hash *fd_stdout_hash;
            struct HYD_int_hash *fd_stderr_hash;
            struct HYD_int_hash *fd_pmi_hash;
            struct HYD_int_hash *pid_hash;
            int *pmi_id;
        } process;
    } immediate;

    /* all nodes in the subtree */
    struct {
        int pgid;
        char *kvsname;
        struct HYD_exec *complete_exec_list;
        int global_process_count;
        struct {
            void *serial_buf;
            int serial_buf_len;
            int argc;
            char **argv;
        } primary_env, secondary_env;
        char *pmi_process_mapping;
    } all;
};

extern struct proxy_params proxy_params;
extern int proxy_ready_to_launch;
extern int **proxy_pids;
extern int **proxy_pmi_ids;
extern int *n_proxy_pids;
extern int **exitcodes;
extern int **exitcode_node_ids;
extern int *n_proxy_exitcodes;

HYD_status proxy_upstream_control_cb(int fd, HYD_dmx_event_t events, void *userp);
HYD_status proxy_downstream_control_cb(int fd, HYD_dmx_event_t events, void *userp);
HYD_status proxy_process_stdout_cb(int fd, HYD_dmx_event_t events, void *userp);
HYD_status proxy_process_stderr_cb(int fd, HYD_dmx_event_t events, void *userp);
HYD_status proxy_process_pmi_cb(int fd, HYD_dmx_event_t events, void *userp);
HYD_status proxy_barrier_in(int fd, struct proxy_kv_hash *hash);
HYD_status proxy_barrier_out(int fd, struct proxy_kv_hash *hash);
HYD_status proxy_pmi_kvcache_out(int num_blocks, int *kvlen, char *kvcache, int buflen);
HYD_status proxy_send_pids_upstream();
HYD_status proxy_send_exitcodes_upstream();

struct proxy_pmi_handle {
    const char *cmd;
     HYD_status(*handler) (int fd, struct proxy_kv_hash * hash);
};

extern struct proxy_pmi_handle *proxy_pmi_handlers;

#endif /* PROXY_H_INCLUDED */
