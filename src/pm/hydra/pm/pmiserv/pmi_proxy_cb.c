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

extern struct HYD_PMCD_pmi_proxy_params HYD_PMCD_pmi_proxy_params;
int HYD_PMCD_pmi_proxy_listenfd;

HYD_Status HYD_PMCD_pmi_proxy_listen_cb(int fd, HYD_Event_t events, void *userp)
{
    int accept_fd, cmd_len;
    char cmd[HYD_PMCD_MAX_CMD_LEN];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (events & HYD_STDIN)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "stdout handler got stdin event\n");

    if (fd == HYD_PMCD_pmi_proxy_listenfd) {    /* mpiexec is trying to connect */
        status = HYDU_sock_accept(fd, &accept_fd);
        HYDU_ERR_POP(status, "accept error\n");

        status =
            HYD_DMX_register_fd(1, &accept_fd, HYD_STDOUT, NULL, HYD_PMCD_pmi_proxy_listen_cb);
        HYDU_ERR_POP(status, "unable to register fd\n");
    }
    else {      /* We got a command from mpiexec */
        status = HYDU_sock_readline(fd, cmd, HYD_PMCD_MAX_CMD_LEN, &cmd_len);
        HYDU_ERR_POP(status, "Error reading command from proxy");
        if (cmd_len == 0) {
            /* The connection has closed */
            status = HYD_DMX_deregister_fd(fd);
            HYDU_ERR_POP(status, "unable to deregister fd\n");
            close(fd);
            goto fn_exit;
        }
        status = HYD_PMCD_pmi_proxy_handle_cmd(fd, cmd, cmd_len);
        HYDU_ERR_POP(status, "Error handling proxy command\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

HYD_Status HYD_PMCD_pmi_proxy_rstdout_cb(int fd, HYD_Event_t events, void *userp)
{
    int closed, i;
    HYD_Status status = HYD_SUCCESS;
    struct HYD_PMCD_pmi_proxy_params *proxy_params;

    HYDU_FUNC_ENTER();
    proxy_params = (struct HYD_PMCD_pmi_proxy_params *) userp;

    status = HYDU_sock_stdout_cb(fd, events, proxy_params->rproxy_connfd, &closed);
    HYDU_ERR_POP(status, "stdout callback error\n");

    if (closed) {
        int all_procs_exited = 1;
        /* The process exited */
        status = HYD_DMX_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        /* FIXME: This could be a perf killer if we have a lot of procs associated with
         * the same job on a single proxy
         */
        for (i = 0; i < proxy_params->exec_proc_count; i++) {
            int ret_status = 0;
            if (proxy_params->out[i] == fd) {
                waitpid(proxy_params->pid[i], &ret_status, WUNTRACED);
                close(proxy_params->in);
                proxy_params->out[i] = -1;
                proxy_params->err[i] = -1;
            }
            if (proxy_params->out[i] != -1)
                all_procs_exited = 0;
        }
        if (all_procs_exited) {
            close(proxy_params->rproxy_connfd);
            status = HYD_DMX_deregister_fd(proxy_params->rproxy_connfd);
            HYDU_ERR_POP(status, "Error deregistering remote job conn fd\n");
            status = HYD_PMCD_pmi_proxy_cleanup_params(proxy_params);
            HYDU_ERR_POP(status, "Error cleaning up proxy params\n");
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_Status HYD_PMCD_pmi_proxy_remote_cb(int fd, HYD_Event_t events, void *userp)
{
    int closed = 0, i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();
    /* FIXME: This cb should take care of the commands from mpiexec */

    if (closed) {
        /* The connection has closed */
        status = HYD_DMX_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++)
            if (HYD_PMCD_pmi_proxy_params.out[i] == fd)
                HYD_PMCD_pmi_proxy_params.out[i] = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_Status HYD_PMCD_pmi_proxy_stdout_cb(int fd, HYD_Event_t events, void *userp)
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

        for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++)
            if (HYD_PMCD_pmi_proxy_params.out[i] == fd)
                HYD_PMCD_pmi_proxy_params.out[i] = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_proxy_stderr_cb(int fd, HYD_Event_t events, void *userp)
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

        for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++)
            if (HYD_PMCD_pmi_proxy_params.err[i] == fd)
                HYD_PMCD_pmi_proxy_params.err[i] = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_proxy_stdin_cb(int fd, HYD_Event_t events, void *userp)
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
