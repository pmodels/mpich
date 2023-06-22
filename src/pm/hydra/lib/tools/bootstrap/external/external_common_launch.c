/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"
#include "topo.h"
#include "common.h"

static int fd_stdout, fd_stderr;

/* We use the following priority order for the executable path: (1)
 * user-specified; (2) search in path; (3) Hard-coded location */
static HYD_status ssh_get_path(char **path)
{
    if (HYDT_bsci_info.launcher_exec)
        *path = MPL_strdup(HYDT_bsci_info.launcher_exec);
    if (*path == NULL)
        *path = HYDU_find_full_path("ssh");
    if (*path == NULL)
        *path = MPL_strdup("/usr/bin/ssh");

    return HYD_SUCCESS;
}

static HYD_status rsh_get_path(char **path)
{
    if (HYDT_bsci_info.launcher_exec)
        *path = MPL_strdup(HYDT_bsci_info.launcher_exec);
    if (*path == NULL)
        *path = HYDU_find_full_path("rsh");
    if (*path == NULL)
        *path = MPL_strdup("/usr/bin/rsh");

    return HYD_SUCCESS;
}

static HYD_status lsf_get_path(char **path)
{
    char *bin_dir = NULL;
    int length;
    HYD_status status = HYD_SUCCESS;

    if (HYDT_bsci_info.launcher_exec)
        *path = MPL_strdup(HYDT_bsci_info.launcher_exec);

    if (*path == NULL) {
        if (MPL_env2str("LSF_BINDIR", (const char **) &bin_dir) && bin_dir) {
            length = strlen(bin_dir) + 2 + strlen("blaunch");
            HYDU_MALLOC_OR_JUMP(*path, char *, length, status);
            snprintf(*path, length, "%s/blaunch", bin_dir);
        }
    }
    if (*path == NULL)
        *path = HYDU_find_full_path("blaunch");
    if (*path == NULL)
        *path = MPL_strdup("/usr/bin/blaunch");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status sge_get_path(char **path)
{
    char *sge_root = NULL, *arc = NULL;
    int length;
    HYD_status status = HYD_SUCCESS;

    if (HYDT_bsci_info.launcher_exec)
        *path = MPL_strdup(HYDT_bsci_info.launcher_exec);

    if (*path == NULL) {
        if (MPL_env2str("SGE_ROOT", (const char **) &sge_root) &&
            MPL_env2str("ARC", (const char **) &arc) && sge_root && arc) {
            length = strlen(sge_root) + strlen("/bin/") + strlen(arc) + 1 + strlen("qrsh") + 1;
            HYDU_MALLOC_OR_JUMP(*path, char *, length, status);
            snprintf(*path, length, "%s/bin/%s/qrsh", sge_root, arc);
        }
    }
    if (*path == NULL)
        *path = HYDU_find_full_path("qrsh");
    if (*path == NULL)
        *path = MPL_strdup("/usr/bin/qrsh");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

/* ---- tree launch ---- */
static void tree_launch_init(int k, int myid, int num, int *id_start, int *id_end)
{
    if (k <= 0) {
        /* flat launch */
        *id_start = 0;
        *id_end = num;
    } else if (myid == -1) {
        /* server only launch the first k proxies */
        *id_start = 0;
        *id_end = k;
    } else {
        /* level 1 - k   proxies
         * level 2 - k^2 proxies
         * level 3 - k^3 proxies
         * ...
         */
        int n = 1;
        int k_n = k;
        int sum = k;
        while (myid >= sum) {
            n++;
            k_n *= k;
            sum += k_n;
        }
        *id_start = (myid - (sum - k_n)) * k + sum;
        *id_end = *id_start + k;
    }
    if (*id_end > num) {
        *id_end = num;
    }
}

HYD_status HYDT_bscd_common_launch_procs(int pgid, char **args, struct HYD_host *hosts,
                                         int num_hosts, int use_rmk, int k, int myid,
                                         int *control_fd)
{
    int idx, i, host_idx, fd, exec_idx, offset, len, rc, autofork;
    int id_idx;
    int *fd_list, *dummy;
    int sockpair[2];
    char *targs[HYD_NUM_TMP_STRINGS] = { NULL }, *path = NULL, *extra_arg_list = NULL, *extra_arg;
    char quoted_exec_string[HYD_TMP_STRLEN], *original_exec_string;
    struct HYD_env *env = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!strcmp(HYDT_bsci_info.launcher, "ssh")) {
        status = ssh_get_path(&path);
        HYDU_ERR_POP(status, "unable to get path to the ssh executable\n");
    } else if (!strcmp(HYDT_bsci_info.launcher, "rsh")) {
        status = rsh_get_path(&path);
        HYDU_ERR_POP(status, "unable to get path to the rsh executable\n");
    } else if (!strcmp(HYDT_bsci_info.launcher, "fork")) {
        /* fork has no separate launcher */
    } else if (!strcmp(HYDT_bsci_info.launcher, "lsf")) {
        status = lsf_get_path(&path);
        HYDU_ERR_POP(status, "unable to get path to the blaunch executable\n");
    } else if (!strcmp(HYDT_bsci_info.launcher, "sge")) {
        status = sge_get_path(&path);
        HYDU_ERR_POP(status, "unable to get path to the qrsh executable\n");
    } else if (!strcmp(HYDT_bsci_info.launcher, "manual")) {
        /* manual has no separate launcher */
    } else {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "bad launcher type %s\n",
                            HYDT_bsci_info.launcher);
    }

    idx = 0;
    if (path)
        targs[idx++] = MPL_strdup(path);
    targs[idx] = NULL;

    /* Allow X forwarding only if explicitly requested */
    if (!strcmp(HYDT_bsci_info.launcher, "ssh")) {
        if (HYDT_bsci_info.enablex == 1)
            targs[idx++] = MPL_strdup("-X");
        else if (HYDT_bsci_info.enablex == 0)
            targs[idx++] = MPL_strdup("-x");
        else    /* default mode is disable X */
            targs[idx++] = MPL_strdup("-x");
    } else if (!strcmp(HYDT_bsci_info.launcher, "lsf")) {
        targs[idx++] = MPL_strdup("-n");
    } else if (!strcmp(HYDT_bsci_info.launcher, "sge")) {
        targs[idx++] = MPL_strdup("-inherit");
        targs[idx++] = MPL_strdup("-V");
    }

    if (MPL_env2str("HYDRA_LAUNCHER_EXTRA_ARGS", (const char **) &extra_arg_list)) {
        char *s = MPL_strdup(extra_arg_list);
        extra_arg = strtok(s, " ");
        while (extra_arg) {
            targs[idx++] = MPL_strdup(extra_arg);
            extra_arg = strtok(NULL, " ");
        }
        MPL_free(s);
    }

    host_idx = idx++;   /* Hostname will come here */
    targs[host_idx] = NULL;

    /* Fill in the remaining arguments */
    exec_idx = idx;     /* Store the executable index */
    for (i = 0; args[i]; i++)
        targs[idx++] = MPL_strdup(args[i]);

    targs[idx++] = MPL_strdup("--proxy-id");
    id_idx = idx++;
    targs[id_idx] = NULL;

    targs[idx] = NULL;

    /* Store the original exec string */
    original_exec_string = targs[exec_idx];

    /* Create a quoted version of the exec string, which is only used
     * when the executable is not launched directly, but through an
     * actual launcher */
    snprintf(quoted_exec_string, HYD_TMP_STRLEN, "\"%s\"", targs[exec_idx]);

    /* Increase pid list to accommodate these new pids */
    HYDT_bscu_pid_list_grow(num_hosts);

    /* Increase fd list to accommodate these new fds */
    HYDU_MALLOC_OR_JUMP(fd_list, int *, (HYD_bscu_fd_count + (2 * num_hosts) + 1) * sizeof(int),
                        status);
    for (i = 0; i < HYD_bscu_fd_count; i++)
        fd_list[i] = HYD_bscu_fd_list[i];
    MPL_free(HYD_bscu_fd_list);
    HYD_bscu_fd_list = fd_list;

    if (k == 0) {
        /* Check if the user disabled automatic forking */
        rc = MPL_env2bool("HYDRA_LAUNCHER_AUTOFORK", &autofork);
        if (rc == 0)
            autofork = 1;
    } else {
        /* disable fork if doing tree-launch */
        autofork = 0;
    }

    int id_start, id_end;
    tree_launch_init(k, myid, num_hosts, &id_start, &id_end);

    for (i = id_start; i < id_end; i++) {
        MPL_free(targs[host_idx]);
        if (hosts[i].user == NULL) {
            targs[host_idx] = MPL_strdup(hosts[i].hostname);
        } else {
            len = strlen(hosts[i].user) + strlen("@") + strlen(hosts[i].hostname) + 1;
            HYDU_MALLOC_OR_JUMP(targs[host_idx], char *, len, status);
            snprintf(targs[host_idx], len, "%s@%s", hosts[i].user, hosts[i].hostname);
        }

        /* append proxy ID */
        MPL_free(targs[id_idx]);
        targs[id_idx] = HYDU_int_to_str(i);

        /* If launcher is 'fork', or this is the localhost, use fork
         * to launch the process */
        if (autofork && (!strcmp(HYDT_bsci_info.launcher, "fork") ||
                         !strcmp(HYDT_bsci_info.launcher, "manual") ||
                         MPL_host_is_local(hosts[i].hostname))) {
            offset = exec_idx;

            if (control_fd) {
                char *str;

                if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair) < 0)
                    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "pipe error\n");

                str = HYDU_int_to_str(sockpair[1]);
                status = HYDU_env_create(&env, "HYDI_CONTROL_FD", str);
                HYDU_ERR_POP(status, "unable to create env\n");
                MPL_free(str);

                control_fd[i] = sockpair[0];

                /* make sure control_fd[i] is not shared by the
                 * processes spawned in the future */
                status = HYDU_sock_cloexec(control_fd[i]);
                HYDU_ERR_POP(status, "unable to set control socket to close on exec\n");
            }
        } else {
            offset = 0;

            /* We are not launching the executable directly; use the
             * quoted version */
            targs[exec_idx] = quoted_exec_string;
        }

        /* When stdin is closed, the launched process may later create a communication channel
         * uses fd = 0. This may cause mysterious issues, esp. with ssh. In one case, we encounter
         * bugs that interrupts CI's ssh connection. Always set stdin to a dummy to save some
         * trouble.
         */
        dummy = &fd;

        if (HYDT_bsci_info.debug) {
            HYDU_dump(stdout, "Launch arguments: ");
            HYDU_print_strlist(targs + offset);
        }

        if (!strcmp(HYDT_bsci_info.launcher, "manual")) {
            HYDU_dump_noprefix(stdout, "HYDRA_LAUNCH: ");
            HYDU_print_strlist(targs + offset);
            continue;
        }

        /* ssh has many types of security controls that do not allow a
         * user to ssh to the same node multiple times very
         * quickly. If this happens, the ssh daemons disables ssh
         * connections causing the job to fail. This is basically a
         * hack to slow down ssh connections to the same node. We
         * check for offset == 0 before applying this hack, so we only
         * slow down the cases where ssh is being used, and not the
         * cases where we fall back to fork. */
        if (!strcmp(HYDT_bsci_info.launcher, "ssh") && !offset) {
            status = HYDTI_bscd_ssh_store_launch_time(hosts[i].hostname);
            HYDU_ERR_POP(status, "error storing launch time\n");
        }

        /* The stdin pointer is a dummy value. We don't just pass it
         * NULL, as older versions of ssh seem to freak out when no
         * stdin socket is provided. */
        int pid;
        status = HYDU_create_process(targs + offset, env, dummy, &fd_stdout, &fd_stderr, &pid, -1);
        HYDT_bscu_pid_list_push(pid, pgid, i);
        HYDU_ERR_POP(status, "create process returned error\n");

        /* Reset the exec string to the original value */
        targs[exec_idx] = original_exec_string;

        if (offset && control_fd) {
            close(sockpair[1]);
            HYDU_env_free(env);
            env = NULL;
        }

        HYD_bscu_fd_list[HYD_bscu_fd_count++] = fd_stdout;
        HYD_bscu_fd_list[HYD_bscu_fd_count++] = fd_stderr;

        status = HYDT_dmx_register_fd(1, &fd_stdout, HYD_POLLIN,
                                      (void *) (size_t) STDOUT_FILENO, HYDT_bscu_stdio_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");

        status = HYDT_dmx_register_fd(1, &fd_stderr, HYD_POLLIN,
                                      (void *) (size_t) STDERR_FILENO, HYDT_bscu_stdio_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");
    }

    if (!strcmp(HYDT_bsci_info.launcher, "manual"))
        HYDU_dump_noprefix(stdout, "HYDRA_LAUNCH_END\n");

  fn_exit:
    HYDU_free_strlist(targs);
    MPL_free(path);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
