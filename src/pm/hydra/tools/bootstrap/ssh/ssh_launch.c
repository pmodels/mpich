/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "ssh.h"

#define older(a,b) \
    ((((a).tv_sec < (b).tv_sec) ||                                      \
      (((a).tv_sec == (b).tv_sec) && ((a).tv_usec < (b).tv_usec))) ? 1 : 0)

static int fd_stdin, fd_stdout, fd_stderr;

static HYD_status create_element(char *hostname, struct HYDT_bscd_ssh_time **e)
{
    int i;
    struct HYDT_bscd_ssh_time *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC((*e), struct HYDT_bscd_ssh_time *, sizeof(struct HYDT_bscd_ssh_time), status);

    (*e)->hostname = HYDU_strdup(hostname);
    for (i = 0; i < SSH_LIMIT; i++) {
        (*e)->init_time[i].tv_sec = 0;
        (*e)->init_time[i].tv_usec = 0;
    }
    (*e)->next = NULL;

    if (HYDT_bscd_ssh_time == NULL)
        HYDT_bscd_ssh_time = (*e);
    else {
        for (tmp = HYDT_bscd_ssh_time; tmp->next; tmp = tmp->next);
        tmp->next = (*e);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status store_launch_time(struct HYDT_bscd_ssh_time *e)
{
    int i, oldest, time_left;
    struct timeval now;
    HYD_status status = HYD_SUCCESS;

    /* Search for an unset element to store the current time */
    for (i = 0; i < SSH_LIMIT; i++) {
        if (e->init_time[i].tv_sec == 0 && e->init_time[i].tv_usec == 0) {
            gettimeofday(&e->init_time[i], NULL);
            goto fn_exit;
        }
    }

    /* No free element found; wait for the oldest element to turn
     * older */
    oldest = 0;
    for (i = 0; i < SSH_LIMIT; i++)
        if (older(e->init_time[i], e->init_time[oldest]))
            oldest = i;

    gettimeofday(&now, NULL);
    time_left = SSH_LIMIT_TIME - now.tv_sec + e->init_time[oldest].tv_sec;

    /* A better approach will be to make progress here, but that would
     * mean that we need to deal with nested calls to the demux engine
     * and process launches. */
    if (time_left > 0)
        sleep(time_left);

    /* Store the current time in the oldest element */
    gettimeofday(&e->init_time[oldest], NULL);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bscd_ssh_launch_procs(char **args, struct HYD_node *node_list,
                                      int enable_stdin,
                                      HYD_status(*stdout_cb) (void *buf, int buflen),
                                      HYD_status(*stderr_cb) (void *buf, int buflen))
{
    int num_hosts, idx, i, host_idx, fd;
    int *pid, *fd_list;
    struct HYD_node *node;
    char *targs[HYD_NUM_TMP_STRINGS], *path = NULL;
    struct HYDT_bscd_ssh_time *e;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We use the following priority order for the executable path:
     * (1) user-specified; (2) search in path; (3) Hard-coded
     * location */
    if (HYDT_bsci_info.bootstrap_exec)
        path = HYDU_strdup(HYDT_bsci_info.bootstrap_exec);
    if (!path)
        path = HYDU_find_full_path("ssh");
    if (!path)
        path = HYDU_strdup("/usr/bin/ssh");

    idx = 0;
    targs[idx++] = HYDU_strdup(path);

    /* Allow X forwarding only if explicitly requested */
    if (HYDT_bsci_info.enablex == 1)
        targs[idx++] = HYDU_strdup("-X");
    else if (HYDT_bsci_info.enablex == 0)
        targs[idx++] = HYDU_strdup("-x");
    else        /* default mode is disable X */
        targs[idx++] = HYDU_strdup("-x");

    host_idx = idx++;   /* Hostname will come here */

    /* Fill in the remaining arguments */
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

        /* Store the launch time */
        for (e = HYDT_bscd_ssh_time; e; e = e->next)
            if (!strcmp(node->hostname, e->hostname))
                break;

        if (e == NULL) { /* Couldn't find an element for this host */
            status = create_element(node->hostname, &e);
            HYDU_ERR_POP(status, "unable to create ssh time element\n");
        }

        status = store_launch_time(e);
        HYDU_ERR_POP(status, "error storing launch time\n");

        /* The stdin pointer will be some value for process_id 0; for
         * everyone else, it's NULL. */
        status = HYDU_create_process(targs, NULL, NULL,
                                     ((i == 0 && enable_stdin) ? &fd_stdin : NULL),
                                     &fd_stdout, &fd_stderr,
                                     &HYD_bscu_pid_list[HYD_bscu_pid_count++], -1);
        HYDU_ERR_POP(status, "create process returned error\n");

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
