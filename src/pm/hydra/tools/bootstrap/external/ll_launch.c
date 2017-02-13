/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
static int total_node_count = 0;

static HYD_status process_mfile_count(char *token, int newline, struct HYD_node **node_list)
{
    HYD_status status = HYD_SUCCESS;

    if (newline)
        total_node_count++;

    return status;
}

static HYD_status query_node_count(int *count)
{
    char *hostfile;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (MPL_env2str("LOADL_HOSTFILE", (const char **) &hostfile) == 0)
        hostfile = NULL;

    if (hostfile == NULL) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "No LL nodefile found\n");
    }
    else {
        total_node_count = 0;
        status = HYDU_parse_hostfile(hostfile, NULL, process_mfile_count);
        HYDU_ERR_POP(status, "error parsing hostfile\n");

        *count = total_node_count;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bscd_ll_launch_procs(const char *rmk, struct HYD_node *node_list, char **args,
                                     int *control_fd)
{
    int idx, i, total_procs, node_count;
    int *pid, *fd_list, exec_idx;
    char *targs[HYD_NUM_TMP_STRINGS], *node_list_str = NULL;
    char *path = NULL, *extra_arg_list = NULL, *extra_arg, quoted_exec_string[HYD_TMP_STRLEN];
    struct HYD_node *node;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We use the following priority order for the executable path:
     * (1) user-specified; (2) search in path; (3) Hard-coded
     * location */
    if (HYDT_bsci_info.launcher_exec)
        path = MPL_strdup(HYDT_bsci_info.launcher_exec);
    if (!path)
        path = HYDU_find_full_path("poe");
    if (!path)
        path = MPL_strdup("/usr/bin/poe");

    idx = 0;
    targs[idx++] = MPL_strdup(path);

    if (!strcmp(rmk, "ll")) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "ll does not support user-defined host lists\n");
    }

    /* Check how many nodes are being passed for the launch */
    status = query_node_count(&total_procs);
    HYDU_ERR_POP(status, "unable to query for the node count\n");

    node_count = 0;
    for (node = node_list; node; node = node->next)
        node_count++;

    if (total_procs != node_count)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "processes to be launched have to cover all nodes\n");


    MPL_env2str("HYDRA_LAUNCHER_EXTRA_ARGS", (const char **) &extra_arg_list);
    if (extra_arg_list) {
        extra_arg = strtok(extra_arg_list, " ");
        while (extra_arg) {
            targs[idx++] = MPL_strdup(extra_arg);
            extra_arg = strtok(NULL, " ");
        }
    }

    /* Fill in the remaining arguments */
    exec_idx = idx;
    for (i = 0; args[i]; i++)
        targs[idx++] = MPL_strdup(args[i]);

    /* Create a quoted version of the exec string, which is only used
     * when the executable is not launched directly, but through an
     * actual launcher */
    MPL_snprintf(quoted_exec_string, HYD_TMP_STRLEN, "\"%s\"", targs[exec_idx]);
    MPL_free(targs[exec_idx]);
    targs[exec_idx] = quoted_exec_string;

    /* Increase pid list to accommodate the new pid */
    HYDU_MALLOC_OR_JUMP(pid, int *, (HYD_bscu_pid_count + 1) * sizeof(int), status);
    for (i = 0; i < HYD_bscu_pid_count; i++)
        pid[i] = HYD_bscu_pid_list[i];
    MPL_free(HYD_bscu_pid_list);
    HYD_bscu_pid_list = pid;

    /* Increase fd list to accommodate these new fds */
    HYDU_MALLOC_OR_JUMP(fd_list, int *, (HYD_bscu_fd_count + 3) * sizeof(int), status);
    for (i = 0; i < HYD_bscu_fd_count; i++)
        fd_list[i] = HYD_bscu_fd_list[i];
    MPL_free(HYD_bscu_fd_list);
    HYD_bscu_fd_list = fd_list;

    /* append node ID as -1 */
    targs[idx++] = HYDU_int_to_str(-1);
    targs[idx++] = NULL;

    status = HYDU_create_process(targs, NULL, NULL, &fd_stdout, &fd_stderr,
                                 &HYD_bscu_pid_list[HYD_bscu_pid_count++], -1);
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
        MPL_free(node_list_str);
    HYDU_free_strlist(targs);
    if (path)
        MPL_free(path);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
