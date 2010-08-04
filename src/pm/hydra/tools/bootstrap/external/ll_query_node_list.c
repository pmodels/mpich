/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "ll.h"

int HYDT_bscd_ll_user_node_list = 1;

static struct HYD_node *global_node_list = NULL;
static int total_node_count = 0;

static HYD_status process_mfile_token(char *token, int newline)
{
    int num_procs;
    char *hostname, *procs;
    HYD_status status = HYD_SUCCESS;

    if (newline) {      /* The first entry gives the hostname and processes */
        hostname = strtok(token, ":");
        procs = strtok(NULL, ":");
        num_procs = procs ? atoi(procs) : 1;

        status = HYDU_add_to_node_list(hostname, num_procs, &global_node_list);
        HYDU_ERR_POP(status, "unable to initialize proxy\n");
    }
    else {      /* Not a new line */
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "token %s not supported at this time\n", token);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

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
        status = HYDU_parse_hostfile(hostfile, process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
    }
    *node_list = global_node_list;

    /* node list is provided by the bootstrap server */
    HYDT_bscd_ll_user_node_list = 0;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status process_mfile_count(char *token, int newline)
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
        status = HYDU_parse_hostfile(hostfile, process_mfile_count);
        HYDU_ERR_POP(status, "error parsing hostfile\n");

        *count = total_node_count;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
