/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "demux.h"
#include "pmi_proxy.h"

struct HYD_PMCD_pmi_proxy_params HYD_PMCD_pmi_proxy_params;
int HYD_PMCD_pmi_proxy_listenfd;

int main(int argc, char **argv)
{
    int i, j, arg, count, pid, ret_status;
    int stdin_fd, process_id, core, pmi_id, rem;
    char *str;
    char *client_args[HYD_NUM_TMP_STRINGS];
    HYD_Env_t *env;
    struct HYD_Partition_exec *exec;
    struct HYD_Partition_segment *segment;
    HYD_Status status = HYD_SUCCESS;

    status = HYD_PMCD_pmi_proxy_get_params(argc, argv);
    HYDU_ERR_POP(status, "bad parameters passed to the proxy\n");

    status = HYDU_sock_listen(&HYD_PMCD_pmi_proxy_listenfd, NULL,
                              (uint16_t *) & HYD_PMCD_pmi_proxy_params.proxy_port);
    HYDU_ERR_POP(status, "unable to listen on socket\n");

    /* Register the listening socket with the demux engine */
    status = HYD_DMX_register_fd(1, &HYD_PMCD_pmi_proxy_listenfd, HYD_STDOUT,
                                 HYD_PMCD_pmi_proxy_listen_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    /* FIXME: We do not use the bootstrap server right now, as the
     * current implementation of the bootstrap launch directly reads
     * the executable information from the HYD_Handle structure. Since
     * we are a different process, we don't share this
     * structure. Without the bootstrap server, we can only launch
     * local processes. That is, we can only have a single-level
     * hierarchy of proxies. */

    HYD_PMCD_pmi_proxy_params.partition_proc_count = 0;
    for (segment = HYD_PMCD_pmi_proxy_params.segment_list; segment; segment = segment->next)
        HYD_PMCD_pmi_proxy_params.partition_proc_count += segment->proc_count;

    HYD_PMCD_pmi_proxy_params.exec_proc_count = 0;
    for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec; exec = exec->next)
        HYD_PMCD_pmi_proxy_params.exec_proc_count += exec->proc_count;

    HYDU_MALLOC(HYD_PMCD_pmi_proxy_params.out, int *,
                HYD_PMCD_pmi_proxy_params.exec_proc_count * sizeof(int), status);
    HYDU_MALLOC(HYD_PMCD_pmi_proxy_params.err, int *,
                HYD_PMCD_pmi_proxy_params.exec_proc_count * sizeof(int), status);
    HYDU_MALLOC(HYD_PMCD_pmi_proxy_params.pid, int *,
                HYD_PMCD_pmi_proxy_params.exec_proc_count * sizeof(int), status);
    HYDU_MALLOC(HYD_PMCD_pmi_proxy_params.exit_status, int *,
                HYD_PMCD_pmi_proxy_params.exec_proc_count * sizeof(int), status);

    /* Initialize the exit status */
    for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++)
        HYD_PMCD_pmi_proxy_params.exit_status[i] = -1;

    /* For local spawning, set the global environment here itself */
    status = HYDU_putenv_list(HYD_PMCD_pmi_proxy_params.global_env);
    HYDU_ERR_POP(status, "putenv returned error\n");

    status = HYDU_bind_init(HYD_PMCD_pmi_proxy_params.user_bind_map);
    HYDU_ERR_POP(status, "unable to initialize process binding\n");

    /* Spawn the processes */
    process_id = 0;
    core = -1;
    for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec; exec = exec->next) {
        for (i = 0; i < exec->proc_count; i++) {

            pmi_id = ((process_id / HYD_PMCD_pmi_proxy_params.partition_proc_count) *
                      HYD_PMCD_pmi_proxy_params.one_pass_count);
            rem = (process_id % HYD_PMCD_pmi_proxy_params.partition_proc_count);

            for (segment = HYD_PMCD_pmi_proxy_params.segment_list; segment;
                 segment = segment->next) {
                if (rem >= segment->proc_count)
                    rem -= segment->proc_count;
                else {
                    pmi_id += segment->start_pid + rem;
                    break;
                }
            }

            str = HYDU_int_to_str(pmi_id);
            status = HYDU_env_create(&env, "PMI_ID", str);
            HYDU_ERR_POP(status, "unable to create env\n");
            HYDU_FREE(str);
            status = HYDU_putenv(env);
            HYDU_ERR_POP(status, "putenv failed\n");

            if (chdir(HYD_PMCD_pmi_proxy_params.wdir) < 0)
                HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                                     "unable to change wdir (%s)\n", HYDU_strerror(errno));

            for (j = 0, arg = 0; exec->exec[j]; j++)
                client_args[arg++] = MPIU_Strdup(exec->exec[j]);
            client_args[arg++] = NULL;

            core = HYDU_next_core(core, HYD_PMCD_pmi_proxy_params.binding);
            if (pmi_id == 0) {
                status = HYDU_create_process(client_args, exec->prop_env,
                                             &HYD_PMCD_pmi_proxy_params.in,
                                             &HYD_PMCD_pmi_proxy_params.out[process_id],
                                             &HYD_PMCD_pmi_proxy_params.err[process_id],
                                             &HYD_PMCD_pmi_proxy_params.pid[process_id], core);

                status = HYDU_sock_set_nonblock(HYD_PMCD_pmi_proxy_params.in);
                HYDU_ERR_POP(status, "unable to set socket as non-blocking\n");

                stdin_fd = 0;
                status = HYDU_sock_set_nonblock(stdin_fd);
                HYDU_ERR_POP(status, "unable to set socket as non-blocking\n");

                HYD_PMCD_pmi_proxy_params.stdin_buf_offset = 0;
                HYD_PMCD_pmi_proxy_params.stdin_buf_count = 0;
                status =
                    HYD_DMX_register_fd(1, &stdin_fd, HYD_STDIN, HYD_PMCD_pmi_proxy_stdin_cb);
                HYDU_ERR_POP(status, "unable to register fd\n");
            }
            else {
                status = HYDU_create_process(client_args, exec->prop_env,
                                             NULL,
                                             &HYD_PMCD_pmi_proxy_params.out[process_id],
                                             &HYD_PMCD_pmi_proxy_params.err[process_id],
                                             &HYD_PMCD_pmi_proxy_params.pid[process_id], core);
            }
            HYDU_ERR_POP(status, "spawn process returned error\n");

            process_id++;
        }
    }

    /* Everything is spawned, now wait for I/O */
    status = HYD_DMX_register_fd(HYD_PMCD_pmi_proxy_params.exec_proc_count,
                                 HYD_PMCD_pmi_proxy_params.out,
                                 HYD_STDOUT, HYD_PMCD_pmi_proxy_stdout_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    status = HYD_DMX_register_fd(HYD_PMCD_pmi_proxy_params.exec_proc_count,
                                 HYD_PMCD_pmi_proxy_params.err,
                                 HYD_STDOUT, HYD_PMCD_pmi_proxy_stderr_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    while (1) {
        /* Wait for some event to occur */
        status = HYD_DMX_wait_for_event(-1);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");

        /* Check to see if there's any open read socket left; if there
         * are, we will just wait for more events. */
        count = 0;
        for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++) {
            if (HYD_PMCD_pmi_proxy_params.out[i] != -1 ||
                HYD_PMCD_pmi_proxy_params.err[i] != -1) {
                count++;
                break;
            }
        }

        /* We are done */
        if (!count)
            break;
    }

    /* FIXME: If we did not break out yet, add a small usleep to yield
     * CPU here. We can not just sleep for the remaining time, as the
     * timeout value might be large and the application might exit
     * much quicker. Note that the sched_yield() call is broken on
     * newer linux kernel versions and should not be used. */
    /* Once all the sockets are closed, wait for all the processes to
     * finish. We poll here, but hopefully not for too long. */
    do {
        pid = waitpid(-1, &ret_status, WNOHANG);

        /* Find the pid and mark it as complete. */
        if (pid > 0)
            for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++)
                if (HYD_PMCD_pmi_proxy_params.pid[i] == pid)
                    HYD_PMCD_pmi_proxy_params.exit_status[i] = WEXITSTATUS(ret_status);

        /* Check how many more processes are pending */
        count = 0;
        for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++) {
            if (HYD_PMCD_pmi_proxy_params.exit_status[i] == -1) {
                count++;
                break;
            }
        }

        if (count == 0)
            break;

        /* Check if there are any messages from the launcher */
        status = HYD_DMX_wait_for_event(0);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");
    } while (1);

    ret_status = 0;
    for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++)
        ret_status |= HYD_PMCD_pmi_proxy_params.exit_status[i];

  fn_exit:
    if (status != HYD_SUCCESS)
        return -1;
    else
        return ret_status;

  fn_fail:
    goto fn_exit;
}
