/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "pmiserv_pmi.h"
#include "pmi_v2_common.h"

HYD_status HYD_pmcd_pmi_v2_queue_req(int fd, int pid, int pgid, char *args[], char *key,
                                     struct HYD_pmcd_pmi_v2_reqs **pending_reqs)
{
    struct HYD_pmcd_pmi_v2_reqs *req, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC(req, struct HYD_pmcd_pmi_v2_reqs *, sizeof(struct HYD_pmcd_pmi_v2_reqs),
                status);
    req->fd = fd;
    req->pid = pid;
    req->pgid = pgid;
    req->prev = NULL;
    req->next = NULL;

    status = HYDU_strdup_list(args, &req->args);
    HYDU_ERR_POP(status, "unable to dup args\n");

    req->key = HYDU_strdup(key);

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
