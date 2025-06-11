/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PMIP_H_INCLUDED
#define PMIP_H_INCLUDED

#include "hydra.h"
#include "pmiserv_common.h"
#include "uthash.h"
#include "utarray.h"

struct HYD_pmcd_pmip_map {
    int left;
    int current;
    int right;
    int total;
};

struct HYD_pmcd_pmip_s {
    struct HYD_user_global user_global;

    int singleton_port;
    int singleton_pid;

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

        int retries;
    } local;
};

/* downstreams */
struct pmip_downstream {
    int idx;
    int pg_idx;

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

#define CACHE_PUT_KEYVAL_MAXLEN  (MAX_PMI_ARGS - 1)
struct cache_put_elem {
    struct PMIU_token tokens[CACHE_PUT_KEYVAL_MAXLEN + 1];
    int keyval_len;
};

struct pmip_kvs {
    char *key;
    char *val;
    UT_hash_handle hh;
};

struct pmip_pg {
    int idx;                    /* index in global dynamic array PMIP_pgs */
    int pgid;
    int proxy_id;

    bool is_singleton;
    int num_procs;
    struct pmip_downstream *downstreams;

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
    char *pmi_process_mapping;
    char *spawner_kvsname;
    char *kvsname;

    /* Process segmentation information for this proxy */
    struct HYD_exec *exec_list;

    struct pmip_kvs *kvs;
    /* An array of kvs entries (keys) that needs to be pushed to
     * server at barrier_in. */
    UT_array *kvs_batch;

    /* for barrier_in. Use uthash to support group barriers */
    struct pmip_barrier *barriers;
    int barrier_count;

    /* environment */
    char *iface_ip_env_name;
    char *hostname;
};

extern struct HYD_pmcd_pmip_s HYD_pmcd_pmip;
extern struct HYD_arg_match_table HYD_pmip_args_match_table[];
extern struct HYD_arg_match_table HYD_pmip_procinfo_match_table[];

void HYD_set_cur_pg(struct pmip_pg *pg);
HYD_status HYD_pmcd_pmip_get_params(char **t_argv);

#define PMIP_EXIT_STATUS_UNSET -1
void HYD_pmcd_pmip_send_signal(int sig);
HYD_status PMIP_send_hdr_upstream(struct pmip_pg *pg, struct HYD_pmcd_hdr *hdr,
                                  void *buf, int buflen);

HYD_status HYD_pmcd_pmip_control_cmd_cb(int fd, HYD_event_t events, void *userp);
const char *HYD_pmip_get_hwloc_xmlfile(void);

void PMIP_pg_init(void);
void PMIP_pg_finalize(void);
struct pmip_pg *PMIP_new_pg(int pgid, int proxy_id);
struct pmip_pg *PMIP_pg_0(void);
struct pmip_pg *PMIP_pg_from_downstream(struct pmip_downstream *downstream);
HYD_status PMIP_foreach_pg_do(HYD_status(*callback) (struct pmip_pg * pg));
HYD_status PMIP_pg_alloc_downstreams(struct pmip_pg *pg, int num_procs);
struct pmip_pg *PMIP_find_pg(int pgid, int proxy_id);

int PMIP_pg_local_to_global_id(struct pmip_pg *pg, int local_id);
int PMIP_pg_global_to_local_id(struct pmip_pg *pg, int global_id);

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

/* barrier */
/* - design features:
 *    * group barriers: each HYD_barrier represents a barrier over a set of processes (or proxies)
 *    * hierarchical:   proxy aggregates downstream processes. Server aggregates proxies.
 *                      proc_list identifies processes in proxy; identifies proxies in server.
 *    * threaded support.
 */

/* group string identifies process set. Support -
 *   - WORLD, NODE, or
 *   - 0,1,4,...   (CSV list of ranks)
 */
#define PMIP_GROUP_ALL ((int *)0)

/* barrier epoch - multiple threads may concurrently use barrier over the same group and tag,
 *                 which will be serialized into epochs (a utlist).
 * Typically we only need a single epoch if it is single threaded or when tags are used to
 * distinguish threads.
 */

/* hash to track barrier epochs */
struct pmip_barrier_proc_hash {
    int proc;
    UT_hash_handle hh;
};

/* epoch */
struct pmip_barrier_epoch {
    int count;
    struct pmip_barrier_proc_hash *proc_hash;
    struct pmip_barrier_epoch *prev;
    struct pmip_barrier_epoch *next;
};

/* a barrier identified by a string key */
struct pmip_barrier {
    const char *name;           /* key identifies each barrier. It is derived from the group string and tag */
    unsigned int tag;           /* tag is for downstream process threads to identify barrier replies between threads */
    bool has_upstream;          /* used by proxy to decide whether to skip server aggregation */
    int num_procs;
    int *proc_list;             /* needed for sending "barrier_out". Accepts PMIP_GROUP_ALL */
    int total_count;            /* total number of processes in this barrier */
    enum {
        PMIP_BARRIER_INCOMPLETE,
        PMIP_BARRIER_LOCAL_COMPLETE,
        PMIP_BARRIER_WAIT_UPSTREAM,
    } stage;
    struct pmip_barrier_epoch *epochs;  /* this can be replaced with `int count` if we do not need support epochs */
    UT_hash_handle hh;
};

HYD_status PMIP_barrier_create(struct pmip_pg *pg, const char *group_str, int proc,
                               struct pmip_barrier **barrier_out,
                               struct pmip_barrier_epoch **epoch_out);
HYD_status PMIP_barrier_find(struct pmip_pg *pg, const char *name, int proc,
                             struct pmip_barrier **barrier_out,
                             struct pmip_barrier_epoch **epoch_out);
HYD_status PMIP_barrier_epoch_in(struct pmip_barrier *barrier, struct pmip_barrier_epoch *epoch,
                                 int proc);
HYD_status PMIP_barrier_complete(struct pmip_pg *pg, struct pmip_barrier **barrier_p);

#endif /* PMIP_H_INCLUDED */
