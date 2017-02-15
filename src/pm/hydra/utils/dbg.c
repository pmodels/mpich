/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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

#ifdef USE_MEMORY_TRACING
    MPL_trinit(0, 0);
#endif

    if (gethostname(hostname, MAX_HOSTNAME_LEN) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "unable to get local host name\n");

    HYDU_MALLOC_OR_JUMP(HYD_dbg_prefix, char *, strlen(hostname) + 1 + strlen(str) + 1, status);
    MPL_snprintf(HYD_dbg_prefix, strlen(hostname) + 1 + strlen(str) + 1, "%s@%s", str, hostname);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDU_dbg_finalize(void)
{
    MPL_free(HYD_dbg_prefix);
}
