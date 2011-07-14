/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"
#include "topo.h"
#include "ll.h"

static int fd_stdout, fd_stderr;

HYD_status HYDT_bscd_ll_launch_procs(char **args, struct HYD_proxy *proxy_list,
                                     int *control_fd)
{
    int idx, i, total_procs, node_count;
    int *pid, *fd_list, exec_idx;
    char *targs[HYD_NUM_TMP_STRINGS], *node_list_str = NULL;
    char *path = NULL, *extra_arg_list = NULL, *extra_arg, quoted_exec_string[HYD_TMP_STRLEN];
    struct HYD_proxy *proxy;
    struct HYDT_topo_cpuset_t cpuset;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We use the following priority order for the executable path:
     * (1) user-specified; (2) search in path; (3) Hard-coded
     * location */
    if (HYDT_bsci_info.launcher_exec)
        path = HYDU_strdup(HYDT_bsci_info.launcher_exec);
    if (!path)
        path = HYDU_find_full_path("poe");
    if (!path)
        path = HYDU_strdup("/usr/bin/poe");

    idx = 0;
    targs[idx++] = HYDU_strdup(path);

    if (!strcmp(HYDT_bsci_info.rmk, "ll")) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "ll does not support user-defined host lists\n");
    }

    /* Check how many nodes are being passed for the launch */
    status = HYDTI_bscd_ll_query_node_count(&total_procs);
    HYDU_ERR_POP(status, "unable to query for the node count\n");

    node_count = 0;
    for (proxy = proxy_list; proxy; proxy = proxy->next)
        node_count++;

    if (total_procs != node_count)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "processes to be launched have to cover all nodes\n");


    MPL_env2str("HYDRA_LAUNCHER_EXTRA_ARGS", (const char **) &extra_arg_list);
    if (extra_arg_list) {
        extra_arg = strtok(extra_arg_list, " ");
        while (extra_arg) {
            targs[idx++] = HYDU_strdup(extra_arg);
            extra_arg = strtok(NULL, " ");
        }
    }

    /* Fill in the remaining arguments */
    exec_idx = idx;
    for (i = 0; args[i]; i++)
        targs[idx++] = HYDU_strdup(args[i]);

    /* Create a quoted version of the exec string, which is only used
     * when the executable is not launched directly, but through an
     * actual launcher */
    HYDU_snprintf(quoted_exec_string, HYD_TMP_STRLEN, "\"%s\"", targs[exec_idx]);
    HYDU_FREE(targs[exec_idx]);
    targs[exec_idx] = quoted_exec_string;

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

    HYDT_topo_cpuset_zero(&cpuset);
    status = HYDU_create_process(targs, NULL, NULL, &fd_stdout, &fd_stderr,
                                 &HYD_bscu_pid_list[HYD_bscu_pid_count++], cpuset);
    HYDU_ERR_POP(status, "create process returned error\n");

    HYD_bscu_fd_list[HYD_bscu_fd_count++] = fd_stdout;
    HYD_bscu_fd_list[HYD_bscu_fd_count++] = fd_stderr;

    status = HYDT_dmx_register_fd(1, &fd_stdout, HYD_POLLIN,
                                  (void *) (size_t) STDOUT_FILENO, HYDT_bscu_stdio_cb);
    HYDU_ERR_POP(status, "demux returned error registering fd\n");

    status = HYDT_dmx_register_fd(1, &fd_stderr, HYD_POLLIN,
                                  (void *) (size_t) STDERR_FILENO, HYDT_bscu_stdio_cb);
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
