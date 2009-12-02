/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmi_serv.h"

HYD_status HYD_pmcd_pmi_send_exec_info(struct HYD_proxy *proxy)
{
    enum HYD_pmcd_pmi_proxy_cmds cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cmd = PROC_INFO;
    status = HYDU_sock_write(proxy->control_fd, &cmd, sizeof(enum HYD_pmcd_pmi_proxy_cmds));
    HYDU_ERR_POP(status, "unable to write data to proxy\n");

    status = HYDU_send_strlist(proxy->control_fd, proxy->exec_launch_info);
    HYDU_ERR_POP(status, "error sending exec info\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
