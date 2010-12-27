/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "pmci.h"
#include "bsci.h"
#include "hydt_ftb.h"
#include "demux.h"
#include "uiu.h"

struct HYD_handle HYD_handle = { {0} };

struct HYD_exec *HYD_uii_mpx_exec_list = NULL;

static void usage(void)
{
    printf("\n");
    printf("Usage: ./mpiexec [global opts] [exec1 local opts] : [exec2 local opts] : ...\n\n");

    printf("Global options (passed to all executables):\n");

    printf("\n");
    printf("  Global environment options:\n");
    printf("    -genv {name} {value}             environment variable name and value\n");
    printf("    -genvlist {env1,env2,...}        environment variable list to pass\n");
    printf("    -genvnone                        do not pass any environment variables\n");
    printf("    -genvall                         pass all environment variables (default)\n");

    printf("\n");
    printf("  Other global options:\n");
    printf("    -f {name}                        file containing the host names\n");
    printf("    -hosts {host list}               comma separated host list\n");
    printf("    -wdir {dirname}                  working directory to use\n");
    printf
        ("    -configfile {name}               config file containing MPMD launch options\n");

    printf("\n");
    printf("\n");

    printf("Local options (passed to individual executables):\n");

    printf("\n");
    printf("  Local environment options:\n");
    printf("    -env {name} {value}              environment variable name and value\n");
    printf("    -envlist {env1,env2,...}         environment variable list to pass\n");
    printf("    -envnone                         do not pass any environment variables\n");
    printf("    -envall                          pass all environment variables (default)\n");

    printf("\n");
    printf("  Other local options:\n");
    printf("    -n/-np {value}                   number of processes\n");
    printf("    {exec_name} {args}               executable name and arguments\n");

    printf("\n");
    printf("\n");

    printf("Hydra specific options (treated as global):\n");

    printf("\n");
    printf("  Launch options:\n");
    printf("    -launcher                        launcher to use (%s)\n",
           HYDRA_AVAILABLE_LAUNCHERS);
    printf("    -launcher-exec                   executable to use to launch processes\n");
    printf("    -enable-x/-disable-x             enable or disable X forwarding\n");

    printf("\n");
    printf("  Resource management kernel options:\n");
    printf("    -rmk                             resource management kernel to use (%s)\n",
           HYDRA_AVAILABLE_RMKS);

    printf("\n");
    printf("  Hybrid programming options:\n");
    printf("    -ranks-per-proc                  assign so many ranks to each process\n");

    printf("\n");
    printf("  Process-core binding options:\n");
    printf("    -binding                         process-to-core binding mode\n");
    printf("    -bindlib                         process-to-core binding library (%s)\n",
           HYDRA_AVAILABLE_BINDLIBS);

    printf("\n");
    printf("  Checkpoint/Restart options:\n");
    printf("    -ckpoint-interval                checkpoint interval\n");
    printf("    -ckpoint-prefix                  checkpoint file prefix\n");
    printf("    -ckpoint-num                     checkpoint number to restart\n");
    printf("    -ckpointlib                      checkpointing library (%s)\n",
           !strcmp(HYDRA_AVAILABLE_CKPOINTLIBS, "") ? "none" : HYDRA_AVAILABLE_CKPOINTLIBS);

    printf("\n");
    printf("  Demux engine options:\n");
    printf("    -demux                           demux engine (%s)\n",
           HYDRA_AVAILABLE_DEMUXES);

    printf("\n");
    printf("  Other Hydra options:\n");
    printf("    -verbose                         verbose mode\n");
    printf("    -info                            build information\n");
    printf("    -print-rank-map                  print rank mapping\n");
    printf("    -print-all-exitcodes             print exit codes of all processes\n");
    printf("    -iface                           network interface to use\n");
    printf("    -ppn                             processes per node\n");
    printf("    -profile                         turn on internal profiling\n");
    printf("    -prepend-rank                    prepend rank to output\n");
    printf
        ("    -nameserver                      name server information (host:port format)\n");
    printf("    -disable-auto-cleanup            don't cleanup processes on error\n");
    printf("    -disable-hostname-propagation    let MPICH2 auto-detect the hostname\n");
    printf("    -order-nodes                     order nodes as ascending/descending cores\n");
}

static void signal_cb(int sig)
{
    struct HYD_cmd cmd;
    int sent, closed;

    HYDU_FUNC_ENTER();

    if (sig == SIGINT || sig == SIGQUIT || sig == SIGTERM) {
        /* We don't want to send the abort signal directly to the
         * proxies, since that would require two processes (this
         * asynchronous process and the main process) to use the same
         * control socket, potentially resulting in data
         * corruption. The strategy we use is to set the abort flag
         * and send a signal to ourselves. The signal callback does
         * the process cleanup instead. */
        cmd.type = HYD_CLEANUP;
        HYDU_sock_write(HYD_handle.cleanup_pipe[1], &cmd, sizeof(cmd), &sent, &closed);
    }
    else if (sig == SIGUSR1 || sig == SIGALRM) {
        if (HYD_handle.user_global.ckpoint_prefix == NULL) {
            HYDU_dump(stderr, "No checkpoint prefix provided\n");
            return;
        }

#if HAVE_ALARM
        if (HYD_handle.ckpoint_int != -1)
            alarm(HYD_handle.ckpoint_int);
#endif /* HAVE_ALARM */

        cmd.type = HYD_CKPOINT;
        HYDU_sock_write(HYD_handle.cleanup_pipe[1], &cmd, sizeof(cmd), &sent, &closed);
    }
    /* Ignore all other signals */

    HYDU_FUNC_EXIT();
    return;
}

#define ordered(n1, n2) \
    (((HYD_handle.sort_order == ASCENDING) && ((n1)->core_count <= (n2)->core_count)) || \
     ((HYD_handle.sort_order == DESCENDING) && ((n1)->core_count >= (n2)->core_count)))

static void append_node_to_list(struct HYD_node *node, struct HYD_node **list)
{
    struct HYD_node *r1;

    if (*list == NULL) {
        *list = node;
        (*list)->next = NULL;
    }
    else if (!ordered(*list, node)) {
        node->next = *list;
        *list = node;
    }
    else {
        for (r1 = *list; r1->next && ordered(r1->next, node); r1 = r1->next);
        node->next = r1->next;
        r1->next = node;
    }
}

static void sort_node_list(void)
{
    struct HYD_node *r1, *node, *new_list;
    int sorted;

    while (1) {
        new_list = NULL;

        for (node = HYD_handle.node_list; node;) {
            r1 = node->next;
            append_node_to_list(node, &new_list);
            node = r1;
        }
        HYD_handle.node_list = new_list;

        /* Check to make sure the nodes are sorted. They might not be
         * sorted if there are duplicate hostname entries. */
        sorted = 1;
        for (node = HYD_handle.node_list; node; node = node->next) {
            if (node->next && !ordered(node, node->next)) {
                sorted = 0;
                break;
            }
        }

        if (sorted)
            break;
    }

    HYD_handle.node_list = new_list;
}

int main(int argc, char **argv)
{
    struct HYD_proxy *proxy;
    struct HYD_exec *exec;
    struct HYD_node *node;
    int exit_status = 0, i, process_id, proc_count, timeout, reset_rmk;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Initialize engines that don't require use to know anything
     * about the user preferences first */
    status = HYDU_dbg_init("mpiexec");
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

    status = HYDU_set_common_signals(signal_cb);
    HYDU_ERR_POP(status, "unable to set signal\n");

    if (pipe(HYD_handle.cleanup_pipe) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "pipe error\n");

    status = HYDT_ftb_init();
    HYDU_ERR_POP(status, "unable to initialize FTB\n");


    /* Get user preferences */
    status = HYD_uii_mpx_get_parameters(argv);
    if (status == HYD_GRACEFUL_ABORT) {
        exit(0);
    }
    else if (status != HYD_SUCCESS) {
        usage();
        goto fn_fail;
    }


    /* Now we initialize engines that require us to know user
     * preferences */
#if HAVE_ALARM
    if (HYD_handle.ckpoint_int != -1)
        alarm(HYD_handle.ckpoint_int);
#endif /* HAVE_ALARM */

    status = HYDT_dmx_init(&HYD_handle.user_global.demux);
    HYDU_ERR_POP(status, "unable to initialize the demux engine\n");

    status = HYDT_bsci_init(HYD_handle.user_global.rmk, HYD_handle.user_global.launcher,
                            HYD_handle.user_global.launcher_exec,
                            HYD_handle.user_global.enablex, HYD_handle.user_global.debug);
    HYDU_ERR_POP(status, "unable to initialize the bootstrap server\n");

    reset_rmk = 0;

    if (HYD_handle.node_list == NULL) {
        /* Node list is not created yet. The user might not have
         * provided the host file. Query the RMK. */
        status = HYDT_bsci_query_node_list(&HYD_handle.node_list);
        HYDU_ERR_POP(status, "unable to query the RMK for a node list\n");

        if (HYD_handle.node_list == NULL) {
            /* The RMK didn't give us anything back; use localhost */
            status = HYDU_add_to_node_list("localhost", 1, &HYD_handle.node_list);
            HYDU_ERR_POP(status, "unable to add to node list\n");

            reset_rmk = 1;
        }
    }

    /* Reset the host list to use only the number of processes per
     * node as specified by the ppn option. */
    if (HYD_handle.ppn != -1)
        for (node = HYD_handle.node_list; node; node = node->next)
            node->core_count = HYD_handle.ppn;

    /* The RMK returned a node list. See if the user requested us to
     * manipulate it in some way */
    if (HYD_handle.sort_order != NONE) {
        sort_node_list();
        reset_rmk = 1;
    }

    if (reset_rmk) {
        /* Reinitialize the bootstrap server with the "none" RMK, so
         * it knows that we are not using the node list provided by
         * the RMK */
        status = HYDT_bsci_finalize();
        HYDU_ERR_POP(status, "unable to finalize bootstrap device\n");

        status = HYDT_bsci_init("none", HYD_handle.user_global.launcher,
                                HYD_handle.user_global.launcher_exec,
                                HYD_handle.user_global.enablex, HYD_handle.user_global.debug);
        HYDU_ERR_POP(status, "unable to reinitialize the bootstrap server\n");
    }

    HYD_handle.global_core_count = 0;
    for (node = HYD_handle.node_list; node; node = node->next)
        HYD_handle.global_core_count += node->core_count;

    /* If the number of processes is not given, we allocate all the
     * available nodes to each executable */
    for (exec = HYD_uii_mpx_exec_list; exec; exec = exec->next) {
        if (exec->proc_count == -1) {
            if (HYD_handle.global_core_count == 0)
                exec->proc_count = 1;
            else
                exec->proc_count = HYD_handle.global_core_count;

            /* If we didn't get anything from the user, take whatever
             * the RMK gave */
            HYD_handle.pg_list.pg_process_count += exec->proc_count;
        }
    }

    status = HYDU_list_inherited_env(&HYD_handle.user_global.global_env.inherited);
    HYDU_ERR_POP(status, "unable to get the inherited env list\n");

    status = HYDU_create_proxy_list(HYD_uii_mpx_exec_list, HYD_handle.node_list,
                                    &HYD_handle.pg_list, 0);
    HYDU_ERR_POP(status, "unable to create proxy list\n");

    if (HYD_handle.user_global.debug)
        HYD_uiu_print_params();

    if (MPL_env2int("MPIEXEC_TIMEOUT", &timeout) == 0)
        timeout = -1;   /* Infinite timeout */

    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "Timeout set to %d (-1 means infinite)\n", timeout);

    /* Check if the user wants us to use a port within a certain
     * range. */
    if (MPL_env2str("MPIEXEC_PORTRANGE", (const char **) &HYD_handle.port_range) ||
        MPL_env2str("MPIEXEC_PORT_RANGE", (const char **) &HYD_handle.port_range) ||
        MPL_env2str("MPICH_PORT_RANGE", (const char **) &HYD_handle.port_range))
        HYD_handle.port_range = HYDU_strdup(HYD_handle.port_range);

    if (HYD_handle.print_rank_map) {
        for (proxy = HYD_handle.pg_list.proxy_list; proxy; proxy = proxy->next) {
            HYDU_dump_noprefix(stdout, "(%s:", proxy->node.hostname);

            process_id = 0;
            for (exec = proxy->exec_list; exec; exec = exec->next) {
                for (i = 0; i < exec->proc_count; i++) {
                    HYDU_dump_noprefix(stdout, "%d",
                                       HYDU_local_to_global_id(process_id++,
                                                               proxy->start_pid,
                                                               proxy->node.core_count,
                                                               HYD_handle.global_core_count));
                    if (i < exec->proc_count - 1 || exec->next)
                        HYDU_dump_noprefix(stdout, ",");
                }
            }
            HYDU_dump_noprefix(stdout, ")\n");
        }
    }

    /* Add the stdout/stdin/stderr callback handlers */
    HYD_handle.stdout_cb = HYD_uiu_stdout_cb;
    HYD_handle.stderr_cb = HYD_uiu_stderr_cb;

    /* Launch the processes */
    status = HYD_pmci_launch_procs();
    HYDU_ERR_POP(status, "process manager returned error launching processes\n");

    /* Wait for their completion */
    status = HYD_pmci_wait_for_completion(timeout);
    HYDU_ERR_POP(status, "process manager error waiting for completion\n");

    /* Check for the exit status for all the processes */
    if (HYD_handle.print_all_exitcodes)
        HYDU_dump(stdout, "Exit codes: ");
    exit_status = 0;
    for (proxy = HYD_handle.pg_list.proxy_list; proxy; proxy = proxy->next) {
        if (proxy->exit_status == NULL) {
            /* We didn't receive the exit status for this proxy */
            exit_status |= 1;
            continue;
        }

        proc_count = 0;
        for (exec = proxy->exec_list; exec; exec = exec->next)
            proc_count += exec->proc_count;
        for (i = 0; i < proc_count; i++) {
            if (HYD_handle.print_all_exitcodes) {
                HYDU_dump_noprefix(stdout, "[%d]",
                                   HYDU_local_to_global_id(i, proxy->start_pid,
                                                           proxy->node.core_count,
                                                           HYD_handle.global_core_count));
                HYDU_dump_noprefix(stdout, "%d", WEXITSTATUS(proxy->exit_status[i]));
                if (i < proc_count - 1)
                    HYDU_dump_noprefix(stdout, ",");
            }
            exit_status |= proxy->exit_status[i];
        }
    }
    if (HYD_handle.print_all_exitcodes)
        HYDU_dump_noprefix(stdout, "\n");

    /* Call finalize functions for lower layers to cleanup their resources */
    status = HYD_pmci_finalize();
    HYDU_ERR_POP(status, "process manager error on finalize\n");

    status = HYDT_ftb_finalize();
    HYDU_ERR_POP(status, "error finalizing FTB\n");

#if defined ENABLE_PROFILING
    if (HYD_handle.enable_profiling) {
        HYDU_dump_noprefix(stdout, "\n");
        HYD_DRAW_LINE(80);
        HYDU_dump(stdout, "Number of PMI calls seen by the server: %d\n",
                  HYD_handle.num_pmi_calls);
        HYD_DRAW_LINE(80);
        HYDU_dump_noprefix(stdout, "\n");
    }
#endif /* ENABLE_PROFILING */

    /* Free the mpiexec params */
    HYD_uiu_free_params();
    HYDU_free_exec_list(HYD_uii_mpx_exec_list);

  fn_exit:
    HYDU_dbg_finalize();
    HYDU_FUNC_EXIT();
    if (status == HYD_GRACEFUL_ABORT)
        return 0;
    else if (status != HYD_SUCCESS)
        return -1;
    else {
        if (WIFSIGNALED(exit_status)) {
            printf("APPLICATION TERMINATED WITH THE EXIT STRING: %s (signal %d)\n",
                   strsignal(WTERMSIG(exit_status)), WTERMSIG(exit_status));
        }
        else if (WIFEXITED(exit_status))
            return (WEXITSTATUS(exit_status));
        else if (WIFSTOPPED(exit_status))
            return (WSTOPSIG(exit_status));
        return exit_status;
    }

  fn_fail:
    goto fn_exit;
}
