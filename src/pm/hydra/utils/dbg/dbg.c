/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"

char *HYD_dbg_prefix = (char *) "unknown";

HYD_status HYDU_dbg_init(const char *str)
{
    char hostname[MAX_HOSTNAME_LEN];
    HYD_status status = HYD_SUCCESS;

    HYDU_mem_init();

    status = HYDU_gethostname(hostname);
    HYDU_ERR_POP(status, "unable to get local host name\n");

    HYDU_MALLOC(HYD_dbg_prefix, char *, strlen(hostname) + 1 + strlen(str) + 1, status);
    HYDU_snprintf(HYD_dbg_prefix, strlen(hostname) + 1 + strlen(str) + 1, "%s@%s", str,
                  hostname);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDU_dbg_finalize(void)
{
    HYDU_FREE(HYD_dbg_prefix);
}
