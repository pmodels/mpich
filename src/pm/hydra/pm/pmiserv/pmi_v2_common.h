/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_V2_COMMON_H_INCLUDED
#define PMI_V2_COMMON_H_INCLUDED

#include "hydra.h"

struct HYD_pmcd_pmi_v2_reqs {
    int fd;
    int pid;
    int pgid;
    char *thrid;
    char **args;
    char *key;

    struct HYD_pmcd_pmi_v2_reqs *prev;
    struct HYD_pmcd_pmi_v2_reqs *next;
};

HYD_status HYD_pmcd_pmi_v2_queue_req(int fd, int pid, int pgid, char *args[], char *key,
                                     struct HYD_pmcd_pmi_v2_reqs **pending_reqs);
void HYD_pmcd_pmi_v2_print_req_list(struct HYD_pmcd_pmi_v2_reqs *pending_reqs);

#endif /* PMI_V2_COMMON_H_INCLUDED */
