/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"

HYD_status HYDT_bsci_launch_procs(const char *rmk, struct HYD_node **nodes, int num_nodes,
                                  char **args, int *control_fd)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_bsci_fns.launch_procs(rmk, nodes, num_nodes, args, control_fd);
    HYDU_ERR_POP(status, "launcher returned error while launching processes\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
