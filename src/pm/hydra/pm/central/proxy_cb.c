/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_sig.h"
#include "proxy.h"
#include "central.h"

struct HYD_Proxy_params HYD_Proxy_params;
int HYD_Proxy_listenfd;

/* FIXME: A lot of this content is copied from the mpiexec
 * callback. This needs to be moved to utility functions instead. */

HYD_Status HYD_Proxy_listen_cb(int fd, HYD_Event_t events)
{
    int accept_fd, count, i;
    enum HYD_Proxy_cmds cmd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (events & HYD_STDIN) {
        HYDU_Error_printf("stdout handler got an stdin event: %d\n",
                          events);
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    if (fd == HYD_Proxy_listenfd) {     /* mpiexec is trying to connect */
        status = HYDU_Sock_accept(fd, &accept_fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf
                ("sock utils returned error when accepting connection\n");
            goto fn_fail;
        }

        status =
            HYD_DMX_Register_fd(1, &accept_fd, HYD_STDOUT,
                                HYD_Proxy_listen_cb);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf
                ("demux engine returned error when registering fd\n");
            goto fn_fail;
        }
    }
    else {      /* We got a command from mpiexec */
        count = read(fd, &cmd, HYD_TMPBUF_SIZE);
        if (count < 0) {
            HYDU_Error_printf("socket read error on fd: %d (errno: %d)\n",
                              fd, errno);
            status = HYD_SOCK_ERROR;
            goto fn_fail;
        }
        else if (count == 0) {
            /* The connection has closed */
            status = HYD_DMX_Deregister_fd(fd);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf
                    ("demux engine returned error when deregistering fd\n");
                goto fn_fail;
            }
            goto fn_exit;
        }

        if (cmd == KILLALL_PROCS) {     /* Got the killall command */
            for (i = 0; i < HYD_Proxy_params.proc_count; i++)
                kill(HYD_Proxy_params.pid[i], SIGKILL);
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
    int count;
    char buf[HYD_TMPBUF_SIZE];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (events & HYD_STDIN) {
        HYDU_Error_printf("stdout handler got an stdin event: %d\n",
                          events);
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    count = read(fd, buf, HYD_TMPBUF_SIZE);
    if (count < 0) {
        HYDU_Error_printf("socket read error on fd: %d (errno: %d)\n", fd,
                          errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }
    else if (count == 0) {
        /* The connection has closed */
        status = HYD_DMX_Deregister_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf
                ("demux engine returned error when deregistering fd\n");
            goto fn_fail;
        }
        goto fn_exit;
    }

    count = write(1, buf, count);
    if (count < 0) {
        HYDU_Error_printf("socket write error on fd: %d (errno: %d)\n", fd,
                          errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_Proxy_stderr_cb(int fd, HYD_Event_t events)
{
    int count;
    char buf[HYD_TMPBUF_SIZE];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (events & HYD_STDIN) {
        HYDU_Error_printf("stderr handler got an stdin event: %d\n",
                          events);
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    count = read(fd, buf, HYD_TMPBUF_SIZE);
    if (count < 0) {
        HYDU_Error_printf("socket read error on fd: %d (errno: %d)\n", fd,
                          errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }
    else if (count == 0) {
        /* The connection has closed */
        status = HYD_DMX_Deregister_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf
                ("demux engine returned error when deregistering fd\n");
            goto fn_fail;
        }
        goto fn_exit;
    }

    count = write(2, buf, count);
    if (count < 0) {
        HYDU_Error_printf("socket write error on fd: %d (errno: %d)\n", fd,
                          errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_Proxy_stdin_cb(int fd, HYD_Event_t events)
{
    int count;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (events & HYD_STDIN) {
        HYDU_Error_printf
            ("stdin handler got a writeable event on local stdin: %d\n",
             events);
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    while (1) {
        /* If we already have buffered data, send it out */
        if (HYD_Proxy_params.stdin_buf_count) {
            count =
                write(HYD_Proxy_params.in,
                      HYD_Proxy_params.stdin_tmp_buf +
                      HYD_Proxy_params.stdin_buf_offset,
                      HYD_Proxy_params.stdin_buf_count);
            if (count < 0) {
                /* We can't get an EAGAIN as we just got out of poll */
                HYDU_Error_printf
                    ("socket write error on fd: %d (errno: %d)\n",
                     HYD_Proxy_params.in, errno);
                status = HYD_SOCK_ERROR;
                goto fn_fail;
            }
            HYD_Proxy_params.stdin_buf_offset += count;
            HYD_Proxy_params.stdin_buf_count -= count;
            break;
        }

        /* If we are still here, we need to refill our temporary buffer */
        count = read(0, HYD_Proxy_params.stdin_tmp_buf, HYD_TMPBUF_SIZE);
        if (count < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                /* This call was interrupted or there was no data to read; just break out. */
                break;
            }

            HYDU_Error_printf("socket read error on fd: %d (errno: %d)\n",
                              fd, errno);
            status = HYD_SOCK_ERROR;
            goto fn_fail;
        }
        else if (count == 0) {
            /* The connection has closed */
            status = HYD_DMX_Deregister_fd(fd);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf
                    ("demux engine returned error when deregistering fd\n");
                goto fn_fail;
            }
            break;
        }
        HYD_Proxy_params.stdin_buf_count += count;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
