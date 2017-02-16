/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_rmk_sge.h"
#include "hydra_err.h"
#include "hydra_hostfile.h"

static HYD_status process_mfile_tokens(char **tokens, struct HYD_node *node)
{
    HYD_status status = HYD_SUCCESS;

    MPL_strncpy(node->hostname, tokens[0], HYD_MAX_HOSTNAME_LEN);
    node->core_count = atoi(tokens[1]);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDI_rmk_sge_query_node_list(int *node_count, struct HYD_node **nodes)
{
    char *hostfile;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    MPL_env2str("PE_HOSTFILE", (const char **) &hostfile);

    if (hostfile == NULL) {
        *node_count = 0;
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "No SGE nodefile found\n");
    }
    else {
        status = HYD_hostfile_parse(hostfile, node_count, nodes, process_mfile_tokens);
        HYD_ERR_POP(status, "error parsing hostfile\n");
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
