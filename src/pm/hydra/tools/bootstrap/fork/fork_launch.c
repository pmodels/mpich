/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "fork.h"

static int fd_stdin, fd_stdout, fd_stderr;

HYD_status HYDT_bscd_fork_launch_procs(char **args, struct HYD_node *node_list,
                                       HYD_status(*stdout_cb) (void *buf, int buflen),
                                       HYD_status(*stderr_cb) (void *buf, int buflen))
{
    int num_hosts, idx, i, fd;
    int *pid, *fd_list;
    struct HYD_node *node;
    char *targs[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    idx = 0;
    for (i = 0; args[i]; i++)
        targs[idx++] = HYDU_strdup(args[i]);

    /* pid_list might already have some PIDs */
    num_hosts = 0;
    for (node = node_list; node; node = node->next)
        num_hosts++;

    /* Increase pid list to accommodate these new pids */
    HYDU_MALLOC(pid, int *, (HYD_bscu_pid_count + num_hosts) * sizeof(int), status);
    for (i = 0; i < HYD_bscu_pid_count; i++)
        pid[i] = HYD_bscu_pid_list[i];
    HYDU_FREE(HYD_bscu_pid_list);
    HYD_bscu_pid_list = pid;

    /* Increase fd list to accommodate these new fds */
    HYDU_MALLOC(fd_list, int *, (HYD_bscu_fd_count + (2 * num_hosts) + 1) * sizeof(int),
                status);
    for (i = 0; i < HYD_bscu_fd_count; i++)
        fd_list[i] = HYD_bscu_fd_list[i];
    HYDU_FREE(HYD_bscu_fd_list);
    HYD_bscu_fd_list = fd_list;

    for (i = 0, node = node_list; node; node = node->next, i++) {
        /* append proxy ID */
        targs[idx] = HYDU_int_to_str(i);
        targs[idx + 1] = NULL;

        /* The stdin pointer will be some value for process_id 0; for
         * everyone else, it's NULL. */
        status = HYDU_create_process(targs, NULL, (i == 0 ? &fd_stdin : NULL),
                                     &fd_stdout, &fd_stderr,
                                     &HYD_bscu_pid_list[HYD_bscu_pid_count++], -1);
        HYDU_ERR_POP(status, "create process returned error\n");

        /* We don't wait for stdin to close */
        HYD_bscu_fd_list[HYD_bscu_fd_count++] = fd_stdout;
        HYD_bscu_fd_list[HYD_bscu_fd_count++] = fd_stderr;

        /* Register stdio callbacks for the spawned process */
        if (i == 0) {
            fd = STDIN_FILENO;
            status = HYDU_dmx_register_fd(1, &fd, HYD_POLLIN, &fd_stdin, HYDT_bscu_stdin_cb);
            HYDU_ERR_POP(status, "demux returned error registering fd\n");
        }

        status = HYDU_dmx_register_fd(1, &fd_stdout, HYD_POLLIN, stdout_cb,
                                      HYDT_bscu_inter_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");

        status = HYDU_dmx_register_fd(1, &fd_stderr, HYD_POLLIN, stderr_cb,
                                      HYDT_bscu_inter_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");
    }

  fn_exit:
    HYDU_free_strlist(targs);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
