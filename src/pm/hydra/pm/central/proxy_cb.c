/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "proxy.h"
#include "demux.h"
#include "central.h"

struct HYD_Proxy_params HYD_Proxy_params;
int HYD_Proxy_listenfd;

HYD_Status HYD_Proxy_listen_cb(int fd, HYD_Event_t events)
{
    int accept_fd, count, i;
    enum HYD_Proxy_cmds cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (events & HYD_STDIN) {
        HYDU_Error_printf("stdout handler got an stdin event: %d\n", events);
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    if (fd == HYD_Proxy_listenfd) {     /* mpiexec is trying to connect */
        status = HYDU_Sock_accept(fd, &accept_fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("sock utils returned error when accepting connection\n");
            goto fn_fail;
        }

        status = HYD_DMX_Register_fd(1, &accept_fd, HYD_STDOUT, HYD_Proxy_listen_cb);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("demux engine returned error when registering fd\n");
            goto fn_fail;
        }
    }
    else {      /* We got a command from mpiexec */
        count = read(fd, &cmd, HYD_TMPBUF_SIZE);
        if (count < 0) {
            HYDU_Error_printf("socket read error on fd: %d (errno: %d)\n", fd, errno);
            status = HYD_SOCK_ERROR;
            goto fn_fail;
        }
        else if (count == 0) {
            /* The connection has closed */
            status = HYD_DMX_Deregister_fd(fd);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("demux engine returned error when deregistering fd\n");
                goto fn_fail;
            }
            close(fd);
            goto fn_exit;
        }

        if (cmd == KILLALL_PROCS) {     /* Got the killall command */
            for (i = 0; i < HYD_Proxy_params.proc_count; i++)
                if (HYD_Proxy_params.pid[i] != -1)
                    kill(HYD_Proxy_params.pid[i], SIGKILL);

            status = HYD_DMX_Deregister_fd(fd);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("demux engine returned error when deregistering fd\n");
                goto fn_fail;
            }
            close(fd);
        }
        else {
            HYDU_Error_printf("got unrecognized command from mpiexec\n");
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_Proxy_stdout_cb(int fd, HYD_Event_t events)
{
    int closed, i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_Sock_stdout_cb(fd, events, 1, &closed);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("sock stdout callback error\n");
        goto fn_fail;
    }

    if (closed) {
        /* The connection has closed */
        status = HYD_DMX_Deregister_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("demux engine returned error when deregistering fd\n");
            goto fn_fail;
        }

        for (i = 0; i < HYD_Proxy_params.proc_count; i++)
            if (HYD_Proxy_params.out[i] == fd)
                HYD_Proxy_params.out[i] = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_Proxy_stderr_cb(int fd, HYD_Event_t events)
{
    int closed, i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_Sock_stdout_cb(fd, events, 2, &closed);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("sock stdout callback error\n");
        goto fn_fail;
    }

    if (closed) {
        /* The connection has closed */
        status = HYD_DMX_Deregister_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("demux engine returned error when deregistering fd\n");
            goto fn_fail;
        }

        for (i = 0; i < HYD_Proxy_params.proc_count; i++)
            if (HYD_Proxy_params.err[i] == fd)
                HYD_Proxy_params.err[i] = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_Proxy_stdin_cb(int fd, HYD_Event_t events)
{
    int closed;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_Sock_stdin_cb(HYD_Proxy_params.in, events, HYD_Proxy_params.stdin_tmp_buf,
                                &HYD_Proxy_params.stdin_buf_count,
                                &HYD_Proxy_params.stdin_buf_offset, &closed);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("sock stdin callback error\n");
        goto fn_fail;
    }

    if (closed) {
        /* The connection has closed */
        status = HYD_DMX_Deregister_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("demux engine returned error when deregistering fd\n");
            goto fn_fail;
        }

        close(HYD_Proxy_params.in);
        close(fd);

        HYD_Proxy_params.in = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
