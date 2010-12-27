/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "bind.h"
#include "external.h"

static int fd_stdin, fd_stdout, fd_stderr;

HYD_status HYDT_bscd_external_launch_procs(char **args, struct HYD_node *node_list,
                                           int *control_fd, int enable_stdin)
{
    int num_hosts, idx, i, host_idx, fd, exec_idx, offset, lh;
    int *pid, *fd_list, *dummy;
    int sockpair[2];
    struct HYD_node *node;
    char *targs[HYD_NUM_TMP_STRINGS], *path = NULL, *extra_arg_list = NULL, *extra_arg;
    struct HYD_env *env = NULL;
    struct HYDT_bind_cpuset_t cpuset;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We use the following priority order for the executable path:
     * (1) user-specified; (2) search in path; (3) Hard-coded
     * location */
    if (HYDT_bsci_info.launcher_exec)
        path = HYDU_strdup(HYDT_bsci_info.launcher_exec);
    if (!strcmp(HYDT_bsci_info.launcher, "ssh")) {
        if (!path)
            path = HYDU_find_full_path("ssh");
        if (!path)
            path = HYDU_strdup("/usr/bin/ssh");
    }
    else if (!strcmp(HYDT_bsci_info.launcher, "rsh")) {
        if (!path)
            path = HYDU_find_full_path("rsh");
        if (!path)
            path = HYDU_strdup("/usr/bin/rsh");
    }
    else if (!strcmp(HYDT_bsci_info.launcher, "lsf")) {
        char *bin_dir = NULL;
        int length;

        MPL_env2str("LSF_BINDIR", (const char **) &bin_dir);
        if (bin_dir) {
            length = strlen(bin_dir) + 2 + strlen("blaunch");
            HYDU_MALLOC(path, char *, length, status);
            MPL_snprintf(path, length, "%s/blaunch", bin_dir);
        }
        if (!path)
            path = HYDU_find_full_path("blaunch");
        if (!path)
            path = HYDU_strdup("/usr/bin/blaunch");
    }
    else if (!strcmp(HYDT_bsci_info.launcher, "sge")) {
        char *sge_root = NULL, *arc = NULL;
        int length;

        MPL_env2str("SGE_ROOT", (const char **) &sge_root);
        MPL_env2str("ARC", (const char **) &arc);
        if (sge_root && arc) {
            length = strlen(sge_root) + strlen("/bin/") + strlen(arc) + 1 + strlen("qrsh") + 1;
            HYDU_MALLOC(path, char *, length, status);
            MPL_snprintf(path, length, "%s/bin/%s/qrsh", sge_root, arc);
        }

        if (!path)
            path = HYDU_find_full_path("qrsh");
        if (!path)
            path = HYDU_strdup("/usr/bin/qrsh");
    }

    idx = 0;
    if (path)
        targs[idx++] = HYDU_strdup(path);

    MPL_env2str("HYDRA_LAUNCH_EXTRA_ARGS", (const char **) &extra_arg_list);
    if (extra_arg_list) {
        extra_arg = strtok(extra_arg_list, " ");
        while (extra_arg) {
            targs[idx++] = HYDU_strdup(extra_arg);
            extra_arg = strtok(NULL, " ");
        }
    }

    /* Allow X forwarding only if explicitly requested */
    if (!strcmp(HYDT_bsci_info.launcher, "ssh")) {
        if (HYDT_bsci_info.enablex == 1)
            targs[idx++] = HYDU_strdup("-X");
        else if (HYDT_bsci_info.enablex == 0)
            targs[idx++] = HYDU_strdup("-x");
        else    /* default mode is disable X */
            targs[idx++] = HYDU_strdup("-x");
    }
    else if (!strcmp(HYDT_bsci_info.launcher, "lsf")) {
        targs[idx++] = HYDU_strdup("-n");
    }
    else if (!strcmp(HYDT_bsci_info.launcher, "sge")) {
        targs[idx++] = HYDU_strdup("-inherit");
        targs[idx++] = HYDU_strdup("-V");
    }

    host_idx = idx++;   /* Hostname will come here */
    targs[host_idx] = NULL;

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

    targs[idx] = NULL;
    HYDT_bind_cpuset_zero(&cpuset);
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
        if (!strcmp(HYDT_bsci_info.launcher, "ssh")) {
            status = HYDT_bscd_ssh_store_launch_time(node->hostname);
            HYDU_ERR_POP(status, "error storing launch time\n");
        }

        status = HYDU_sock_is_local(node->hostname, &lh);
        HYDU_ERR_POP(status, "error checking if node is localhost\n");

        /* If launcher is 'fork', or this is the localhost, use fork
         * to launch the process */
        if (!strcmp(HYDT_bsci_info.launcher, "fork") || lh) {
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

            dummy = NULL;
        }
        else {
            offset = 0;

            /* dummy is NULL only for launchers that can handle a
             * closed stdin socket. Older versions of ssh and SGE seem
             * to have problems when stdin is closed before they are
             * launched. */
            if (!strcmp(HYDT_bsci_info.launcher, "ssh") ||
                !strcmp(HYDT_bsci_info.launcher, "rsh") ||
                !strcmp(HYDT_bsci_info.launcher, "sge"))
                dummy = &fd;
            else
                dummy = NULL;
        }

        if (HYDT_bsci_info.debug) {
            HYDU_dump(stdout, "Launch arguments: ");
            HYDU_print_strlist(targs + offset);
        }

        /* The stdin pointer will be some value for process_id 0; for
         * everyone else, it's a dummy value. We don't just pass it
         * NULL, as older versions of ssh seem to freak out when no
         * stdin socket is provided. */
        status = HYDU_create_process(targs + offset, env,
                                     ((i == 0 && enable_stdin) ? &fd_stdin : dummy),
                                     &fd_stdout, &fd_stderr,
                                     &HYD_bscu_pid_list[HYD_bscu_pid_count++], cpuset);
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
            status = HYDT_dmx_register_fd(1, &fd, HYD_POLLIN,
                                          (void *) (size_t) fd_stdin, HYDT_bscu_stdio_cb);
            HYDU_ERR_POP(status, "demux returned error registering fd\n");
        }

        status = HYDT_dmx_register_fd(1, &fd_stdout, HYD_POLLIN,
                                      (void *) (size_t) STDOUT_FILENO, HYDT_bscu_stdio_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");

        status = HYDT_dmx_register_fd(1, &fd_stderr, HYD_POLLIN,
                                      (void *) (size_t) STDERR_FILENO, HYDT_bscu_stdio_cb);
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
