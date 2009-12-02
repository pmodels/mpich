/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "demux.h"

HYD_status HYD_uii_mpx_stdout_cb(int fd, HYD_event_t events, void *userp)
{
    int closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_forward_stdio(fd, STDOUT_FILENO, &closed);
    HYDU_ERR_POP(status, "error forwarding to stdout\n");

    if (closed) {
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_SETANDJUMP1(status, status, "error deregistering fd %d\n", fd);

        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_uii_mpx_stderr_cb(int fd, HYD_event_t events, void *userp)
{
    int closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_forward_stdio(fd, STDERR_FILENO, &closed);
    HYDU_ERR_POP(status, "error forwarding to stdout\n");

    if (closed) {
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_SETANDJUMP1(status, status, "error deregistering fd %d\n", fd);

        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_uii_mpx_stdin_cb(int fd, HYD_event_t events, void *userp)
{
    int closed, in_fd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    in_fd = *((int *) userp);

    status = HYDU_sock_forward_stdio(STDIN_FILENO, in_fd, &closed);
    HYDU_ERR_POP(status, "stdin forwarding error\n");

    if (closed) {
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_SETANDJUMP1(status, status, "error deregistering fd %d\n", fd);

        close(STDIN_FILENO);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
