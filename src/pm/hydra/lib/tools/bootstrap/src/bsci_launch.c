/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "bsci.h"

HYD_status HYDT_bsci_launch_procs(int pgid, char **args, struct HYD_host *hosts,
                                  int num_hosts, int use_rmk, int k, int my_id, int *control_fd)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_bsci_fns.launch_procs(pgid, args, hosts, num_hosts, use_rmk, k, my_id,
                                        control_fd);
    HYDU_ERR_POP(status, "launcher returned error while launching processes\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
