/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_server.h"
#include "hydra.h"
#include "mpiexec.h"
#include "pmci.h"
#include "bsci.h"
#include "demux.h"
#include "ui.h"
#include "uiu.h"
#include "rmk.h"

struct HYD_server_info_s HYD_server_info = { {0} };

struct HYD_exec *HYD_uii_mpx_exec_list = NULL;
struct HYD_ui_info_s HYD_ui_info;
struct HYD_ui_mpich_info_s HYD_ui_mpich_info;

static void signal_cb(int signum)
{
    struct HYD_cmd cmd;
    static int sigint_count = 0;
    int sent, closed;

    HYDU_FUNC_ENTER();

    cmd.type = HYD_SIGNAL;
    cmd.signum = signum;

    /* SIGINT is a partially special signal. The first time we see it,
     * we will send it to the processes. The next time, we will treat
     * it as a SIGKILL (user convenience to force kill processes). */
    if (signum == SIGINT && ++sigint_count > 1)
        cmd.type = HYD_CLEANUP;
    else if (signum == SIGINT) {
        /* First Ctrl-C */
        HYDU_dump(stdout, "Sending Ctrl-C to processes as requested\n");
        HYDU_dump(stdout, "Press Ctrl-C again to force abort\n");
    }

    HYDU_sock_write(HYD_server_info.cmd_pipe[1], &cmd, sizeof(cmd), &sent, &closed,
                    HYDU_SOCK_COMM_MSGWAIT);

  fn_exit:
    HYDU_FUNC_EXIT();
    return;
}

int main(int argc, char **argv)
{
    struct HYD_proxy *proxy;
    struct HYD_exec *exec;
    struct HYD_node *node;
    int exit_status = 0, i, timeout, global_core_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Initialize engines that don't require use to know anything
     * about the user preferences first */
    status = HYDU_dbg_init("mpiexec");
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

    status = HYDU_set_common_signals(signal_cb);
    HYDU_ERR_POP(status, "unable to set signal\n");

    /* Get user preferences */
    status = HYD_uii_mpx_get_parameters(argv);
    HYDU_ERR_POP(status, "error parsing parameters\n");

    /* The demux engine should be initialized before any sockets are
     * created, since it checks for STDIN's validity.  If STDIN was
     * closed and we opened a socket that got the same fd as STDIN,
     * this test will not be possible. */
    status = HYDT_dmx_init(&HYD_server_info.user_global.demux);
    HYDU_ERR_POP(status, "unable to initialize the demux engine\n");

    if (HYD_server_info.node_list) {
        /* if we already have the node list filled out, it had to come
         * from the user */
        HYD_server_info.user_global.rmk = MPL_strdup("user");
    }
    else {
        const char *rmk;

        /* Node list is not created yet. The user might not have
         * provided the host file. Find an RMK to query. */

        /* check for environment settings */
        if (HYD_server_info.user_global.rmk == NULL)
            MPL_env2str("HYDRA_RMK", (const char **) &HYDT_bsci_info.rmk);

        /* try to autodetect */
        if (HYD_server_info.user_global.rmk == NULL) {
            rmk = HYDT_rmk_detect();
            if (rmk)
                HYD_server_info.user_global.rmk = MPL_strdup(rmk);
        }

        /* if we found an RMK, ask it for a list of nodes */
        if (HYD_server_info.user_global.rmk)
            HYDT_rmk_query_node_list(HYD_server_info.user_global.rmk, &HYD_server_info.node_list);

        /* if we didn't find anything, use localhost */
        if (HYD_server_info.node_list == NULL) {
            char localhost[MAX_HOSTNAME_LEN] = { 0 };

            /* The RMK didn't give us anything back; use localhost */
            if (gethostname(localhost, MAX_HOSTNAME_LEN) < 0)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "unable to get local hostname\n");

            status = HYDU_add_to_node_list(localhost, 1, &HYD_server_info.node_list);
            HYDU_ERR_POP(status, "unable to add to node list\n");

            HYD_server_info.user_global.rmk = MPL_strdup("user");
        }
    }

    /* we are ready to initialize the bootstrap server now */
    status =
        HYDT_bsci_init(HYD_server_info.user_global.launcher,
                       HYD_server_info.user_global.launcher_exec,
                       HYD_server_info.user_global.enablex, HYD_server_info.user_global.debug);
    HYDU_ERR_POP(status, "unable to initialize the bootstrap server\n");

    if (HYD_server_info.user_global.debug)
        for (node = HYD_server_info.node_list; node; node = node->next)
            HYDU_dump_noprefix(stdout, "host: %s\n", node->hostname);

    /* Reset the host list to use only the number of processes per
     * node as specified by the ppn option. */
    if (HYD_ui_mpich_info.ppn != -1) {
        for (node = HYD_server_info.node_list; node; node = node->next)
            node->core_count = HYD_ui_mpich_info.ppn;
    }

    /* If the number of processes is not given, we allocate all the
     * available nodes to each executable */
    HYD_server_info.pg_list.pg_process_count = 0;
    for (exec = HYD_uii_mpx_exec_list; exec; exec = exec->next) {
        if (exec->proc_count == -1) {
            global_core_count = 0;
            for (node = HYD_server_info.node_list, i = 0; node; node = node->next, i++)
                global_core_count += node->core_count;
            exec->proc_count = global_core_count;
        }
        HYD_server_info.pg_list.pg_process_count += exec->proc_count;
    }

    status = HYDU_list_inherited_env(&HYD_server_info.user_global.global_env.inherited);
    HYDU_ERR_POP(status, "unable to get the inherited env list\n");

    status = HYDU_create_proxy_list(HYD_uii_mpx_exec_list, HYD_server_info.node_list,
                                    &HYD_server_info.pg_list);
    HYDU_ERR_POP(status, "unable to create proxy list\n");

    /* calculate the core count used by the PG */
    HYD_server_info.pg_list.pg_core_count = 0;
    for (proxy = HYD_server_info.pg_list.proxy_list; proxy; proxy = proxy->next)
        HYD_server_info.pg_list.pg_core_count += proxy->node->core_count;

    /* If the user didn't specify a local hostname, try to find one in
     * the list of nodes passed to us */
    if (HYD_server_info.localhost == NULL) {
        /* See if the node list contains a localhost */
        for (node = HYD_server_info.node_list; node; node = node->next)
            if (MPL_host_is_local(node->hostname))
                break;

        if (node)
            HYD_server_info.localhost = MPL_strdup(node->hostname);
        else {
            HYDU_MALLOC_OR_JUMP(HYD_server_info.localhost, char *, MAX_HOSTNAME_LEN, status);
            if (gethostname(HYD_server_info.localhost, MAX_HOSTNAME_LEN) < 0)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "unable to get local hostname\n");
        }
    }

    if (HYD_server_info.user_global.debug)
        HYD_uiu_print_params();

    if (MPL_env2int("MPIEXEC_TIMEOUT", &timeout) == 0)
        timeout = -1;   /* Infinite timeout */

    if (HYD_server_info.user_global.debug)
        HYDU_dump(stdout, "Timeout set to %d (-1 means infinite)\n", timeout);

    /* Check if the user wants us to use a port within a certain
     * range. */
    if (MPL_env2str("MPIR_CVAR_CH3_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIR_PARAM_CH3_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPICH_CH3_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIR_CVAR_PORTRANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIR_CVAR_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIR_PARAM_PORTRANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIR_PARAM_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPICH_PORTRANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPICH_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIEXEC_PORTRANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIEXEC_PORT_RANGE", (const char **) &HYD_server_info.port_range))
        HYD_server_info.port_range = MPL_strdup(HYD_server_info.port_range);

    /* Add the stdout/stderr callback handlers */
    HYD_server_info.stdout_cb = HYD_uiu_stdout_cb;
    HYD_server_info.stderr_cb = HYD_uiu_stderr_cb;

    /* Create a pipe connection to wake up the process manager */
    if (pipe(HYD_server_info.cmd_pipe) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "pipe error\n");

    /* Launch the processes */
    status = HYD_pmci_launch_procs();
    HYDU_ERR_POP(status, "process manager returned error launching processes\n");

    /* Wait for their completion */
    status = HYD_pmci_wait_for_completion(timeout);
    HYDU_ERR_POP(status, "process manager error waiting for completion\n");

    /* Check for the exit status for all the processes */
    if (HYD_ui_mpich_info.print_all_exitcodes)
        HYDU_dump(stdout, "Exit codes: ");
    exit_status = 0;
    for (proxy = HYD_server_info.pg_list.proxy_list; proxy; proxy = proxy->next) {
        if (proxy->exit_status == NULL) {
            /* We didn't receive the exit status for this proxy */
            continue;
        }

        if (HYD_ui_mpich_info.print_all_exitcodes)
            HYDU_dump_noprefix(stdout, "[%s] ", proxy->node->hostname);

        for (i = 0; i < proxy->proxy_process_count; i++) {
            if (HYD_ui_mpich_info.print_all_exitcodes) {
                HYDU_dump_noprefix(stdout, "%d", proxy->exit_status[i]);
                if (i < proxy->proxy_process_count - 1)
                    HYDU_dump_noprefix(stdout, ",");
            }

            exit_status |= proxy->exit_status[i];
        }

        if (HYD_ui_mpich_info.print_all_exitcodes)
            HYDU_dump_noprefix(stdout, "\n");
    }

    /* Call finalize functions for lower layers to cleanup their resources */
    status = HYD_pmci_finalize();
    HYDU_ERR_POP(status, "process manager error on finalize\n");

    /* Free the mpiexec params */
    HYD_uiu_free_params();
    HYDU_free_exec_list(HYD_uii_mpx_exec_list);
    HYDU_sock_finalize();

  fn_exit:
    HYDU_dbg_finalize();
    HYDU_FUNC_EXIT();
    if (status == HYD_GRACEFUL_ABORT)
        return 0;
    else if (status != HYD_SUCCESS)
        return -1;
    else if (WIFSIGNALED(exit_status)) {
        printf("YOUR APPLICATION TERMINATED WITH THE EXIT STRING: %s (signal %d)\n",
               strsignal(WTERMSIG(exit_status)), WTERMSIG(exit_status));
        printf("This typically refers to a problem with your application.\n");
        printf("Please see the FAQ page for debugging suggestions\n");
        return exit_status;
    }
    else if (WIFEXITED(exit_status)) {
        return WEXITSTATUS(exit_status);
    }
    else if (WIFSTOPPED(exit_status)) {
        return WSTOPSIG(exit_status);
    }
    else {
        return exit_status;
    }

  fn_fail:
    goto fn_exit;
}
