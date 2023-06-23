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

/* handlers for initialization via pmi port */
HYD_status fn_fullinit(struct pmip_downstream *p, struct PMIU_cmd *pmi);

/* handlers for client pmi commands */
HYD_status fn_init(struct pmip_downstream *p, struct PMIU_cmd *pmi);
HYD_status fn_get_maxes(struct pmip_downstream *p, struct PMIU_cmd *pmi);
HYD_status fn_get_appnum(struct pmip_downstream *p, struct PMIU_cmd *pmi);
HYD_status fn_get_my_kvsname(struct pmip_downstream *p, struct PMIU_cmd *pmi);
HYD_status fn_get_usize(struct pmip_downstream *p, struct PMIU_cmd *pmi);
HYD_status fn_get(struct pmip_downstream *p, struct PMIU_cmd *pmi);
HYD_status fn_put(struct pmip_downstream *p, struct PMIU_cmd *pmi);
HYD_status fn_barrier_in(struct pmip_downstream *p, struct PMIU_cmd *pmi);
HYD_status fn_finalize(struct pmip_downstream *p, struct PMIU_cmd *pmi);
HYD_status fn_info_putnodeattr(struct pmip_downstream *p, struct PMIU_cmd *pmi);
HYD_status fn_info_getnodeattr(struct pmip_downstream *p, struct PMIU_cmd *pmi);

/* handlers for server pmi commands */
HYD_status fn_keyval_cache(struct pmip_pg *pg, struct PMIU_cmd *pmi);
HYD_status fn_barrier_out(struct pmip_pg *pg, struct PMIU_cmd *pmi);

#endif /* PMIP_PMI_H_INCLUDED */
