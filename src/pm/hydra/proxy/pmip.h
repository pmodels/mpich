/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PMIP_H_INCLUDED
#define PMIP_H_INCLUDED

#include "hydra.h"
#include "pmiserv_common.h"

struct HYD_pmcd_pmip_map {
    int left;
    int current;
    int right;
    int total;
};

struct HYD_pmcd_pmip_s {
    struct HYD_user_global user_global;

    /* FIXME: move to struct pmip_pg */
    bool is_singleton;
    int singleton_port;
    int singleton_pid;

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

    /* Proxy details */
    struct {
        int id;
        int pgid;
        char *iface_ip_env_name;
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

#define PMIP_EXIT_STATUS_UNSET -1
HYD_status PMIP_send_hdr_upstream(struct HYD_pmcd_hdr *hdr, void *buf, int buflen);

HYD_status HYD_pmcd_pmip_control_cmd_cb(int fd, HYD_event_t events, void *userp);
const char *HYD_pmip_get_hwloc_xmlfile(void);

/* downstreams */
struct pmip_downstream {
    struct pmip_pg *pg;

    int out;
    int err;
    int in;

    int pid;
    int exit_status;

    int pmi_appnum;
    int pmi_rank;
    int pmi_fd;
    int pmi_fd_active;
};

/* Each launch belongs to separate MPI_COMM_WORLD, which is tracked by (pgid, proxy_id)
 * at the server. Each pmip_pg hosts a list of downstream processes.
 */

struct pmip_pg {
    int pgid;
    int proxy_id;

    int num_procs;
    struct pmip_downstream *downstreams;
};

void PMIP_pg_init(void);
void PMIP_pg_finalize(void);
struct pmip_pg *PMIP_new_pg(int pgid, int proxy_id);
struct pmip_pg *PMIP_pg_0(void);
HYD_status PMIP_pg_alloc_downstreams(struct pmip_pg *pg, int num_procs);
struct pmip_pg *PMIP_find_pg(int pgid, int proxy_id);

bool PMIP_pg_has_open_stdoe(struct pmip_pg *pg);

int *PMIP_pg_get_pid_list(struct pmip_pg *pg);
int *PMIP_pg_get_stdout_list(struct pmip_pg *pg);
int *PMIP_pg_get_stderr_list(struct pmip_pg *pg);
int *PMIP_pg_get_exit_status_list(struct pmip_pg *pg);

struct pmip_downstream *PMIP_find_downstream_by_fd(int fd);
struct pmip_downstream *PMIP_find_downstream_by_pid(int pid);
int PMIP_get_total_process_count(void);
bool PMIP_has_open_stdoe(void);
void PMIP_bcast_signal(int sig);

#endif /* PMIP_H_INCLUDED */
