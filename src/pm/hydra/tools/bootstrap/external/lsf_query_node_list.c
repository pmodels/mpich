/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"
#include "lsf.h"

HYD_status HYDT_bscd_lsf_query_node_list(struct HYD_node **node_list)
{
    char *hosts, *hostname, *num_procs_str, *thosts = NULL;
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
        hosts = HYDU_strdup(hosts);
        thosts = hosts;

        hostname = strtok(hosts, " ");
        while (1) {
            if (hostname == NULL)
                break;

            /* the even fields in the list should be the number of
             * cores */
            num_procs_str = strtok(NULL, " ");
            HYDU_ASSERT(num_procs_str, status);

            num_procs = atoi(num_procs_str);

            status = HYDU_add_to_node_list(hostname, num_procs, node_list);
            HYDU_ERR_POP(status, "unable to add to node list\n");

            hostname = strtok(NULL, " ");
        }

        if (thosts)
            HYDU_FREE(thosts);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
