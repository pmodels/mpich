/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_err.h"

char HYD_print_prefix_str[DBG_PREFIX_LEN];

HYD_status HYD_print_set_prefix_str(const char *str)
{
    char hostname[HYD_MAX_HOSTNAME_LEN];
    HYD_status status = HYD_SUCCESS;

#ifdef USE_MEMORY_TRACING
    MPL_trinit(0, 0);
#endif

    if (gethostname(hostname, HYD_MAX_HOSTNAME_LEN) < 0)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "unable to get local host name\n");

    MPL_snprintf(HYD_print_prefix_str, strlen(hostname) + 1 + strlen(str) + 1, "%s@%s", str,
                 hostname);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
