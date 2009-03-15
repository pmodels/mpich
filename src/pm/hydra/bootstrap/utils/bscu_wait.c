/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "bscu.h"

HYD_Handle handle;

/*
 * HYD_BSCU_wait_for_completion: We first wait for communication
 * events from the available processes till all connections have
 * closed. In the meanwhile, the SIGCHLD handler keeps track of all
 * the terminated processes.
 */
HYD_Status HYD_BSCU_wait_for_completion(void)
{
    int pid, ret_status, not_completed;
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    not_completed = 0;
    for (proc_params = handle.proc_params; proc_params; proc_params = proc_params->next)
        for (partition = proc_params->partition; partition; partition = partition->next)
            if (partition->exit_status == -1)
                not_completed++;

    /* We get here only after the I/O sockets have been closed. If the
     * application did not manually close its stdout and stderr
     * sockets, this means that the processes have terminated. In that
     * case the below loop will return almost immediately. If not, we
     * poll for some time, burning CPU. */
    do {
        pid = waitpid(-1, &ret_status, WNOHANG);
        if (pid > 0) {
            /* Find the pid and mark it as complete. */
            for (proc_params = handle.proc_params; proc_params;
                 proc_params = proc_params->next) {
                for (partition = proc_params->partition; partition;
                     partition = partition->next) {
                    if (partition->pid == pid) {
                        partition->exit_status = WEXITSTATUS(ret_status);
                        not_completed--;
                    }
                }
            }
        }
        if (HYDU_Time_left(handle.start, handle.timeout) == 0)
            break;

        /* FIXME: If we did not break out yet, add a small usleep to
         * yield CPU here. We can not just sleep for the remaining
         * time, as the timeout value might be large and the
         * application might exit much quicker. Note that the
         * sched_yield() call is broken on newer linux kernel versions
         * and should not be used. */
    } while (not_completed > 0);

    if (not_completed)
        status = HYD_INTERNAL_ERROR;

    HYDU_FUNC_EXIT();
    return status;
}
