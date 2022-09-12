/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "bsci.h"
#include "pmiserv_common.h"
#include "pmi_v2_common.h"

HYD_status HYD_pmcd_pmi_v2_queue_req(int fd, int pid, int pgid, struct PMIU_cmd *pmi,
                                     const char *key, struct HYD_pmcd_pmi_v2_reqs **pending_reqs)
{
    struct HYD_pmcd_pmi_v2_reqs *req, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC_OR_JUMP(req, struct HYD_pmcd_pmi_v2_reqs *, sizeof(struct HYD_pmcd_pmi_v2_reqs),
                        status);
    req->fd = fd;
    req->pid = pid;
    req->pgid = pgid;
    req->prev = NULL;
    req->next = NULL;

    req->pmi = PMIU_cmd_dup(pmi);
    req->key = MPL_strdup(key);

    if (*pending_reqs == NULL)
        *pending_reqs = req;
    else {
        for (tmp = *pending_reqs; tmp->next; tmp = tmp->next);
        tmp->next = req;
        req->prev = tmp;
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
