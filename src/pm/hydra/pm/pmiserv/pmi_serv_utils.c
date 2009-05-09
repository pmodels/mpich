/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmi_serv.h"
#include "demux.h"

HYD_Handle handle;
struct HYD_PMCD_pmi_handle *HYD_PMCD_pmi_handle;

HYD_Status HYD_PMCD_pmi_send_exec_info(struct HYD_Partition *partition)
{
    enum HYD_PMCD_pmi_proxy_cmds cmd;
    int i, list_len, arg_len;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cmd = PROC_INFO;
    status = HYDU_sock_write(partition->control_fd, &cmd,
                             sizeof(enum HYD_PMCD_pmi_proxy_cmds));
    HYDU_ERR_POP(status, "unable to write data to proxy\n");

    /* Check how many arguments we have */
    list_len = HYDU_strlist_lastidx(partition->base->exec_args);
    status = HYDU_sock_write(partition->control_fd, &list_len, sizeof(int));
    HYDU_ERR_POP(status, "unable to write data to proxy\n");

    /* Convert the string list to parseable data and send */
    for (i = 0; partition->base->exec_args[i]; i++) {
        arg_len = strlen(partition->base->exec_args[i]) + 1;

        status = HYDU_sock_write(partition->control_fd, &arg_len, sizeof(int));
        HYDU_ERR_POP(status, "unable to write data to proxy\n");

        status = HYDU_sock_write(partition->control_fd, partition->base->exec_args[i],
                                 arg_len);
        HYDU_ERR_POP(status, "unable to write data to proxy\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
