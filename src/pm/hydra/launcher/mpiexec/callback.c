/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "mpiexec.h"
#include "csi.h"

HYD_CSI_Handle csi_handle;

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_LCHI_stdout_cb"
HYD_Status HYD_LCHI_stdout_cb(int fd, HYD_CSI_Event_t events)
{
    int count;
    char buf[HYD_CSI_TMPBUF_SIZE];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (events & HYD_CSI_IN) {
        HYDU_Error_printf("stdout handler got an stdin event: %d\n", events);
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    count = read(fd, buf, HYD_CSI_TMPBUF_SIZE);
    if (count < 0) {
        HYDU_Error_printf("socket read error on fd: %d (errno: %d)\n", fd, errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }
    else if (count == 0) {
        /* The connection has closed */
        status = HYD_CSI_Close_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("socket close error on fd: %d (errno: %d)\n", fd, errno);
            goto fn_fail;
        }
        goto fn_exit;
    }

    count = write(1, buf, count);
    if (count < 0) {
        HYDU_Error_printf("socket write error on fd: %d (errno: %d)\n", fd, errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_LCHI_stderr_cb"
HYD_Status HYD_LCHI_stderr_cb(int fd, HYD_CSI_Event_t events)
{
    int count;
    char buf[HYD_CSI_TMPBUF_SIZE];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (events & HYD_CSI_IN) {
        HYDU_Error_printf("stderr handler got an stdin event: %d\n", events);
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    count = read(fd, buf, HYD_CSI_TMPBUF_SIZE);
    if (count < 0) {
        HYDU_Error_printf("socket read error on fd: %d (errno: %d)\n", fd, errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }
    else if (count == 0) {
        /* The connection has closed */
        status = HYD_CSI_Close_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("socket close error on fd: %d (errno: %d)\n", fd, errno);
            goto fn_fail;
        }
        goto fn_exit;
    }

    count = write(2, buf, count);
    if (count < 0) {
        HYDU_Error_printf("socket write error on fd: %d (errno: %d)\n", fd, errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_LCHI_stdin_cb"
HYD_Status HYD_LCHI_stdin_cb(int fd, HYD_CSI_Event_t events)
{
    int count;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (events & HYD_CSI_IN) {
        HYDU_Error_printf("stdin handler got an stdout event: %d\n", events);
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    while (1) {
        /* If we already have buffered data, send it out */
        if (csi_handle.stdin_buf_count) {
            count = write(csi_handle.in, csi_handle.stdin_tmp_buf + csi_handle.stdin_buf_offset,
                          csi_handle.stdin_buf_count);
            if (count < 0) {
                /* We can't get an EAGAIN as we just got out of poll */
                HYDU_Error_printf("socket write error on fd: %d (errno: %d)\n", fd, errno);
                status = HYD_SOCK_ERROR;
                goto fn_fail;
            }
            csi_handle.stdin_buf_offset += count;
            csi_handle.stdin_buf_count -= count;
            break;
        }

        /* If we are still here, we need to refill our temporary buffer */
        count = read(0, csi_handle.stdin_tmp_buf, HYD_CSI_TMPBUF_SIZE);
        if (count < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                /* This call was interrupted or there was no data to read; just break out. */
                break;
            }

            HYDU_Error_printf("socket read error on fd: %d (errno: %d)\n", fd, errno);
            status = HYD_SOCK_ERROR;
            goto fn_fail;
        }
        else if (count == 0) {
            /* The connection has closed */
            status = HYD_CSI_Close_fd(fd);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("socket close error on fd: %d (errno: %d)\n", fd, errno);
                goto fn_fail;
            }
            break;
        }
        csi_handle.stdin_buf_count += count;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
