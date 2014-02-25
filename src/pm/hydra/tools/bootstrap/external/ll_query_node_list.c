/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"
#include "ll.h"

static int total_node_count = 0;

HYD_status HYDT_bscd_ll_query_node_list(struct HYD_node **node_list)
{
    char *hostfile;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (MPL_env2str("LOADL_HOSTFILE", (const char **) &hostfile) == 0)
        hostfile = NULL;

    if (hostfile == NULL) {
        *node_list = NULL;
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "No LL nodefile found\n");
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

static HYD_status process_mfile_count(char *token, int newline, struct HYD_node **node_list)
{
    HYD_status status = HYD_SUCCESS;

    if (newline)
        total_node_count++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDTI_bscd_ll_query_node_count(int *count)
{
    char *hostfile;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (MPL_env2str("LOADL_HOSTFILE", (const char **) &hostfile) == 0)
        hostfile = NULL;

    if (hostfile == NULL) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "No LL nodefile found\n");
    }
    else {
        total_node_count = 0;
        status = HYDU_parse_hostfile(hostfile, NULL, process_mfile_count);
        HYDU_ERR_POP(status, "error parsing hostfile\n");

        *count = total_node_count;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
