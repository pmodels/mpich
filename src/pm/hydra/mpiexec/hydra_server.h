/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef HYDRA_SERVER_H_INCLUDED
#define HYDRA_SERVER_H_INCLUDED

#include "hydra.h"

/* Interaction commands between the UI and the proxy */
struct HYD_cmd {
    enum {
        HYD_CLEANUP,
        HYD_SIGCHLD,
        HYD_SIGNAL
    } type;

    int signum;
};

/* Process group */
struct HYD_pg {
    int pgid;
    struct HYD_proxy *proxy_list;
    int proxy_count;
    int pg_process_count;
    int barrier_count;
    bool is_active;

    struct HYD_pg *spawner_pg;

    /* user-specified node-list */
    struct HYD_node *user_node_list;
    int pg_core_count;

    /* scratch space for the PM */
    void *pg_scratch;

    struct HYD_pg *next;
};

struct HYD_server_info_s {
    struct HYD_user_global user_global;

    bool hybrid_hosts;
    char *base_path;
    char *port_range;
    char *iface_ip_env_name;
    char *nameserver;
    char *localhost;
    time_t time_start;

    int singleton_port;
    int singleton_pid;

     HYD_status(*stdout_cb) (int pgid, int proxy_id, int rank, void *buf, int buflen);
     HYD_status(*stderr_cb) (int pgid, int proxy_id, int rank, void *buf, int buflen);

    /* All of the available nodes */
    struct HYD_node *node_list;

    /* Process groups */
    struct HYD_pg pg_list;

    /* Hash for fast proxy lookup */
    struct HYD_proxy *proxy_hash;

    /* Cleanup */
    int cmd_pipe[2];

#if defined ENABLE_PROFILING
    int enable_profiling;
    int num_pmi_calls;
#endif                          /* ENABLE_PROFILING */
};

extern struct HYD_server_info_s HYD_server_info;

void HYDU_init_pg(struct HYD_pg *pg, int pgid);
HYD_status HYDU_alloc_pg(struct HYD_pg **pg, int pgid);
void HYDU_free_pg_list(struct HYD_pg *pg_list);
struct HYD_pg *HYDU_get_pg(int pgid);

#endif /* HYDRA_SERVER_H_INCLUDED */
