/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PMI_V2_COMMON_H_INCLUDED
#define PMI_V2_COMMON_H_INCLUDED

#include "hydra.h"
#include "pmi_wire.h"

struct HYD_pmcd_pmi_v2_reqs {
    int fd;
    int pid;
    int pgid;
    const char *thrid;
    struct PMIU_cmd *pmi;
    const char *key;

    struct HYD_pmcd_pmi_v2_reqs *prev;
    struct HYD_pmcd_pmi_v2_reqs *next;
};

HYD_status HYD_pmcd_pmi_v2_queue_req(int fd, int pid, int pgid, struct PMIU_cmd *pmi,
                                     const char *key, struct HYD_pmcd_pmi_v2_reqs **pending_reqs);

#endif /* PMI_V2_COMMON_H_INCLUDED */
