/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bscu.h"

int *HYD_bscu_fd_list = NULL;
int HYD_bscu_fd_count = 0;
int *HYD_bscu_pid_list = NULL;
int HYD_bscu_pid_count = 0;

HYD_status HYDT_bscu_wait_for_completion(int timeout)
{
    int pid, ret, count, i, time_elapsed, time_left;
    struct timeval start, now;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* FIXME: We rely on gettimeofday here. This needs to detect the
     * timer type available and use that. Probably more of an MPL
     * functionality than Hydra's. */
    gettimeofday(&start, NULL);

    /* Loop till all sockets have closed */
  restart_wait:
    while (1) {
        count = 0;
        for (i = 0; i < HYD_bscu_fd_count; i++) {
            if (HYD_bscu_fd_list[i] == HYD_FD_CLOSED)
                continue;

            ret = HYDT_dmx_query_fd_registration(HYD_bscu_fd_list[i]);
            if (ret) {  /* still registered */
                count++;        /* We still need to wait */

                gettimeofday(&now, NULL);
                time_elapsed = (now.tv_sec - start.tv_sec);     /* Ignore microsec granularity */

                if (timeout > 0) {
                    if (time_elapsed > timeout) {
                        status = HYD_TIMED_OUT;
                        goto fn_exit;
                    }
                    else
                        time_left = timeout - time_elapsed;
                }
                else
                    time_left = -1;

                status = HYDT_dmx_wait_for_event(time_left);
                HYDU_ERR_POP(status, "error waiting for event\n");

                /* Check if any processes terminated badly; if they
                 * did, return an error. */
                pid = waitpid(-1, &ret, WNOHANG);
                if (pid > 0) {
                    /* Find the pid and mark it as complete */
                    for (i = 0; i < HYD_bscu_pid_count; i++)
                        if (HYD_bscu_pid_list[i] == pid) {
                            HYD_bscu_pid_list[i] = -1;
                            break;
                        }

                    if (ret) {
                        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                            "one of the processes terminated badly; aborting\n");
                    }
                }

                goto restart_wait;
            }
            else
                HYD_bscu_fd_list[i] = HYD_FD_CLOSED;
        }

        if (count == 0)
            break;
    }

    /* Loop till all processes have completed */
    while (1) {
        count = 0;
        for (i = 0; i < HYD_bscu_pid_count; i++)
            if (HYD_bscu_pid_list[i] != -1)
                count++;

        /* If there are no processes to wait, we are done */
        if (count == 0)
            break;

        pid = waitpid(-1, &ret, WNOHANG);
        if (pid > 0) {
            /* Find the pid and mark it as complete */
            for (i = 0; i < HYD_bscu_pid_count; i++)
                if (HYD_bscu_pid_list[i] == pid) {
                    HYD_bscu_pid_list[i] = -1;
                    break;
                }
        }
    }

    if (HYD_bscu_pid_list) {
        HYDU_FREE(HYD_bscu_pid_list);
        HYD_bscu_pid_list = NULL;
        HYD_bscu_pid_count = 0;
    }

    if (HYD_bscu_fd_list) {
        HYDU_FREE(HYD_bscu_fd_list);
        HYD_bscu_fd_list = NULL;
        HYD_bscu_fd_count = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
