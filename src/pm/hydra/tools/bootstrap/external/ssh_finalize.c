/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bscd_ssh_launcher_finalize(void)
{
    struct HYDT_bscd_ssh_time *e, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (e = HYDT_bscd_ssh_time; e;) {
        MPL_free(e->init_time);
        MPL_free(e->hostname);
        tmp = e->next;
        MPL_free(e);
        e = tmp;
    }

    HYDU_FUNC_EXIT();

    return status;
}
