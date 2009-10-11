/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "demux.h"
#include "pmi_proxy.h"

struct HYD_PMCD_pmi_proxy_params HYD_PMCD_pmi_proxy_params;

static HYD_Status wait_for_procs_to_finish(void)
{
    int i, out_count, err_count, count, pid, ret_status;
    HYD_Status status = HYD_SUCCESS;

    while (1) {
        /* Wait for some event to occur */
        status = HYD_DMX_wait_for_event(-1);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");

        /* Check to see if there's any open read socket left; if there
         * are, we will just wait for more events. */
        out_count = 0;
        err_count = 0;
        for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++) {
            if (HYD_PMCD_pmi_proxy_params.out[i] != -1)
                out_count++;
            if (HYD_PMCD_pmi_proxy_params.err[i] != -1)
                err_count++;

            if (out_count && err_count)
                break;
        }

        if (HYD_PMCD_pmi_proxy_params.procs_are_launched) {
            if (out_count == 0)
                close(HYD_PMCD_pmi_proxy_params.upstream.out);

            if (err_count == 0)
                close(HYD_PMCD_pmi_proxy_params.upstream.err);

            /* We are done */
            if (!out_count && !err_count)
                break;
        }
    }

    do {
        if (HYD_PMCD_pmi_proxy_params.procs_are_launched == 0)
            break;

        pid = waitpid(-1, &ret_status, 0);

        /* Find the pid and mark it as complete. */
        if (pid > 0)
            for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++)
                if (HYD_PMCD_pmi_proxy_params.pid[i] == pid)
                    HYD_PMCD_pmi_proxy_params.exit_status[i] = ret_status;

        /* Check how many more processes are pending */
        count = 0;
        for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++) {
            if (HYD_PMCD_pmi_proxy_params.exit_status[i] == -1) {
                count++;
                break;
            }
        }

        if (count == 0)
            break;

        /* Check if there are any messages from the launcher */
        status = HYD_DMX_wait_for_event(0);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");
    } while (1);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

int main(int argc, char **argv)
{
    int listenfd;
    HYD_Status status = HYD_SUCCESS;

    status = HYDU_dbg_init("proxy");
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

    status = HYD_PMCD_pmi_proxy_get_params(argv);
    HYDU_ERR_POP(status, "bad parameters passed to the proxy\n");

    /* Process launching only happens in the runtime case over here */
    if (HYD_PMCD_pmi_proxy_params.proxy.launch_mode == HYD_LAUNCH_RUNTIME) {
        HYD_PMCD_pmi_proxy_params.upstream.out = 1;
        HYD_PMCD_pmi_proxy_params.upstream.err = 2;
        HYD_PMCD_pmi_proxy_params.upstream.in = 0;

        /* Connect back upstream and the socket to a demux engine */
        status = HYDU_sock_connect(HYD_PMCD_pmi_proxy_params.proxy.server_name,
                                   HYD_PMCD_pmi_proxy_params.proxy.server_port,
                                   &HYD_PMCD_pmi_proxy_params.upstream.control);
        HYDU_ERR_POP(status, "unable to connect to the main server\n");

        status = HYDU_sock_write(HYD_PMCD_pmi_proxy_params.upstream.control,
                                 &HYD_PMCD_pmi_proxy_params.proxy.proxy_id,
                                 sizeof(HYD_PMCD_pmi_proxy_params.proxy.proxy_id));
        HYDU_ERR_POP(status, "unable to send the proxy ID to the server\n");

        status = HYD_DMX_register_fd(1, &HYD_PMCD_pmi_proxy_params.upstream.control,
                                     HYD_STDOUT, NULL, HYD_PMCD_pmi_proxy_control_cmd_cb);
        HYDU_ERR_POP(status, "unable to register fd\n");

        /* Now wait for the processes to finish */
        status = wait_for_procs_to_finish();
        HYDU_ERR_POP(status, "error waiting for processes to finish\n");

        /* Send the exit status upstream */
        status = HYDU_sock_write(HYD_PMCD_pmi_proxy_params.upstream.control,
                                 HYD_PMCD_pmi_proxy_params.exit_status,
                                 HYD_PMCD_pmi_proxy_params.exec_proc_count * sizeof(int));
        HYDU_ERR_POP(status, "unable to return exit status upstream\n");

        status = HYD_DMX_deregister_fd(HYD_PMCD_pmi_proxy_params.upstream.control);
        HYDU_ERR_POP(status, "unable to deregister fd\n");
        close(HYD_PMCD_pmi_proxy_params.upstream.control);

        /* cleanup the params structure for the next job */
        status = HYD_PMCD_pmi_proxy_cleanup_params();
        HYDU_ERR_POP(status, "unable to cleanup params\n");
    }
    else {      /* Persistent mode */
        status = HYDU_sock_listen(&listenfd, NULL,
                                  (uint16_t *) & HYD_PMCD_pmi_proxy_params.proxy.server_port);
        HYDU_ERR_POP(status, "unable to listen on socket\n");

        /* Register the listening socket with the demux engine */
        status = HYD_DMX_register_fd(1, &listenfd, HYD_STDOUT, NULL,
                                     HYD_PMCD_pmi_proxy_control_connect_cb);
        HYDU_ERR_POP(status, "unable to register fd\n");

        if (HYD_PMCD_pmi_proxy_params.proxy.launch_mode == HYD_LAUNCH_BOOT) {
            /* Spawn a persistent daemon proxy and exit parent proxy */
            status = HYDU_fork_and_exit(-1);
            HYDU_ERR_POP(status, "Error spawning persistent proxy\n");
        }

        do {
            /* Wait for the processes to finish. If there are no
             * processes, we will just wait blocking for the next job
             * to arrive. */
            status = wait_for_procs_to_finish();
            HYDU_ERR_POP(status, "error waiting for processes to finish\n");

            /* If processes had been launched and terminated, find the
             * exit status, return it and cleanup everything. */
            if (HYD_PMCD_pmi_proxy_params.procs_are_launched) {
                /* Send the exit status upstream */
                status = HYDU_sock_write(HYD_PMCD_pmi_proxy_params.upstream.control,
                                         HYD_PMCD_pmi_proxy_params.exit_status,
                                         HYD_PMCD_pmi_proxy_params.exec_proc_count *
                                         sizeof(int));
                HYDU_ERR_POP(status, "unable to return exit status upstream\n");

                status = HYD_DMX_deregister_fd(HYD_PMCD_pmi_proxy_params.upstream.control);
                HYDU_ERR_POP(status, "unable to deregister fd\n");
                close(HYD_PMCD_pmi_proxy_params.upstream.control);

                /* cleanup the params structure for the next job */
                status = HYD_PMCD_pmi_proxy_cleanup_params();
                HYDU_ERR_POP(status, "unable to cleanup params\n");
            }
        } while (1);
    }

  fn_exit:
    if (status != HYD_SUCCESS)
        return -1;
    else
        return 0;

  fn_fail:
    goto fn_exit;
}
