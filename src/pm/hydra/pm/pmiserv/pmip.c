/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmip.h"
#include "demux.h"
#include "bind.h"

struct HYD_pmcd_pmip HYD_pmcd_pmip;

static HYD_status init_params(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_init_user_global(&HYD_pmcd_pmip.user_global);

    HYD_pmcd_pmip.system_global.enable_stdin = -1;
    HYD_pmcd_pmip.system_global.global_core_count = -1;
    HYD_pmcd_pmip.system_global.pmi_port = NULL;
    HYD_pmcd_pmip.system_global.pmi_id = -1;
    HYD_pmcd_pmip.system_global.pmi_kvsname = NULL;

    HYD_pmcd_pmip.upstream.server_name = NULL;
    HYD_pmcd_pmip.upstream.server_port = -1;
    HYD_pmcd_pmip.upstream.control = -1;

    HYD_pmcd_pmip.downstream.out = NULL;
    HYD_pmcd_pmip.downstream.err = NULL;
    HYD_pmcd_pmip.downstream.in = -1;
    HYD_pmcd_pmip.downstream.pid = NULL;
    HYD_pmcd_pmip.downstream.exit_status = NULL;
    HYD_pmcd_pmip.downstream.pmi_id = NULL;
    HYD_pmcd_pmip.downstream.pmi_fd = NULL;

    HYD_pmcd_pmip.local.id = -1;
    HYD_pmcd_pmip.local.pgid = -1;
    HYD_pmcd_pmip.local.interface_env_name = NULL;
    HYD_pmcd_pmip.local.hostname = NULL;
    HYD_pmcd_pmip.local.local_binding = NULL;
    HYD_pmcd_pmip.local.proxy_core_count = -1;
    HYD_pmcd_pmip.local.proxy_process_count = -1;

    HYD_pmcd_pmip.start_pid = -1;
    HYD_pmcd_pmip.exec_list = NULL;

    return status;
}

static void cleanup_params(void)
{
    struct HYD_exec *exec, *texec;

    HYDU_FUNC_ENTER();

    if (HYD_pmcd_pmip.upstream.server_name)
        HYDU_FREE(HYD_pmcd_pmip.upstream.server_name);

    if (HYD_pmcd_pmip.user_global.bootstrap)
        HYDU_FREE(HYD_pmcd_pmip.user_global.bootstrap);

    if (HYD_pmcd_pmip.user_global.bootstrap_exec)
        HYDU_FREE(HYD_pmcd_pmip.user_global.bootstrap_exec);

    if (HYD_pmcd_pmip.system_global.pmi_port)
        HYDU_FREE(HYD_pmcd_pmip.system_global.pmi_port);

    if (HYD_pmcd_pmip.system_global.pmi_kvsname)
        HYDU_FREE(HYD_pmcd_pmip.system_global.pmi_kvsname);

    if (HYD_pmcd_pmip.user_global.binding)
        HYDU_FREE(HYD_pmcd_pmip.user_global.binding);

    if (HYD_pmcd_pmip.user_global.bindlib)
        HYDU_FREE(HYD_pmcd_pmip.user_global.bindlib);

    if (HYD_pmcd_pmip.user_global.ckpointlib)
        HYDU_FREE(HYD_pmcd_pmip.user_global.ckpointlib);

    if (HYD_pmcd_pmip.user_global.ckpoint_prefix)
        HYDU_FREE(HYD_pmcd_pmip.user_global.ckpoint_prefix);

    if (HYD_pmcd_pmip.user_global.demux)
        HYDU_FREE(HYD_pmcd_pmip.user_global.demux);

    if (HYD_pmcd_pmip.user_global.iface)
        HYDU_FREE(HYD_pmcd_pmip.user_global.iface);

    if (HYD_pmcd_pmip.user_global.global_env.system)
        HYDU_env_free_list(HYD_pmcd_pmip.user_global.global_env.system);

    if (HYD_pmcd_pmip.user_global.global_env.user)
        HYDU_env_free_list(HYD_pmcd_pmip.user_global.global_env.user);

    if (HYD_pmcd_pmip.user_global.global_env.inherited)
        HYDU_env_free_list(HYD_pmcd_pmip.user_global.global_env.inherited);

    if (HYD_pmcd_pmip.exec_list) {
        exec = HYD_pmcd_pmip.exec_list;
        while (exec) {
            texec = exec->next;
            HYDU_free_strlist(exec->exec);
            if (exec->user_env)
                HYDU_env_free(exec->user_env);
            HYDU_FREE(exec);
            exec = texec;
        }
    }

    if (HYD_pmcd_pmip.downstream.pid)
        HYDU_FREE(HYD_pmcd_pmip.downstream.pid);

    if (HYD_pmcd_pmip.downstream.out)
        HYDU_FREE(HYD_pmcd_pmip.downstream.out);

    if (HYD_pmcd_pmip.downstream.err)
        HYDU_FREE(HYD_pmcd_pmip.downstream.err);

    if (HYD_pmcd_pmip.downstream.exit_status)
        HYDU_FREE(HYD_pmcd_pmip.downstream.exit_status);

    if (HYD_pmcd_pmip.downstream.pmi_id)
        HYDU_FREE(HYD_pmcd_pmip.downstream.pmi_id);

    if (HYD_pmcd_pmip.downstream.pmi_fd)
        HYDU_FREE(HYD_pmcd_pmip.downstream.pmi_fd);

    if (HYD_pmcd_pmip.local.interface_env_name)
        HYDU_FREE(HYD_pmcd_pmip.local.interface_env_name);

    if (HYD_pmcd_pmip.local.hostname)
        HYDU_FREE(HYD_pmcd_pmip.local.hostname);

    if (HYD_pmcd_pmip.local.local_binding)
        HYDU_FREE(HYD_pmcd_pmip.local.local_binding);

    HYDT_bind_finalize();

    HYDU_FUNC_EXIT();
}

int main(int argc, char **argv)
{
    int i, count, pid, ret_status;
    enum HYD_pmcd_pmi_cmd cmd;
    HYD_status status = HYD_SUCCESS;

    status = HYDU_dbg_init("proxy");
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

    status = init_params();
    HYDU_ERR_POP(status, "Error initializing proxy params\n");

    status = HYD_pmcd_pmip_get_params(argv);
    HYDU_ERR_POP(status, "bad parameters passed to the proxy\n");

    /* Connect back upstream and add the socket to the demux engine */
    status = HYDU_sock_connect(HYD_pmcd_pmip.upstream.server_name,
                               HYD_pmcd_pmip.upstream.server_port,
                               &HYD_pmcd_pmip.upstream.control);
    HYDU_ERR_POP(status, "unable to connect to the main server\n");

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control,
                             &HYD_pmcd_pmip.local.id, sizeof(HYD_pmcd_pmip.local.id));
    HYDU_ERR_POP(status, "unable to send the proxy ID to the server\n");

    status = HYDT_dmx_register_fd(1, &HYD_pmcd_pmip.upstream.control,
                                  HYD_POLLIN, NULL, HYD_pmcd_pmip_control_cmd_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    while (1) {
        /* Wait for some event to occur */
        status = HYDT_dmx_wait_for_event(-1);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");

        /* Check to see if there's any open read socket left; if there
         * are, we will just wait for more events. */
        count = 0;
        for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
            if (HYD_pmcd_pmip.downstream.out[i] != -1)
                count++;
            if (HYD_pmcd_pmip.downstream.err[i] != -1)
                count++;

            if (count)
                break;
        }
        if (!count)
            break;
    }

    /* Now wait for the processes to finish */
    while (1) {
        pid = waitpid(-1, &ret_status, 0);

        /* Find the pid and mark it as complete. */
        if (pid > 0)
            for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
                if (HYD_pmcd_pmip.downstream.pid[i] == pid)
                    HYD_pmcd_pmip.downstream.exit_status[i] = ret_status;

        /* Check how many more processes are pending */
        count = 0;
        for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
            if (HYD_pmcd_pmip.downstream.exit_status[i] == -1) {
                count++;
                break;
            }
        }

        if (count == 0)
            break;

        /* Check if there are any messages from the launcher */
        status = HYDT_dmx_wait_for_event(0);
        HYDU_IGNORE_TIMEOUT(status);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");
    }

    /* Send the exit status upstream */
    cmd = EXIT_STATUS;
    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &cmd, sizeof(cmd));
    HYDU_ERR_POP(status, "unable to send EXIT_STATUS command upstream\n");

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control,
                             HYD_pmcd_pmip.downstream.exit_status,
                             HYD_pmcd_pmip.local.proxy_process_count * sizeof(int));
    HYDU_ERR_POP(status, "unable to return exit status upstream\n");

    status = HYDT_dmx_deregister_fd(HYD_pmcd_pmip.upstream.control);
    HYDU_ERR_POP(status, "unable to deregister fd\n");
    close(HYD_pmcd_pmip.upstream.control);

    /* cleanup the params structure */
    cleanup_params();

  fn_exit:
    HYDU_dbg_finalize();
    return status;

  fn_fail:
    HYD_pmcd_pmip_killjob();
    goto fn_exit;
}
