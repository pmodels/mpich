/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmi_proxy.h"
#include "demux.h"
#include "pmi_serv.h"

struct HYD_PMCD_pmi_proxy_params HYD_PMCD_pmi_proxy_params;
int HYD_PMCD_pmi_proxy_listenfd;

HYD_Status HYD_PMCD_pmi_proxy_listen_cb(int fd, HYD_Event_t events)
{
    int accept_fd, count, i;
    enum HYD_PMCD_pmi_proxy_cmds cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (events & HYD_STDIN)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "stdout handler got stdin event\n");

    if (fd == HYD_PMCD_pmi_proxy_listenfd) {    /* mpiexec is trying to connect */
        status = HYDU_sock_accept(fd, &accept_fd);
        HYDU_ERR_POP(status, "accept error\n");

        status = HYD_DMX_register_fd(1, &accept_fd, HYD_STDOUT, HYD_PMCD_pmi_proxy_listen_cb);
        HYDU_ERR_POP(status, "unable to register fd\n");
    }
    else {      /* We got a command from mpiexec */
        count = read(fd, &cmd, sizeof(enum HYD_PMCD_pmi_proxy_cmds));
        if (count < 0) {
            HYDU_ERR_SETANDJUMP2(status, HYD_SOCK_ERROR, "read error on %d (%s)\n",
                                 fd, HYDU_strerror(errno));
        }
        else if (count == 0) {
            /* The connection has closed */
            status = HYD_DMX_deregister_fd(fd);
            HYDU_ERR_POP(status, "unable to deregister fd\n");
            close(fd);
            goto fn_exit;
        }

        if (cmd == KILLALL_PROCS) {     /* Got the killall command */
            for (i = 0; i < HYD_PMCD_pmi_proxy_params.partition_proc_count; i++)
                if (HYD_PMCD_pmi_proxy_params.pid[i] != -1)
                    kill(HYD_PMCD_pmi_proxy_params.pid[i], SIGKILL);

            status = HYD_DMX_deregister_fd(fd);
            HYDU_ERR_POP(status, "unable to register fd\n");
            close(fd);
        }
        else {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "got unrecognized command from mpiexec\n");
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_proxy_stdout_cb(int fd, HYD_Event_t events)
{
    int closed, i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_stdout_cb(fd, events, 1, &closed);
    HYDU_ERR_POP(status, "stdout callback error\n");

    if (closed) {
        /* The connection has closed */
        status = HYD_DMX_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        for (i = 0; i < HYD_PMCD_pmi_proxy_params.partition_proc_count; i++)
            if (HYD_PMCD_pmi_proxy_params.out[i] == fd)
                HYD_PMCD_pmi_proxy_params.out[i] = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_proxy_stderr_cb(int fd, HYD_Event_t events)
{
    int closed, i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_stdout_cb(fd, events, 2, &closed);
    HYDU_ERR_POP(status, "stdout callback error\n");

    if (closed) {
        /* The connection has closed */
        status = HYD_DMX_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        for (i = 0; i < HYD_PMCD_pmi_proxy_params.partition_proc_count; i++)
            if (HYD_PMCD_pmi_proxy_params.err[i] == fd)
                HYD_PMCD_pmi_proxy_params.err[i] = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_proxy_stdin_cb(int fd, HYD_Event_t events)
{
    int closed;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_stdin_cb(HYD_PMCD_pmi_proxy_params.in, events,
                                HYD_PMCD_pmi_proxy_params.stdin_tmp_buf,
                                &HYD_PMCD_pmi_proxy_params.stdin_buf_count,
                                &HYD_PMCD_pmi_proxy_params.stdin_buf_offset, &closed);
    HYDU_ERR_POP(status, "stdin callback error\n");

    if (closed) {
        /* The connection has closed */
        status = HYD_DMX_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        close(HYD_PMCD_pmi_proxy_params.in);
        close(fd);

        HYD_PMCD_pmi_proxy_params.in = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
