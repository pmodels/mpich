/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "external.h"

static int fd_stdin, fd_stdout, fd_stderr;

static HYD_status is_local_host(char *host, int *bool)
{
    char localhost[MAX_HOSTNAME_LEN];
    HYD_status status = HYD_SUCCESS;

    *bool = 0;
    if (!strcmp(host, "localhost")) {
        *bool = 1;
        goto fn_exit;
    }

    status = HYDU_gethostname(localhost);
    HYDU_ERR_POP(status, "unable to get local hostname\n");

    if (!strcmp(host, localhost)) {
        *bool = 1;
        goto fn_exit;
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bscd_external_launch_procs(char **args, struct HYD_node *node_list,
                                           int *control_fd, int enable_stdin,
                                           HYD_status(*stdout_cb) (void *buf, int buflen),
                                           HYD_status(*stderr_cb) (void *buf, int buflen))
{
    int num_hosts, idx, i, host_idx, fd, exec_idx, offset, lh;
    int *pid, *fd_list;
    int sockpair[2];
    struct HYD_node *node;
    char *targs[HYD_NUM_TMP_STRINGS], *path = NULL;
    struct HYD_env *env = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We use the following priority order for the executable path:
     * (1) user-specified; (2) search in path; (3) Hard-coded
     * location */
    if (HYDT_bsci_info.bootstrap_exec)
        path = HYDU_strdup(HYDT_bsci_info.bootstrap_exec);
    if (!strcmp(HYDT_bsci_info.bootstrap, "ssh")) {
        if (!path)
            path = HYDU_find_full_path("ssh");
        if (!path)
            path = HYDU_strdup("/usr/bin/ssh");
    }
    else if (!strcmp(HYDT_bsci_info.bootstrap, "rsh")) {
        if (!path)
            path = HYDU_find_full_path("rsh");
        if (!path)
            path = HYDU_strdup("/usr/bin/rsh");
    }

    idx = 0;
    targs[idx++] = HYDU_strdup(path);

    /* Allow X forwarding only if explicitly requested */
    if (!strcmp(HYDT_bsci_info.bootstrap, "ssh")) {
        if (HYDT_bsci_info.enablex == 1)
            targs[idx++] = HYDU_strdup("-X");
        else if (HYDT_bsci_info.enablex == 0)
            targs[idx++] = HYDU_strdup("-x");
        else    /* default mode is disable X */
            targs[idx++] = HYDU_strdup("-x");
    }

    host_idx = idx++;   /* Hostname will come here */

    /* Fill in the remaining arguments */
    exec_idx = idx;     /* Store the executable index */
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

    targs[host_idx] = NULL;
    targs[idx] = NULL;
    for (i = 0, node = node_list; node; node = node->next, i++) {

        if (targs[host_idx])
            HYDU_FREE(targs[host_idx]);
        targs[host_idx] = HYDU_strdup(node->hostname);

        /* append proxy ID */
        if (targs[idx])
            HYDU_FREE(targs[idx]);
        targs[idx] = HYDU_int_to_str(i);
        targs[idx + 1] = NULL;

        /* ssh has many types of security controls that do not allow a
         * user to ssh to the same node multiple times very
         * quickly. If this happens, the ssh daemons disables ssh
         * connections causing the job to fail. This is basically a
         * hack to slow down ssh connections to the same node. */
        if (!strcmp(HYDT_bsci_info.bootstrap, "ssh")) {
            status = HYDT_bscd_ssh_store_launch_time(node->hostname);
            HYDU_ERR_POP(status, "error storing launch time\n");
        }

        status = is_local_host(node->hostname, &lh);
        HYDU_ERR_POP(status, "error checking if node is localhost\n");

        /* If bootstrap server is 'fork', or this is the localhost,
         * use fork to launch the process */
        if (!strcmp(HYDT_bsci_info.bootstrap, "fork") || lh) {
            offset = exec_idx;

            if (control_fd) {
                char *str;

                if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair) < 0)
                    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "pipe error\n");

                str = HYDU_int_to_str(sockpair[1]);
                status = HYDU_env_create(&env, "HYDRA_CONTROL_FD", str);
                HYDU_ERR_POP(status, "unable to create env\n");
                HYDU_FREE(str);

                control_fd[i] = sockpair[0];

                /* make sure control_fd[i] is not shared by the
                 * processes spawned in the future */
                status = HYDU_sock_cloexec(control_fd[i]);
                HYDU_ERR_POP(status, "unable to set control socket to close on exec\n");
            }
        }
        else
            offset = 0;

        if (HYDT_bsci_info.debug) {
            HYDU_dump(stdout, "Launch arguments: ");
            HYDU_print_strlist(targs + offset);
        }

        /* The stdin pointer will be some value for process_id 0; for
         * everyone else, it's NULL. */
        status = HYDU_create_process(targs + offset, env,
                                     ((i == 0 && enable_stdin) ? &fd_stdin : NULL),
                                     &fd_stdout, &fd_stderr,
                                     &HYD_bscu_pid_list[HYD_bscu_pid_count++], -1);
        HYDU_ERR_POP(status, "create process returned error\n");

        if (offset && control_fd) {
            close(sockpair[1]);
            HYDU_env_free(env);
            env = NULL;
        }

        /* We don't wait for stdin to close */
        HYD_bscu_fd_list[HYD_bscu_fd_count++] = fd_stdout;
        HYD_bscu_fd_list[HYD_bscu_fd_count++] = fd_stderr;

        /* Register stdio callbacks for the spawned process */
        if (i == 0 && enable_stdin) {
            fd = STDIN_FILENO;
            status = HYDT_dmx_register_fd(1, &fd, HYD_POLLIN, &fd_stdin, HYDT_bscu_stdin_cb);
            HYDU_ERR_POP(status, "demux returned error registering fd\n");
        }

        status = HYDT_dmx_register_fd(1, &fd_stdout, HYD_POLLIN, stdout_cb,
                                      HYDT_bscu_inter_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");

        status = HYDT_dmx_register_fd(1, &fd_stderr, HYD_POLLIN, stderr_cb,
                                      HYDT_bscu_inter_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");
    }

  fn_exit:
    HYDU_free_strlist(targs);
    if (path)
        HYDU_FREE(path);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
