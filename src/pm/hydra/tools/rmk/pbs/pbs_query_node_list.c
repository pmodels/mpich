/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "rmk.h"
#include "pbs_rmk.h"

HYD_status HYDT_rmki_pbs_query_node_list(struct HYD_node **node_list)
{
    char *hostfile;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    MPL_env2str("PBS_NODEFILE", (const char **) &hostfile);

    if (hostfile == NULL) {
        *node_list = NULL;
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "No PBS nodefile found\n");
    }
    else {
        status = HYDU_parse_hostfile(hostfile, node_list, HYDU_process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
