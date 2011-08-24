/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_SERVER_H_INCLUDED
#define HYDRA_SERVER_H_INCLUDED

#include "hydra.h"

/* Interaction commands between the UI and the proxy */
struct HYD_cmd {
    enum {
        HYD_CLEANUP,
        HYD_CKPOINT,
        HYD_SIGNAL
    } type;

    int signum;
};

struct HYD_server_info {
    struct HYD_user_global user_global;

    char *base_path;
    char *port_range;
    char *iface_ip_env_name;
    char *nameserver;
    char *local_hostname;

     HYD_status(*stdout_cb) (int pgid, int proxy_id, int rank, void *buf, int buflen);
     HYD_status(*stderr_cb) (int pgid, int proxy_id, int rank, void *buf, int buflen);

    /* All of the available nodes */
    struct HYD_node *node_list;

    /* Process groups */
    struct HYD_pg pg_list;

    /* Cleanup */
    int cleanup_pipe[2];

#if defined ENABLE_PROFILING
    int enable_profiling;
    int num_pmi_calls;
#endif                          /* ENABLE_PROFILING */
};

extern struct HYD_server_info HYD_server_info;

#endif /* HYDRA_SERVER_H_INCLUDED */
