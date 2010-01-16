/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMISERV_PMI_H_INCLUDED
#define PMISERV_PMI_H_INCLUDED

#include "hydra_base.h"
#include "demux.h"
#include "pmi_common.h"

/* PMI-1 specific definitions */
extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v1;

/* PMI-2 specific definitions */
extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v2;

struct HYD_pmcd_token_segment {
    int start_idx;
    int token_count;
};

struct HYD_pmcd_token {
    char *key;
    char *val;
};

struct HYD_pmcd_pmi_process {
    int fd;                     /* downstream fd to send the response on */
    int pid;                    /* unique id for the processes sharing the same fd */
    int rank;                   /* process rank */
    int epoch;                  /* Epoch this process has reached */
    struct HYD_proxy *proxy;    /* Back pointer to the proxy */
    struct HYD_pmcd_pmi_process *next;
};

struct HYD_pmcd_pmi_proxy_scratch {
    struct HYD_pmcd_pmi_process *process_list;
    struct HYD_pmcd_pmi_kvs *kvs;       /* Node-level KVS space for node attributes */
};

struct HYD_pmcd_pmi_pg_scratch {
    int barrier_count;

    int control_listen_fd;
    int pmi_listen_fd;

    struct HYD_pmcd_pmi_kvs *kvs;
};

HYD_status HYD_pmcd_pmi_id_to_rank(int id, int pgid, int *rank);
HYD_status HYD_pmcd_pmi_args_to_tokens(char *args[], struct HYD_pmcd_token **tokens,
                                       int *count);
void HYD_pmcd_pmi_free_tokens(struct HYD_pmcd_token *tokens, int token_count);
void HYD_pmcd_pmi_segment_tokens(struct HYD_pmcd_token *tokens, int token_count,
                                 struct HYD_pmcd_token_segment *segment_list,
                                 int *num_segments);
char *HYD_pmcd_pmi_find_token_keyval(struct HYD_pmcd_token *tokens, int count,
                                     const char *key);
HYD_status HYD_pmcd_pmi_add_kvs(const char *key, char *val, struct HYD_pmcd_pmi_kvs *kvs,
                                int *ret);
HYD_status HYD_pmcd_pmi_add_process_to_pg(struct HYD_pg *pg, int fd, int key, int rank);
struct HYD_pmcd_pmi_process *HYD_pmcd_pmi_find_process(int fd, int key);
HYD_status HYD_pmcd_pmi_process_mapping(struct HYD_pmcd_pmi_process *process,
                                        char **process_mapping);
HYD_status HYD_pmcd_pmi_finalize(void);

HYD_status HYD_pmcd_pmi_v1_cmd_response(int fd, int pid, const char *cmd, int cmd_len);

struct HYD_pmcd_pmi_handle {
    const char *cmd;
     HYD_status(*handler) (int fd, int pid, int pgid, char *args[]);
};

extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_handle;

#endif /* PMISERV_PMI_H_INCLUDED */
