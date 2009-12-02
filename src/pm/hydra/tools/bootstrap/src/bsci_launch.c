/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"

HYD_status HYDT_bsci_launch_procs(
    char **args, struct HYD_node *node_list, void *userp,
    HYD_status(*stdin_cb) (int fd, HYD_event_t events, void *userp),
    HYD_status(*stdout_cb) (int fd, HYD_event_t events, void *userp),
    HYD_status(*stderr_cb) (int fd, HYD_event_t events, void *userp))
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_bsci_fns.launch_procs(args, node_list, userp, stdin_cb, stdout_cb, stderr_cb);
    HYDU_ERR_POP(status, "bootstrap device returned error while launching processes\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
