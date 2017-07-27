/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_node.h"
#include "hydra_mem.h"
#include "hydra_err.h"

HYD_status HYD_node_list_append(const char *hostname, int num_procs, struct HYD_node **nodes,
                                int *node_count, int *max_node_count)
{
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    if (*max_node_count == 0) {
        HYD_MALLOC(*nodes, struct HYD_node *, sizeof(struct HYD_node), status);
        MPL_VG_MEM_INIT(*nodes, sizeof(struct HYD_node));
        *max_node_count = 1;
    }
    else if (*node_count == *max_node_count) {
        HYD_REALLOC(*nodes, struct HYD_node *, (*max_node_count) * 2 * sizeof(struct HYD_node),
                    status);
        MPL_VG_MEM_INIT(*nodes + (*max_node_count), (*max_node_count) * sizeof(struct HYD_node));
        *max_node_count *= 2;
    }

    MPL_strncpy((*nodes)[*node_count].hostname, hostname, HYD_MAX_HOSTNAME_LEN);
    (*nodes)[*node_count].core_count = num_procs;
    (*nodes)[*node_count].node_id = *node_count;

    (*node_count)++;

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
