/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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

static void add_to_completed_list(int *count, int **completed_pids, int **completed_statuses, int pid, int exit_status) {
    HYD_status status = HYD_SUCCESS;

    *count = *count + 1;
    if ((*count) == 1) {
        HYDU_MALLOC_OR_JUMP(*completed_pids, int *, sizeof(int), status);
        HYDU_MALLOC_OR_JUMP(*completed_statuses, int *, sizeof(int), status);
    } else {
        HYDU_REALLOC_OR_JUMP(*completed_pids, int *, (*count) * sizeof(int), status);
        HYDU_REALLOC_OR_JUMP(*completed_statuses, int *, (*count) * sizeof(int), status);
    }
    (*completed_pids)[(*count)-1] = pid;
    (*completed_statuses)[(*count)-1] = exit_status;

  fn_exit:
    return;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bscu_wait_for_completion(int timeout, int *ncompleted, int **procs, int **exit_statuses)
{
    int pgid, pid, ret, count, i, time_elapsed, time_left, found;
    int completed_num = 0, *completed_pids = NULL, *completed_statuses = NULL;
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

                time_left = -1;
                if (timeout >= 0) {
                    if (time_elapsed > timeout) {
#if defined(HAVE_GETPGID) && defined(HAVE_SETSID)
                        /* If we are able to get the process group ID,
                         * send a signal to the entire process
                         * group */
                        pgid = getpgid(HYD_bscu_pid_list[i]);
                        killpg(pgid, SIGKILL);
#else
                        kill(HYD_bscu_pid_list[i], SIGKILL);
#endif
                    }
                    else
                        time_left = timeout - time_elapsed;
                }

                status = HYDT_dmx_wait_for_event(time_left);
                HYDU_ERR_POP(status, "error waiting for event\n");

                /* Check if any processes terminated badly; if they
                 * did, return an error. */
                pid = waitpid(-1, &ret, WNOHANG);
                if (pid > 0) {
                    add_to_completed_list(&completed_num, &completed_pids, &completed_statuses, pid, ret);
                    found = 0;
                    /* Find the pid and mark it as complete */
                    for (i = 0; i < HYD_bscu_pid_count; i++)
                        if (HYD_bscu_pid_list[i] == pid) {
                            HYD_bscu_pid_list[i] = -1;
                            found = 1;
                            break;
                        }

                    if (ret && found) {
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
            add_to_completed_list(&completed_num, &completed_pids, &completed_statuses, pid, ret);
            /* Find the pid and mark it as complete */
            for (i = 0; i < HYD_bscu_pid_count; i++)
                if (HYD_bscu_pid_list[i] == pid) {
                    HYD_bscu_pid_list[i] = -1;
                    break;
                }
        }
    }

    if (HYD_bscu_pid_list) {
        MPL_free(HYD_bscu_pid_list);
        HYD_bscu_pid_list = NULL;
        HYD_bscu_pid_count = 0;
    }

    if (HYD_bscu_fd_list) {
        MPL_free(HYD_bscu_fd_list);
        HYD_bscu_fd_list = NULL;
        HYD_bscu_fd_count = 0;
    }

    if (ncompleted)
        *ncompleted = completed_num;

    if (procs && completed_num) {
        HYDU_MALLOC_OR_JUMP(*procs, int *, completed_num * sizeof(int), status);
        memcpy(*procs, completed_pids, completed_num * sizeof(int));
    }

    if (exit_statuses && completed_num) {
        HYDU_MALLOC_OR_JUMP(*exit_statuses, int *, completed_num * sizeof(int), status);
        memcpy(*exit_statuses, completed_statuses, completed_num * sizeof(int));
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
