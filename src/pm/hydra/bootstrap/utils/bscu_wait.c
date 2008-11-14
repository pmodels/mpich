/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_mem.h"
#include "bsci.h"
#include "bscu.h"

HYD_Handle handle;

/*
 * HYD_BSCU_Wait_for_completion: We first wait for communication
 * events from the available processes till all connections have
 * closed. In the meanwhile, the SIGCHLD handler keeps track of all
 * the terminated processes.
 */
HYD_Status HYD_BSCU_Wait_for_completion(void)
{
    int pid, ret_status, i, not_completed;
    struct HYD_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    not_completed = 0;
    proc_params = handle.proc_params;
    while (proc_params) {
        for (i = 0; i < proc_params->user_num_procs; i++)
            if (proc_params->exit_status_valid[i] == 0)
                not_completed++;
        proc_params = proc_params->next;
    }

    /* We get here only after the I/O sockets have been closed. If the
     * application did not manually close its stdout and stderr
     * sockets, this means that the processes have terminated. That
     * means the below loop will return almost immediately. If not, we
     * poll for some time, burning CPU. */
    do {
        pid = waitpid(-1, &ret_status, WNOHANG);
        if (pid > 0) {
            /* Find the pid and mark it as complete. */
            proc_params = handle.proc_params;
            while (proc_params) {
                for (i = 0; i < proc_params->user_num_procs; i++) {
                    if (proc_params->pid[i] == pid) {
                        proc_params->exit_status[i] = WEXITSTATUS(ret_status);
                        proc_params->exit_status_valid[i] = 1;
                        not_completed--;
                    }
                }
                proc_params = proc_params->next;
            }
        }
        if (HYD_CSU_Time_left() == 0)
            break;
    } while (not_completed > 0);

    if (not_completed) {
        status = HYD_BSCI_Cleanup_procs();
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("bootstrap process cleanup failed\n");
            goto fn_fail;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
