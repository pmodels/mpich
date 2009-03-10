/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "csi.h"

HYD_Handle handle;

HYD_Status HYD_LCHI_stdout_cb(int fd, HYD_Event_t events)
{
    int closed;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Write output to fd 1 */
    status = HYDU_Sock_stdout_cb(fd, events, 1, &closed);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("socket stdout callback error on fd: %d (errno: %d)\n", fd, errno);
        goto fn_fail;
    }

    if (closed) {
        status = HYD_CSI_Close_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("socket close error on fd: %d (errno: %d)\n", fd, errno);
            goto fn_fail;
        }
        goto fn_exit;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_LCHI_stderr_cb(int fd, HYD_Event_t events)
{
    int count, closed;
    char buf[HYD_TMPBUF_SIZE];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Write output to fd 2 */
    status = HYDU_Sock_stdout_cb(fd, events, 2, &closed);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("socket stdout callback error on fd: %d (errno: %d)\n", fd, errno);
        goto fn_fail;
    }

    if (closed) {
        status = HYD_CSI_Close_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("socket close error on fd: %d (errno: %d)\n", fd, errno);
            goto fn_fail;
        }
        goto fn_exit;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_LCHI_stdin_cb(int fd, HYD_Event_t events)
{
    int count, closed;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_Sock_stdin_cb(handle.in, events, handle.stdin_tmp_buf,
                                &handle.stdin_buf_count, &handle.stdin_buf_offset, &closed);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("sock stdin callback returned an error\n");
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

    if (closed) {
        status = HYD_CSI_Close_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("socket close error on fd: %d (errno: %d)\n", fd, errno);
            goto fn_fail;
        }

        /* Close the input handler for the process, so it knows that
         * we got a close event */
        close(handle.in);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
