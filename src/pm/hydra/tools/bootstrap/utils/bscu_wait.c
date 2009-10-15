/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bscu.h"

HYD_Status HYD_BSCU_wait_for_completion(struct HYD_Proxy *proxy_list)
{
    int pid, ret_status, not_completed;
    struct HYD_Proxy *proxy;
    struct HYD_Proxy_exec *exec;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    not_completed = 0;
    FORALL_ACTIVE_PROXIES(proxy, proxy_list) {
        if (proxy->exit_status == NULL) {
            for (exec = proxy->exec_list; exec; exec = exec->next)
                not_completed += exec->proc_count;
        }
    }

    /* We get here only after the I/O sockets have been closed. If the
     * application did not manually close its stdout and stderr
     * sockets, this means that the processes have terminated. In that
     * case the below loop will return almost immediately. If not, we
     * poll for some time, burning CPU. */
    while (not_completed > 0) {
        pid = waitpid(-1, &ret_status, WNOHANG);
        if (pid > 0) {
            /* Find the pid and mark it as complete. */
            FORALL_ACTIVE_PROXIES(proxy, proxy_list) {
                if (proxy->pid == pid)
                    not_completed--;
            }
        }
    }

    if (not_completed)
        status = HYD_INTERNAL_ERROR;

    HYDU_FUNC_EXIT();
    return status;
}
