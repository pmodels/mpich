/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "demux.h"

extern HYD_Handle handle;

static HYD_Status close_fd(int fd)
{
    struct HYD_Partition *partition;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Deregister the FD with the demux engine and close it. */
    status = HYD_DMX_deregister_fd(fd);
    HYDU_ERR_SETANDJUMP1(status, status, "error deregistering fd %d\n", fd);
    close(fd);

    /* Find the FD in the handle and remove it. */
    FORALL_ACTIVE_PARTITIONS(partition, handle.partition_list) {
        if (partition->base->out == fd) {
            partition->base->out = -1;
            goto fn_exit;
        }
        if (partition->base->err == fd) {
            partition->base->err = -1;
            goto fn_exit;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_UII_mpx_stdout_cb(int fd, HYD_Event_t events, void *userp)
{
    int closed;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Write output to fd 1 */
    status = HYDU_sock_stdout_cb(fd, events, 1, &closed);
    HYDU_ERR_SETANDJUMP2(status, status, "stdout callback error on fd %d: %s\n",
                         fd, HYDU_strerror(errno));

    if (closed) {
        status = close_fd(fd);
        HYDU_ERR_SETANDJUMP2(status, status, "socket close error on fd %d: %s\n",
                             fd, HYDU_strerror(errno));
        goto fn_exit;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_Status HYD_UII_mpx_stderr_cb(int fd, HYD_Event_t events, void *userp)
{
    int closed;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Write output to fd 2 */
    status = HYDU_sock_stdout_cb(fd, events, 2, &closed);
    HYDU_ERR_SETANDJUMP2(status, status, "stdout callback error on %d (%s)\n",
                         fd, HYDU_strerror(errno));

    if (closed) {
        status = close_fd(fd);
        HYDU_ERR_SETANDJUMP2(status, status, "socket close error on fd %d (%s)\n",
                             fd, HYDU_strerror(errno));
        goto fn_exit;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_UII_mpx_stdin_cb(int fd, HYD_Event_t events, void *userp)
{
    int closed;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_stdin_cb(fd, events, 0, handle.stdin_tmp_buf,
                                &handle.stdin_buf_count, &handle.stdin_buf_offset, &closed);
    HYDU_ERR_POP(status, "stdin callback error\n");

    if (closed) {
        /* Close the input handler for the process, so it knows that
         * we got a close event */
        status = close_fd(fd);
        HYDU_ERR_SETANDJUMP2(status, status, "socket close error on fd %d (errno: %d)\n",
                             fd, errno);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
