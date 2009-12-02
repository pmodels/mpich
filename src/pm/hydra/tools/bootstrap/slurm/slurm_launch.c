/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "slurm.h"

static int fd_stdin, fd_stdout, fd_stderr;

static HYD_status node_list_to_str(struct HYD_node *node_list, char **node_list_str,
                                   int *num_hosts)
{
    int i;
    char *tmp[HYD_NUM_TMP_STRINGS], *foo = NULL;
    struct HYD_node *node;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = 0;
    *num_hosts = 0;
    for (node = node_list; node; node = node->next) {
        tmp[i++] = HYDU_strdup(node->hostname);
        (*num_hosts)++;

        if (node->next)
            tmp[i++] = HYDU_strdup(",");

        /* If we used up more than half of the array elements, merge
         * what we have so far */
        if (i > (HYD_NUM_TMP_STRINGS / 2)) {
            if (foo)
                HYDU_FREE(foo);

            tmp[i++] = NULL;
            status = HYDU_str_alloc_and_join(tmp, &foo);
            HYDU_ERR_POP(status, "error joining strings\n");

            i = 0;
            tmp[i++] = foo;
        }
    }

    tmp[i++] = NULL;
    status = HYDU_str_alloc_and_join(tmp, &foo);
    HYDU_ERR_POP(status, "error joining strings\n");

    *node_list_str = foo;
    foo = NULL;

  fn_exit:
    HYDU_free_strlist(tmp);
    if (foo)
        HYDU_FREE(foo);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bscd_slurm_launch_procs(char **args, struct HYD_node *node_list,
                                        HYD_status(*stdout_cb) (void *buf, int buflen),
                                        HYD_status(*stderr_cb) (void *buf, int buflen))
{
    int num_hosts, idx, i;
    int *pid, *fd_list;
    char *targs[HYD_NUM_TMP_STRINGS], *node_list_str = NULL;
    char *path = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We use the following priority order for the executable path:
     * (1) user-specified; (2) search in path; (3) Hard-coded
     * location */
    if (HYDT_bsci_info.bootstrap_exec)
        path = HYDU_strdup(HYDT_bsci_info.bootstrap_exec);
    if (!path)
        path = HYDU_find_full_path("srun");
    if (!path)
        path = HYDU_strdup("/usr/bin/srun");

    idx = 0;
    targs[idx++] = HYDU_strdup(path);
    targs[idx++] = HYDU_strdup("--nodelist");

    status = node_list_to_str(node_list, &node_list_str, &num_hosts);
    HYDU_ERR_POP(status, "unable to build a node list string\n");

    targs[idx++] = HYDU_strdup(node_list_str);

    targs[idx++] = HYDU_strdup("-N");
    targs[idx++] = HYDU_int_to_str(num_hosts);

    targs[idx++] = HYDU_strdup("-n");
    targs[idx++] = HYDU_int_to_str(num_hosts);

    /* Fill in the remaining arguments */
    for (i = 0; args[i]; i++)
        targs[idx++] = HYDU_strdup(args[i]);

    /* Increase pid list to accommodate the new pid */
    HYDU_MALLOC(pid, int *, (HYD_bscu_pid_count + 1) * sizeof(int), status);
    for (i = 0; i < HYD_bscu_pid_count; i++)
        pid[i] = HYD_bscu_pid_list[i];
    HYDU_FREE(HYD_bscu_pid_list);
    HYD_bscu_pid_list = pid;

    /* Increase fd list to accommodate these new fds */
    HYDU_MALLOC(fd_list, int *, (HYD_bscu_fd_count + 3) * sizeof(int), status);
    for (i = 0; i < HYD_bscu_fd_count; i++)
        fd_list[i] = HYD_bscu_fd_list[i];
    HYDU_FREE(HYD_bscu_fd_list);
    HYD_bscu_fd_list = fd_list;

    /* append proxy ID as -1 */
    targs[idx++] = HYDU_int_to_str(-1);
    targs[idx++] = NULL;

    status = HYDU_create_process(targs, NULL, &fd_stdin, &fd_stdout, &fd_stderr,
                                 &HYD_bscu_pid_list[HYD_bscu_pid_count++], -1);
    HYDU_ERR_POP(status, "create process returned error\n");

    HYD_bscu_fd_list[HYD_bscu_fd_count++] = fd_stdin;
    HYD_bscu_fd_list[HYD_bscu_fd_count++] = fd_stdout;
    HYD_bscu_fd_list[HYD_bscu_fd_count++] = fd_stderr;

    /* Register stdio callbacks for the spawned process */
    status = HYDU_dmx_register_fd(1, &fd_stdin, HYD_POLLIN, &fd_stdin, HYDT_bscu_stdin_cb);
    HYDU_ERR_POP(status, "demux returned error registering fd\n");

    status = HYDU_dmx_register_fd(1, &fd_stdout, HYD_POLLIN, stdout_cb, HYDT_bscu_inter_cb);
    HYDU_ERR_POP(status, "demux returned error registering fd\n");

    status = HYDU_dmx_register_fd(1, &fd_stderr, HYD_POLLIN, stderr_cb, HYDT_bscu_inter_cb);
    HYDU_ERR_POP(status, "demux returned error registering fd\n");

  fn_exit:
    if (node_list_str)
        HYDU_FREE(node_list_str);
    HYDU_free_strlist(targs);
    if (path)
        HYDU_FREE(path);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
