/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PMIP_PMI_H_INCLUDED
#define PMIP_PMI_H_INCLUDED

#include "hydra.h"
#include "pmiserv_common.h"
#include "pmip.h"

struct HYD_pmcd_pmip_pmi_handle {
    const char *cmd;
     HYD_status(*handler) (int fd, struct PMIU_cmd * pmi);
};

HYD_status fn_init(int fd, struct PMIU_cmd *pmi);
HYD_status fn_initack(int fd, struct PMIU_cmd *pmi);
HYD_status fn_get_maxes(int fd, struct PMIU_cmd *pmi);
HYD_status fn_get_appnum(int fd, struct PMIU_cmd *pmi);
HYD_status fn_get_my_kvsname(int fd, struct PMIU_cmd *pmi);
HYD_status fn_get_usize(int fd, struct PMIU_cmd *pmi);
HYD_status fn_get(int fd, struct PMIU_cmd *pmi);
HYD_status fn_put(int fd, struct PMIU_cmd *pmi);
HYD_status fn_barrier_in(int fd, struct PMIU_cmd *pmi);
HYD_status fn_finalize(int fd, struct PMIU_cmd *pmi);
HYD_status fn_fullinit(int fd, struct PMIU_cmd *pmi);
HYD_status fn_info_putnodeattr(int fd, struct PMIU_cmd *pmi);
HYD_status fn_info_getnodeattr(int fd, struct PMIU_cmd *pmi);

/* handlers for server pmi commands */
HYD_status fn_keyval_cache(struct pmip_pg *pg, struct PMIU_cmd *pmi);
HYD_status fn_barrier_out(struct pmip_pg *pg, struct PMIU_cmd *pmi);

#endif /* PMIP_PMI_H_INCLUDED */
