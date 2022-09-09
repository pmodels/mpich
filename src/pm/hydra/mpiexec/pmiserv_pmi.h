/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PMISERV_PMI_H_INCLUDED
#define PMISERV_PMI_H_INCLUDED

#include "hydra.h"
#include "demux.h"
#include "pmiserv_common.h"

/* PMI-1 specific definitions */
extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v1;

/* PMI-2 specific definitions */
extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v2;

struct HYD_pmcd_pmi_pg_scratch {
    /* PMI-1's PMI_Barrier is blocking, thus a single barrier_count works */
    int barrier_count;
    /* PMI-2's PMI2_KVS_Fence is non-blocking, thus need track epoch */
    int epoch;
    int fence_count;
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
    int keyval_dist_count;      /* Number of keyvals distributed */
};

struct HYD_proxy *HYD_pmcd_pmi_find_proxy(int fd);
HYD_status HYD_pmcd_pmi_finalize(void);

struct HYD_pmcd_pmi_handle {
    const char *cmd;
     HYD_status(*handler) (int fd, int pid, int pgid, struct PMIU_cmd * pmi);
};

extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_handle;

HYD_status HYD_pmiserv_pmi_reply(int fd, int pid, struct PMIU_cmd *pmi);
HYD_status HYD_pmiserv_bcast_keyvals(int fd, int pid);
HYD_status HYD_pmiserv_epoch_init(struct HYD_pg *pg);
HYD_status HYD_pmiserv_epoch_free(struct HYD_pg *pg);

HYD_status HYD_pmiserv_kvs_get(int fd, int pid, int pgid, struct PMIU_cmd *pmi, bool sync);
HYD_status HYD_pmiserv_kvs_put(int fd, int pid, int pgid, struct PMIU_cmd *pmi);
HYD_status HYD_pmiserv_kvs_mput(int fd, int pid, int pgid, struct PMIU_cmd *pmi);
HYD_status HYD_pmiserv_kvs_fence(int fd, int pid, int pgid, struct PMIU_cmd *pmi);

HYD_status HYD_pmiserv_barrier(int fd, int pid, int pgid, struct PMIU_cmd *pmi);
HYD_status HYD_pmiserv_abort(int fd, int pid, int pgid, struct PMIU_cmd *pmi);

HYD_status HYD_pmiserv_spawn(int fd, int pid, int pgid, struct PMIU_cmd *pmi);
HYD_status HYD_pmiserv_publish(int fd, int pid, int pgid, struct PMIU_cmd *pmi);
HYD_status HYD_pmiserv_unpublish(int fd, int pid, int pgid, struct PMIU_cmd *pmi);
HYD_status HYD_pmiserv_lookup(int fd, int pid, int pgid, struct PMIU_cmd *pmi);

#endif /* PMISERV_PMI_H_INCLUDED */
