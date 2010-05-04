/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "rmki.h"
#include "rmk_lsf.h"

HYD_status HYDT_rmkd_lsf_query_node_list(struct HYD_node **node_list)
{
    char *hosts, *hostname, *num_procs_str;
    int num_procs;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (MPL_env2str("LSB_MCPU_HOSTS", (const char **) &hosts) == 0)
        hosts = NULL;

    if (hosts == NULL) {
        *node_list = NULL;
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "No LSF node list found\n");
    }
    else {
        while (1) {
            hostname = strtok(hosts, " ");
            if (hostname == NULL)
                break;

            num_procs_str = strtok(hosts, " ");
            HYDU_ASSERT(num_procs_str, status);

            num_procs = atoi(num_procs_str);

            status = HYDU_add_to_node_list(hostname, num_procs, node_list);
            HYDU_ERR_POP(status, "unable to add to node list\n");
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
