/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_server.h"
#include "hydra.h"
#include "mpiexec.h"
#include "pmci.h"
#include "bsci.h"
#include "hydt_ftb.h"
#include "demux.h"
#include "ui.h"
#include "uiu.h"

struct HYD_server_info HYD_server_info = { {0} };

struct HYD_exec *HYD_uii_mpx_exec_list = NULL;
struct HYD_ui_info HYD_ui_info;
struct HYD_ui_mpich_info HYD_ui_mpich_info;

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
    printf
        ("    -genvall                         pass all environment variables not managed\n");
    printf("                                          by the launcher (default)\n");

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
    printf("  Processor topology options:\n");
    printf("    -binding                         process-to-core binding mode\n");
    printf("    -topolib                         processor topology library (%s)\n",
           HYDRA_AVAILABLE_TOPOLIBS);

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
    printf("    -print-all-exitcodes             print exit codes of all processes\n");
    printf("    -iface                           network interface to use\n");
    printf("    -ppn                             processes per node\n");
    printf("    -profile                         turn on internal profiling\n");
    printf("    -prepend-rank                    prepend rank to output\n");
    printf("    -prepend-pattern                 prepend pattern to output\n");
    printf("    -outfile-pattern                 direct stdout to file\n");
    printf("    -errfile-pattern                 direct stderr to file\n");
    printf
        ("    -nameserver                      name server information (host:port format)\n");
    printf("    -disable-auto-cleanup            don't cleanup processes on error\n");
    printf("    -disable-hostname-propagation    let MPICH2 auto-detect the hostname\n");
    printf("    -order-nodes                     order nodes as ascending/descending cores\n");

    printf("\n");
    printf("Please see the intructions provided at\n");
    printf("http://wiki.mcs.anl.gov/mpich2/index.php/Using_the_Hydra_Process_Manager\n");
    printf("for further details\n\n");
}

static void signal_cb(int signum)
{
    struct HYD_cmd cmd;
    int sent, closed;

    HYDU_FUNC_ENTER();

    if (signum == SIGINT) {
        /* SIGINT is a special signal which we don't send to the
         * proxies directly. This is our fallback path if something
         * went wrong with Hydra and the user wants to abort. */
        cmd.type = HYD_CLEANUP;
        HYDU_sock_write(HYD_server_info.cleanup_pipe[1], &cmd, sizeof(cmd), &sent, &closed);
    }
    else if (signum == SIGALRM) {
        /* SIGALRM is special signal that indicates that a checkpoint
         * needs to be initiated */
        if (HYD_server_info.user_global.ckpoint_prefix == NULL) {
            HYDU_dump(stderr, "No checkpoint prefix provided\n");
            return;
        }

#if HAVE_ALARM
        if (HYD_ui_mpich_info.ckpoint_int != -1)
            alarm(HYD_ui_mpich_info.ckpoint_int);
#endif /* HAVE_ALARM */

        cmd.type = HYD_CKPOINT;
        HYDU_sock_write(HYD_server_info.cleanup_pipe[1], &cmd, sizeof(cmd), &sent, &closed);
    }
    else {
        /* All other signals are forwarded to the user */
        cmd.type = HYD_SIGNAL;
        cmd.signum = signum;
        HYDU_sock_write(HYD_server_info.cleanup_pipe[1], &cmd, sizeof(cmd), &sent, &closed);
    }

    HYDU_FUNC_EXIT();
    return;
}

#define ordered(n1, n2) \
    (((HYD_ui_mpich_info.sort_order == ASCENDING) && ((n1)->core_count <= (n2)->core_count)) || \
     ((HYD_ui_mpich_info.sort_order == DESCENDING) && ((n1)->core_count >= (n2)->core_count)))

static int compar(const void *_n1, const void *_n2)
{
    struct HYD_node *n1, *n2;
    int ret;

    n1 = *((struct HYD_node **) _n1);
    n2 = *((struct HYD_node **) _n2);

    if (n1->core_count == n2->core_count)
        ret = 0;
    else if (n1->core_count < n2->core_count)
        ret = -1;
    else
        ret = 1;

    if (HYD_ui_mpich_info.sort_order == DESCENDING)
        ret *= -1;

    return ret;
}

static HYD_status qsort_node_list(void)
{
    struct HYD_node **node_list, *node, *new_list, *r1;
    int count, i;
    HYD_status status = HYD_SUCCESS;

    for (count = 0, node = HYD_server_info.node_list; node; node = node->next, count++);

    HYDU_MALLOC(node_list, struct HYD_node **, count * sizeof(struct HYD_node *), status);
    for (i = 0, node = HYD_server_info.node_list; node; node = node->next, i++)
        node_list[i] = node;

    qsort((void *) node_list, (size_t) count, sizeof(struct HYD_node *), compar);

    r1 = new_list = node_list[0];
    for (i = 1; i < count; i++) {
        r1->next = node_list[i];
        r1 = r1->next;
        r1->next = NULL;
    }
    HYD_server_info.node_list = new_list;

    HYDU_FREE(node_list);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

int main(int argc, char **argv)
{
    struct HYD_proxy *proxy;
    struct HYD_exec *exec;
    struct HYD_node *node;
    int exit_status = 0, i, timeout, reset_rmk, global_core_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Initialize engines that don't require use to know anything
     * about the user preferences first */
    status = HYDU_dbg_init("mpiexec");
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

    status = HYDU_set_common_signals(signal_cb);
    HYDU_ERR_POP(status, "unable to set signal\n");

    if (pipe(HYD_server_info.cleanup_pipe) < 0)
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
    if (HYD_ui_mpich_info.ckpoint_int != -1)
        alarm(HYD_ui_mpich_info.ckpoint_int);
#endif /* HAVE_ALARM */

    status = HYDT_dmx_init(&HYD_server_info.user_global.demux);
    HYDU_ERR_POP(status, "unable to initialize the demux engine\n");

    status =
        HYDT_bsci_init(HYD_server_info.user_global.rmk, HYD_server_info.user_global.launcher,
                       HYD_server_info.user_global.launcher_exec,
                       HYD_server_info.user_global.enablex, HYD_server_info.user_global.debug);
    HYDU_ERR_POP(status, "unable to initialize the bootstrap server\n");

    reset_rmk = 0;

    if (HYD_server_info.node_list == NULL) {
        /* Node list is not created yet. The user might not have
         * provided the host file. Query the RMK. */
        status = HYDT_bsci_query_node_list(&HYD_server_info.node_list);
        HYDU_ERR_POP(status, "unable to query the RMK for a node list\n");

        if (HYD_server_info.node_list == NULL) {
            char localhost[MAX_HOSTNAME_LEN] = { 0 };

            /* The RMK didn't give us anything back; use localhost */
            status = HYDU_gethostname(localhost);
            HYDU_ERR_POP(status, "unable to get local hostname\n");

            status = HYDU_add_to_node_list(localhost, 1, &HYD_server_info.node_list);
            HYDU_ERR_POP(status, "unable to add to node list\n");

            reset_rmk = 1;
        }
    }

    if (HYD_server_info.user_global.debug)
        for (node = HYD_server_info.node_list; node; node = node->next)
            HYDU_dump_noprefix(stdout, "host: %s\n", node->hostname);

    /* Reset the host list to use only the number of processes per
     * node as specified by the ppn option. */
    if (HYD_ui_mpich_info.ppn != -1) {
        for (node = HYD_server_info.node_list; node; node = node->next)
            node->core_count = HYD_ui_mpich_info.ppn;
        reset_rmk = 1;
    }

    /* The RMK returned a node list. See if the user requested us to
     * manipulate it in some way */
    if (HYD_ui_mpich_info.sort_order != NONE) {
        qsort_node_list();
        reset_rmk = 1;
    }

    if (reset_rmk) {
        /* Reinitialize the bootstrap server with the "user" RMK, so
         * it knows that we are not using the node list provided by
         * the RMK */
        status = HYDT_bsci_finalize();
        HYDU_ERR_POP(status, "unable to finalize bootstrap device\n");

        status = HYDT_bsci_init("user", HYDT_bsci_info.launcher, HYDT_bsci_info.launcher_exec,
                                HYDT_bsci_info.enablex, HYDT_bsci_info.debug);
        HYDU_ERR_POP(status, "unable to reinitialize the bootstrap server\n");
    }

    /* Assign a node ID to each node */
    for (node = HYD_server_info.node_list, i = 0; node; node = node->next, i++)
        node->node_id = i;

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

    /* See if the node list contains a remotely accessible localhost */
    for (proxy = HYD_server_info.pg_list.proxy_list; proxy; proxy = proxy->next) {
        int is_local, remote_access;

        status = HYDU_sock_is_local(proxy->node->hostname, &is_local);
        HYDU_ERR_POP(status, "unable to check if %s is local\n", proxy->node->hostname);

        if (is_local) {
            status = HYDU_sock_remote_access(proxy->node->hostname, &remote_access);
            HYDU_ERR_POP(status, "unable to check if %s is remotely accessible\n",
                         proxy->node->hostname);

            if (remote_access)
                break;
        }
    }

    if (proxy)
        HYD_server_info.local_hostname = HYDU_strdup(proxy->node->hostname);

    if (HYD_server_info.user_global.debug)
        HYD_uiu_print_params();

    if (MPL_env2int("MPIEXEC_TIMEOUT", &timeout) == 0)
        timeout = -1;   /* Infinite timeout */

    if (HYD_server_info.user_global.debug)
        HYDU_dump(stdout, "Timeout set to %d (-1 means infinite)\n", timeout);

    /* Check if the user wants us to use a port within a certain
     * range. */
    if (MPL_env2str("MPIEXEC_PORTRANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIEXEC_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPICH_PORT_RANGE", (const char **) &HYD_server_info.port_range))
        HYD_server_info.port_range = HYDU_strdup(HYD_server_info.port_range);

    /* Add the stdout/stderr callback handlers */
    HYD_server_info.stdout_cb = HYD_uiu_stdout_cb;
    HYD_server_info.stderr_cb = HYD_uiu_stderr_cb;

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

    status = HYDT_ftb_finalize();
    HYDU_ERR_POP(status, "error finalizing FTB\n");

#if defined ENABLE_PROFILING
    if (HYD_server_info.enable_profiling) {
        HYDU_dump_noprefix(stdout, "\n");
        HYD_DRAW_LINE(80);
        HYDU_dump(stdout, "Number of PMI calls seen by the server: %d\n",
                  HYD_server_info.num_pmi_calls);
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
    else if (WIFSIGNALED(exit_status)) {
        printf("APPLICATION TERMINATED WITH THE EXIT STRING: %s (signal %d)\n",
               strsignal(WTERMSIG(exit_status)), WTERMSIG(exit_status));
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
