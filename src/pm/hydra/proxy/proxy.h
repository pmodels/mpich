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
    MPL_UT_hash_handle hh;
};

struct proxy_int_hash {
    int key;
    int val;
    MPL_UT_hash_handle hh;
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
        int barrier_ref_count;
        int pid_ref_count;
    } root;

    /* immediate children */
    struct {
        struct {
            int num_children;
            struct proxy_int_hash *control_fd_hash;
            struct proxy_int_hash *stdin_fd_hash;
            struct proxy_int_hash *stdout_fd_hash;
            struct proxy_int_hash *stderr_fd_hash;
            struct proxy_int_hash *pid_hash;
            int *block_start;
            int *block_size;

            void **kvcache;
            int *kvcache_size;
            int *kvcache_num_blocks;
        } proxy;

        struct {
            int num_children;
            struct proxy_int_hash *stdout_fd_hash;
            struct proxy_int_hash *stderr_fd_hash;
            struct proxy_int_hash *pmi_fd_hash;
            struct proxy_int_hash *pid_hash;
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

HYD_status proxy_upstream_control_cb(int fd, HYD_dmx_event_t events, void *userp);
HYD_status proxy_downstream_control_cb(int fd, HYD_dmx_event_t events, void *userp);
HYD_status proxy_process_stdout_cb(int fd, HYD_dmx_event_t events, void *userp);
HYD_status proxy_process_stderr_cb(int fd, HYD_dmx_event_t events, void *userp);
HYD_status proxy_process_pmi_cb(int fd, HYD_dmx_event_t events, void *userp);
HYD_status proxy_barrier_in(int fd, struct proxy_kv_hash *hash);
HYD_status proxy_barrier_out(int fd, struct proxy_kv_hash *hash);
HYD_status proxy_pmi_kvcache_out(int num_blocks, int *kvlen, char *kvcache, int buflen);
HYD_status proxy_send_pids_upstream();

struct proxy_pmi_handle {
    const char *cmd;
     HYD_status(*handler) (int fd, struct proxy_kv_hash * hash);
};

extern struct proxy_pmi_handle *proxy_pmi_handlers;

#endif /* PROXY_H_INCLUDED */
