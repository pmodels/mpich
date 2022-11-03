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
    int *rankmap;
    int pg_process_count;
    int barrier_count;
    bool is_active;

    int spawner_pgid;

    /* user-specified node-list */
    struct HYD_node *user_node_list;

    /* scratch space for the PM */
    void *pg_scratch;
};

struct HYD_server_info_s {
    struct HYD_user_global user_global;

    bool hybrid_hosts;
    char *base_path;
    char *port_range;
    char *iface_ip_env_name;
    char *nameserver;
    char *localhost;
    char *rankmap;
    time_t time_start;

    bool is_singleton;
    int singleton_port;
    int singleton_pid;

     HYD_status(*stdout_cb) (int pgid, int proxy_id, int rank, void *buf, int buflen);
     HYD_status(*stderr_cb) (int pgid, int proxy_id, int rank, void *buf, int buflen);

    /* All of the available nodes */
    struct HYD_node *node_list;

    /* Cleanup */
    int cmd_pipe[2];

#if defined ENABLE_PROFILING
    int enable_profiling;
    int num_pmi_calls;
#endif                          /* ENABLE_PROFILING */
};

extern struct HYD_server_info_s HYD_server_info;

void PMISERV_pg_init(void);
int PMISERV_pg_alloc(void);
void PMISERV_pg_finalize(void);
int PMISERV_pg_max_id(void);
struct HYD_pg *PMISERV_pg_by_id(int pgid);

HYD_status HYD_pmcd_pmi_alloc_pg_scratch(struct HYD_pg *pg);
HYD_status HYD_pmcd_pmi_free_pg_scratch(struct HYD_pg *pg);

void PMISERV_proxy_init(void);
void PMISERV_proxy_finalize(void);
int PMISERV_proxy_alloc(void);  /* return proxy_id */
int PMISERV_proxy_max_id(void);
struct HYD_proxy *PMISERV_proxy_by_id(int proxy_id);

#endif /* HYDRA_SERVER_H_INCLUDED */
