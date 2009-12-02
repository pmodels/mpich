/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "rmki.h"
#include "rmk_pbs.h"

static int total_num_procs;

static HYD_status process_mfile_token(char *token, int newline)
{
    int num_procs;
    char *hostname, *procs;
    HYD_status status = HYD_SUCCESS;

    if (newline) {      /* The first entry gives the hostname and processes */
        hostname = strtok(token, ":");
        procs = strtok(NULL, ":");
        num_procs = procs ? atoi(procs) : 1;

        status = HYDU_add_to_node_list(hostname, num_procs, &HYD_handle.node_list);
        HYDU_ERR_POP(status, "unable to initialize proxy\n");

        total_num_procs += num_procs;
    }
    else {      /* Not a new line */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "token %s not supported at this time\n", token);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_rmkd_pbs_query_node_list(int *num_cores, struct HYD_proxy **proxy_list)
{
    char *hostfile;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    hostfile = getenv("PBS_NODEFILE");
    if (hostfile == NULL) {
        *proxy_list = NULL;
    }
    else {
        total_num_procs = 0;
        status = HYDU_parse_hostfile(hostfile, process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
        *num_cores = total_num_procs;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
