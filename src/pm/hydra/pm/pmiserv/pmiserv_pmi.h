/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMISERV_PMI_H_INCLUDED
#define PMISERV_PMI_H_INCLUDED

#include "hydra.h"
#include "demux.h"
#include "common.h"

/* PMI-1 specific definitions */
extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v1;

/* PMI-2 specific definitions */
extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v2;

struct HYD_pmcd_token_segment {
    int start_idx;
    int token_count;
};

struct HYD_pmcd_pmi_pg_scratch {
    int barrier_count;
    struct HYD_pmcd_pmi_ecount {
        int fd;
        int pid;
        int epoch;
    } *ecount;

    int control_listen_fd;
    int pmi_listen_fd;

    char *dead_processes;
    int dead_process_count;

    struct HYD_pmcd_pmi_kvs *kvs;
};

struct HYD_pmcd_pmi_publish {
    char *name;
    char *port;
    int infokeycount;

    struct HYD_pmcd_pmi_info_keys {
        char *key;
        char *val;
    } *info_keys;

    struct HYD_pmcd_pmi_publish *next;
};

struct HYD_proxy *HYD_pmcd_pmi_find_proxy(int fd);
HYD_status HYD_pmcd_pmi_finalize(void);
HYD_status HYD_pmcd_pmi_free_publish(struct HYD_pmcd_pmi_publish *publish);
HYD_status HYD_pmcd_pmi_publish(char *name, char *port, int *success);
HYD_status HYD_pmcd_pmi_unpublish(char *name, int *success);
HYD_status HYD_pmcd_pmi_lookup(char *name, char **value);

struct HYD_pmcd_pmi_handle {
    const char *cmd;
     HYD_status(*handler) (int fd, int pid, int pgid, char *args[]);
};

extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_handle;
extern struct HYD_pmcd_pmi_publish *HYD_pmcd_pmi_publish_list;

#endif /* PMISERV_PMI_H_INCLUDED */
