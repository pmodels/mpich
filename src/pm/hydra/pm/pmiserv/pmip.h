/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
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

struct HYD_pmcd_pmip_s {
    struct HYD_user_global user_global;

    struct {
        struct {
            int local_filler;
            int local_count;
            int global_count;
        } global_core_map;

        struct {
            int filler_start;
            int non_filler_start;
        } pmi_id_map;

        int global_process_count;

        /* PMI */
        char *pmi_fd;
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
        char *hostname;

        int proxy_core_count;
        int proxy_process_count;

        char *spawner_kvsname;
        struct HYD_pmcd_pmi_kvs *kvs;   /* Node-level KVS space for node attributes */

        int retries;
    } local;

    /* Process segmentation information for this proxy */
    struct HYD_exec *exec_list;
};

extern struct HYD_pmcd_pmip_s HYD_pmcd_pmip;
extern struct HYD_arg_match_table HYD_pmcd_pmip_match_table[];

HYD_status HYD_pmcd_pmip_get_params(char **t_argv);
void HYD_pmcd_pmip_send_signal(int sig);
HYD_status HYD_pmcd_pmip_control_cmd_cb(int fd, HYD_event_t events, void *userp);

#endif /* PMIP_H_INCLUDED */
