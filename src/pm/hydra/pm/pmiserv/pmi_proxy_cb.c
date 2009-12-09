/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmi_proxy.h"
#include "ckpoint.h"

struct HYD_pmcd_pmip HYD_pmcd_pmip;

HYD_status HYD_pmcd_pmi_proxy_control_cmd_cb(int fd, HYD_event_t events, void *userp)
{
    int cmd_len;
    enum HYD_pmu_cmd cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We got a command from upstream */
    status = HYDU_sock_read(fd, &cmd, sizeof(enum HYD_pmu_cmd), &cmd_len,
                            HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading command from launcher\n");
    if (cmd_len == 0) {
        /* The connection has closed */
        status = HYDU_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");
        close(fd);
        goto fn_exit;
    }

    if (cmd == PROC_INFO) {
        status = HYD_pmcd_pmi_proxy_procinfo(fd);
        HYDU_ERR_POP(status, "error parsing process info\n");

        status = HYD_pmcd_pmi_proxy_launch_procs();
        HYDU_ERR_POP(status, "HYD_pmcd_pmi_proxy_launch_procs returned error\n");
    }
    else if (cmd == KILL_JOB) {
        HYD_pmcd_pmi_proxy_killjob();
        status = HYD_SUCCESS;
    }
    else if (cmd == PROXY_SHUTDOWN) {
        /* FIXME: shutdown should be handled more cleanly. That is,
         * check if there are other processes still running and kill
         * them before exiting. */
        exit(-1);
    }
    else if (cmd == CKPOINT) {
        HYDU_dump(stdout, "requesting checkpoint\n");
        status = HYDT_ckpoint_suspend();
        HYDU_dump(stdout, "checkpoint completed\n");
    }
    else {
        status = HYD_INTERNAL_ERROR;
    }

    HYDU_ERR_POP(status, "error handling proxy command\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_proxy_stdout_cb(int fd, HYD_event_t events, void *userp)
{
    int closed, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_forward_stdio(fd, STDOUT_FILENO, &closed);
    HYDU_ERR_POP(status, "stdout forwarding error\n");

    if (closed) {
        /* The connection has closed */
        status = HYDU_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
            if (HYD_pmcd_pmip.downstream.out[i] == fd)
                HYD_pmcd_pmip.downstream.out[i] = -1;

        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_pmcd_pmi_proxy_stderr_cb(int fd, HYD_event_t events, void *userp)
{
    int closed, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_forward_stdio(fd, STDERR_FILENO, &closed);
    HYDU_ERR_POP(status, "stderr forwarding error\n");

    if (closed) {
        /* The connection has closed */
        status = HYDU_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
            if (HYD_pmcd_pmip.downstream.err[i] == fd)
                HYD_pmcd_pmip.downstream.err[i] = -1;

        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_pmcd_pmi_proxy_stdin_cb(int fd, HYD_event_t events, void *userp)
{
    int closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_forward_stdio(fd, HYD_pmcd_pmip.downstream.in, &closed);
    HYDU_ERR_POP(status, "stdin forwarding error\n");

    if (closed) {
        status = HYDU_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        close(fd);

        close(HYD_pmcd_pmip.downstream.in);
        HYD_pmcd_pmip.downstream.in = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
