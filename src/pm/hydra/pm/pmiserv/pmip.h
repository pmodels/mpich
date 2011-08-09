/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMIP_H_INCLUDED
#define PMIP_H_INCLUDED

#include "hydra.h"
#include "common.h"

struct HYD_pmcd_pmip_map {
    int left;
    int current;
    int right;
    int total;
};

struct HYD_pmcd_pmip {
    struct HYD_user_global user_global;

    struct {
        struct HYD_pmcd_pmip_map global_core_map;
        struct HYD_pmcd_pmip_map filler_process_map;

        int global_process_count;
        char *jobid;

        /* PMI */
        char *pmi_fd;
        char *pmi_port;
        int pmi_rank;           /* If this is -1, we auto-generate it */
        char *pmi_process_mapping;
    } system_global;            /* Global system parameters */

    struct {
        /* Upstream server contact information */
        char *server_name;
        int server_port;
        int control;
    } upstream;

    /* Currently our downstream only consists of actual MPI
     * processes */
    struct {
        int *out;
        int *err;
        int in;

        int *pid;
        int *exit_status;

        int *pmi_rank;
        int *pmi_fd;
        int *pmi_fd_active;

        int forced_cleanup;
    } downstream;

    /* Proxy details */
    struct {
        int id;
        int pgid;
        char *iface_ip_env_name;
        char *hostname;
        char *local_binding;

        int proxy_core_count;
        int proxy_process_count;

        char *spawner_kvs_name;
        struct HYD_pmcd_pmi_kvs *kvs;   /* Node-level KVS space for node attributes */

        char **ckpoint_prefix_list;

        int retries;
    } local;

    /* Process segmentation information for this proxy */
    struct HYD_exec *exec_list;
};

extern struct HYD_pmcd_pmip HYD_pmcd_pmip;
extern struct HYD_arg_match_table HYD_pmcd_pmip_match_table[];

HYD_status HYD_pmcd_pmip_get_params(char **t_argv);
void HYD_pmcd_pmip_kill_localprocs(void);
HYD_status HYD_pmcd_pmip_control_cmd_cb(int fd, HYD_event_t events, void *userp);

#endif /* PMIP_H_INCLUDED */
