/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"

HYD_status HYDT_bsci_launch_procs(char **args, struct HYD_node *node_list, int *control_fd,
                                  int enable_stdin)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_bsci_fns.launch_procs(args, node_list, control_fd, enable_stdin);
    HYDU_ERR_POP(status, "launcher returned error while launching processes\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
