/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_rmk_lsf.h"
#include "hydra_err.h"
#include "hydra_node.h"
#include "hydra_mem.h"

HYD_status HYDI_rmk_lsf_query_node_list(int *node_count, struct HYD_node **nodes)
{
    char *hosts, *hostname, *num_procs_str, *thosts = NULL;
    int num_procs;
    int nodes_alloc = 1;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_MALLOC(*nodes, struct HYD_node *, nodes_alloc * sizeof(struct HYD_node), status);

    MPL_env2str("LSB_MCPU_HOSTS", (const char **) &hosts);
    if (hosts == NULL) {
        *node_count = 0;
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "No LSF node list found\n");
    }
    else {
        hosts = MPL_strdup(hosts);
        thosts = hosts;

        hostname = strtok(hosts, " ");

        *node_count = 0;
        while (1) {
            if (hostname == NULL)
                break;

            /* the even fields in the list should be the number of
             * cores */
            num_procs_str = strtok(NULL, " ");
            HYD_ASSERT(num_procs_str, status);

            num_procs = atoi(num_procs_str);

            if (*node_count == nodes_alloc) {
                HYD_REALLOC(*nodes, struct HYD_node *, nodes_alloc * 2 * sizeof(struct HYD_node),
                            status);
                nodes_alloc *= 2;
            }

            MPL_strncpy((*nodes)[*node_count].hostname, hostname, HYD_MAX_HOSTNAME_LEN);
            (*nodes)[*node_count].core_count = num_procs;
            (*node_count)++;

            hostname = strtok(NULL, " ");
        }

        if (thosts)
            MPL_free(thosts);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
