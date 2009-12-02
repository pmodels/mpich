/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "demux.h"
#include "pmi_proxy.h"

struct HYD_pmcd_pmip HYD_pmcd_pmip;

int main(int argc, char **argv)
{
    int i, count, pid, ret_status;
    HYD_status status = HYD_SUCCESS;

    status = HYDU_dbg_init("proxy");
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

    status = HYD_pmcd_pmi_proxy_get_params(argv);
    HYDU_ERR_POP(status, "bad parameters passed to the proxy\n");

    /* Connect back upstream and the socket to a demux engine */
    status = HYDU_sock_connect(HYD_pmcd_pmip.upstream.server_name,
                               HYD_pmcd_pmip.upstream.server_port,
                               &HYD_pmcd_pmip.upstream.control);
    HYDU_ERR_POP(status, "unable to connect to the main server\n");

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control,
                             &HYD_pmcd_pmip.local.id, sizeof(HYD_pmcd_pmip.local.id));
    HYDU_ERR_POP(status, "unable to send the proxy ID to the server\n");

    status = HYDT_dmx_register_fd(1, &HYD_pmcd_pmip.upstream.control,
                                  HYD_STDOUT, NULL, HYD_pmcd_pmi_proxy_control_cmd_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    while (1) {
        /* Wait for some event to occur */
        status = HYDT_dmx_wait_for_event(-1);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");

        /* Check to see if there's any open read socket left; if there
         * are, we will just wait for more events. */
        count = 0;
        for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
            if (HYD_pmcd_pmip.downstream.out[i] != -1)
                count++;
            if (HYD_pmcd_pmip.downstream.err[i] != -1)
                count++;

            if (count)
                break;
        }
        if (!count)
            break;
    }

    /* Now wait for the processes to finish */
    while (1) {
        pid = waitpid(-1, &ret_status, 0);

        /* Find the pid and mark it as complete. */
        if (pid > 0)
            for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
                if (HYD_pmcd_pmip.downstream.pid[i] == pid)
                    HYD_pmcd_pmip.downstream.exit_status[i] = ret_status;

        /* Check how many more processes are pending */
        count = 0;
        for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
            if (HYD_pmcd_pmip.downstream.exit_status[i] == -1) {
                count++;
                break;
            }
        }

        if (count == 0)
            break;

        /* Check if there are any messages from the launcher */
        status = HYDT_dmx_wait_for_event(0);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");
    }

    /* Send the exit status upstream */
    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control,
                             HYD_pmcd_pmip.downstream.exit_status,
                             HYD_pmcd_pmip.local.proxy_process_count * sizeof(int));
    HYDU_ERR_POP(status, "unable to return exit status upstream\n");

    status = HYDT_dmx_deregister_fd(HYD_pmcd_pmip.upstream.control);
    HYDU_ERR_POP(status, "unable to deregister fd\n");
    close(HYD_pmcd_pmip.upstream.control);

    /* cleanup the params structure for the next job */
    status = HYD_pmcd_pmi_proxy_cleanup_params();
    HYDU_ERR_POP(status, "unable to cleanup params\n");

  fn_exit:
    if (status != HYD_SUCCESS)
        return -1;
    else
        return 0;

  fn_fail:
    goto fn_exit;
}
