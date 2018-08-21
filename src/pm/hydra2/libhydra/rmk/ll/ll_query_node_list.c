/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_rmk_ll.h"
#include "hydra_err.h"
#include "hydra_hostfile.h"

HYD_status HYDI_rmk_ll_query_node_list(int *node_count, struct HYD_node **nodes)
{
    char *hostfile;
    char *dummy;
    int nprocs;
    int node_num = 0;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    if (MPL_env2str("LOADL_TOTAL_TASKS", (const char **) &dummy) == 0) {
        *node_count = 0;
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "No LL total tasks information\n");
    }
    nprocs = atoi(dummy);

    MPL_env2str("LOADL_HOSTFILE", (const char **) &hostfile);

    if (hostfile == NULL) {
        *node_count = 0;
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "No LL nodefile found\n");
    } else {
        status = HYD_hostfile_parse(hostfile, node_count, nodes, HYD_hostfile_process_tokens);
        HYD_ERR_POP(status, "error parsing hostfile\n");
    }


    if (*node_count > 0) {
        for (i = 0; i < *node_count; i++) {
            (*nodes)[i].core_count =
                (nprocs >
                 (*node_count)) ? (nprocs / (*node_count) +
                                   ((node_num < nprocs % (*node_count)) ? 1 : 0)) : 1;
            node_num++;
        }
    } else {
        (*nodes)[0].core_count = nprocs;
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
