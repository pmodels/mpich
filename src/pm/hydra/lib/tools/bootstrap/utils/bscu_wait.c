/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "bscu.h"
#include <assert.h>

int *HYD_bscu_fd_list = NULL;
int HYD_bscu_fd_count = 0;

static struct HYD_proxy_pid *HYD_bscu_pid_list = NULL;
static int HYD_bscu_pid_count = 0;
static int HYD_bscu_pid_size = 0;

HYD_status HYDT_bscu_pid_list_grow(int add_count)
{
    HYD_status status = HYD_SUCCESS;

    HYD_bscu_pid_size += add_count;

    HYDU_REALLOC_OR_JUMP(HYD_bscu_pid_list, struct HYD_proxy_pid *,
                         HYD_bscu_pid_size * sizeof(struct HYD_proxy_pid), status);

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

void HYDT_bscu_pid_list_push(struct HYD_proxy *proxy, int pid)
{
    int i = HYD_bscu_pid_count;
    assert(i < HYD_bscu_pid_size);
    HYD_bscu_pid_list[i].proxy = proxy;
    HYD_bscu_pid_list[i].pid = pid;
    HYD_bscu_pid_count++;
}

struct HYD_proxy_pid *HYDT_bscu_pid_list_find(int pid)
{
    for (int i = 0; i < HYD_bscu_pid_count; i++) {
        if (HYD_bscu_pid_list[i].pid == pid) {
            return &HYD_bscu_pid_list[i];
        }
    }
    return NULL;
}

HYD_status HYDT_bscu_wait_for_completion(int timeout)
{
    int pgid, pid, ret, count, i, time_elapsed, time_left;
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
                        pgid = getpgid(HYD_bscu_pid_list[i].pid);
                        killpg(pgid, SIGKILL);
#else
                        kill(HYD_bscu_pid_list[i].pid, SIGKILL);
#endif
                    } else
                        time_left = timeout - time_elapsed;
                }

                status = HYDT_dmx_wait_for_event(time_left);
                HYDU_ERR_POP(status, "error waiting for event\n");

                /* Check if any processes terminated badly; if they
                 * did, return an error. */
                pid = waitpid(-1, &ret, WNOHANG);
                if (pid > 0) {
                    struct HYD_proxy_pid *p;
                    p = HYDT_bscu_pid_list_find(pid);
                    if (p) {
                        p->pid = -1;
                    }

                    if (ret) {
                        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                            "one of the processes terminated badly; aborting\n");
                    }
                }

                goto restart_wait;
            } else
                HYD_bscu_fd_list[i] = HYD_FD_CLOSED;
        }

        if (count == 0)
            break;
    }

    /* Loop till all processes have completed */
    while (1) {
        count = 0;
        for (i = 0; i < HYD_bscu_pid_count; i++)
            if (HYD_bscu_pid_list[i].pid != -1)
                count++;

        /* If there are no processes to wait, we are done */
        if (count == 0)
            break;

        pid = waitpid(-1, &ret, WNOHANG);
        if (pid > 0) {
            /* Find the pid and mark it as complete */
            for (i = 0; i < HYD_bscu_pid_count; i++)
                if (HYD_bscu_pid_list[i].pid == pid) {
                    HYD_bscu_pid_list[i].pid = -1;
                    break;
                }
        }
    }

    MPL_free(HYD_bscu_pid_list);
    HYD_bscu_pid_list = NULL;
    HYD_bscu_pid_count = 0;

    MPL_free(HYD_bscu_fd_list);
    HYD_bscu_fd_list = NULL;
    HYD_bscu_fd_count = 0;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
