/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "rmki.h"
#include "pmci.h"
#include "uiu.h"
#include "demux.h"

struct HYD_handle HYD_handle = { {0} };

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
    printf("    -wdir {dirname}                  working directory to use\n");

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
    printf("  Bootstrap options:\n");
    printf("    -bootstrap                       bootstrap server to use\n");
    printf("    -bootstrap-exec                  executable to use to bootstrap processes\n");
    printf("    -enable-x/-disable-x             enable or disable X forwarding\n");

    printf("\n");
    printf("  Proxy options (only needed for persistent mode):\n");
    printf("    -boot-proxies                    boot proxies to run in persistent mode\n");
    printf("    -boot-foreground-proxies         boot foreground proxies (persistent mode)\n");
    printf("    -shutdown-proxies                shutdown persistent mode proxies\n");
    printf("    -proxy-port                      port for proxies to listen (boot proxies)\n");
    printf("    -use-persistent                  use persistent mode proxies to launch\n");

    printf("\n");
    printf("  Communication sub-system options:\n");
    printf("    -css                             communication sub-system to use\n");

    printf("\n");
    printf("  Resource management kernel options:\n");
    printf("    -rmk                             resource management kernel to use\n");

    printf("\n");
    printf("  Hybrid programming options:\n");
    printf("    -ranks-per-proc                  assign so many ranks to each process\n");
    printf("    -enable/-disable-pm-env          process manager environment settings\n");

    printf("\n");
    printf("  Process-core binding options:\n");
    printf("    -binding                         process-to-core binding mode\n");
    printf("    -bindlib                         process-to-core binding library (plpa)\n");

    printf("\n");
    printf("  Checkpoint/Restart options:\n");
    printf("    -ckpoint-interval                checkpoint interval\n");
    printf("    -ckpoint-prefix                  checkpoint file prefix\n");
    printf("    -ckpointlib                      checkpointing library (blcr)\n");
    printf("    -ckpoint-restart                 restart a checkpointed application\n");

    printf("\n");
    printf("  Other Hydra options:\n");
    printf("    -verbose                         verbose mode\n");
    printf("    -info                            build information\n");
    printf("    -print-rank-map                  print rank mapping\n");
    printf("    -print-all-exitcodes             print exit codes of all processes\n");
}


int main(int argc, char **argv)
{
    struct HYD_proxy *proxy;
    struct HYD_proxy_exec *exec;
    struct HYD_exec_info *exec_info;
    int exit_status = 0, timeout, i, process_id, proc_count, num_cores;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_dbg_init("mpiexec");
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

    status = HYD_uii_mpx_get_parameters(argv);
    if (status == HYD_GRACEFUL_ABORT) {
        exit(0);
    }
    else if (status != HYD_SUCCESS) {
        usage();
        goto fn_fail;
    }

    status = HYD_rmki_init(HYD_handle.rmk);
    HYDU_ERR_POP(status, "unable to initialize RMK\n");

    if (HYD_handle.proxy_list == NULL) {
        /* Proxy list is not created yet. The user might not have
         * provided the host file. Query the RMK. We pass a zero core
         * count, so the RMK will give us all the nodes it already has
         * and won't try to allocate any more. */
        num_cores = 0;
        status = HYD_rmki_query_node_list(&num_cores, &HYD_handle.proxy_list);
        HYDU_ERR_POP(status, "unable to query the RMK for a node list\n");

        /* We don't have an allocation capability yet, but when we do,
         * we should try it here. */

        if (HYD_handle.proxy_list == NULL) {
            /* The RMK didn't give us anything back; use localhost */
            status = HYDU_add_to_proxy_list((char *) "localhost", 1, &HYD_handle.proxy_list);
            HYDU_ERR_POP(status, "unable to initialize proxy\n");
            HYD_handle.global_core_count += 1;
        }
        else {
            /* The RMK returned a node list */
            HYD_handle.global_core_count += num_cores;
        }
    }

    /* If the number of processes is not given, we allocate all the
     * available nodes to each executable */
    for (exec_info = HYD_handle.exec_info_list; exec_info; exec_info = exec_info->next) {
        if (exec_info->process_count == 0) {
            if (num_cores == 0)
                exec_info->process_count = 1;
            else
                exec_info->process_count = num_cores;

            /* If we didn't get anything from the user, take whatever
             * the RMK gave */
            HYD_handle.global_process_count += exec_info->process_count;
        }
    }

    /* Figure out what the active proxys are: in RUNTIME and
     * PERSISTENT modes, only proxys which have an executable are
     * active. In BOOT, BOOT_FOREGROUND and SHUTDOWN modes, all
     * proxys are active. */
    if (HYD_handle.user_global.launch_mode == HYD_LAUNCH_RUNTIME ||
        HYD_handle.user_global.launch_mode == HYD_LAUNCH_PERSISTENT) {
        for (proxy = HYD_handle.proxy_list;
             proxy && (proxy->start_pid <= HYD_handle.global_process_count);
             proxy = proxy->next)
            proxy->active = 1;
    }
    else {
        for (proxy = HYD_handle.proxy_list; proxy; proxy = proxy->next)
            proxy->active = 1;
    }

    status = HYD_uiu_merge_exec_info_to_proxy();
    HYDU_ERR_POP(status, "unable to merge exec info\n");

    if (HYD_handle.user_global.debug)
        HYD_uiu_print_params();

    HYDU_time_set(&HYD_handle.start, NULL);     /* NULL implies right now */
    if (getenv("MPIEXEC_TIMEOUT"))
        timeout = atoi(getenv("MPIEXEC_TIMEOUT"));
    else
        timeout = -1;   /* Set a negative timeout */
    HYDU_time_set(&HYD_handle.timeout, &timeout);
    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "Timeout set to %d (-1 means infinite)\n", timeout);

    if (HYD_handle.print_rank_map) {
        FORALL_ACTIVE_PROXIES(proxy, HYD_handle.proxy_list) {
            HYDU_dump_noprefix(stdout, "(%s:", proxy->info.hostname);

            process_id = 0;
            for (exec = proxy->exec_list; exec; exec = exec->next) {
                for (i = 0; i < exec->proc_count; i++) {
                    HYDU_dump_noprefix(stdout, "%d",
                                       HYDU_local_to_global_id(process_id++,
                                                               proxy->start_pid,
                                                               proxy->info.core_count,
                                                               HYD_handle.global_core_count));
                    if (i < exec->proc_count - 1 || exec->next)
                        HYDU_dump_noprefix(stdout, ",");
                }
            }
            HYDU_dump_noprefix(stdout, ")\n");
        }
    }

    FORALL_ACTIVE_PROXIES(proxy, HYD_handle.proxy_list)
    HYDU_MALLOC(proxy->exit_status, int *, proxy->proxy_process_count * sizeof(int), status);

    /* Launch the processes */
    status = HYD_pmci_launch_procs();
    HYDU_ERR_POP(status, "process manager returned error launching processes\n");

    /* During shutdown, no processes are launched, so there is nothing
     * to wait for. If the launch command didn't return an error, we
     * are OK; just return a success. */
    /* FIXME: We are assuming a working model for the process manager
     * here. We need to get how many processes the PM has launched
     * instead of assuming this. For example, it is possible to have a
     * PM implementation that launches separate "new" proxies on a
     * different port and kills the original proxies using them. */
    if (HYD_handle.user_global.launch_mode == HYD_LAUNCH_SHUTDOWN) {
        /* Call finalize functions for lower layers to cleanup their resources */
        status = HYD_pmci_finalize();
        HYDU_ERR_POP(status, "process manager error on finalize\n");

        /* Free the mpiexec params */
        HYD_uiu_free_params();

        exit_status = 0;
        goto fn_exit;
    }

    /* Setup stdout/stderr/stdin handlers */
    status = HYDU_sock_set_nonblock(0);
    HYDU_ERR_POP(status, "unable to set socket as non-blocking\n");

    HYD_handle.stdin_buf_count = 0;
    HYD_handle.stdin_buf_offset = 0;

    FORALL_ACTIVE_PROXIES(proxy, HYD_handle.proxy_list) {
        if (proxy->out != -1) {
            status = HYDT_dmx_register_fd(1, &proxy->out, HYD_STDOUT, NULL,
                                          HYD_uii_mpx_stdout_cb);
            HYDU_ERR_POP(status, "demux returned error registering fd\n");
        }

        if (proxy->err != -1) {
            status = HYDT_dmx_register_fd(1, &proxy->err, HYD_STDOUT, NULL,
                                          HYD_uii_mpx_stderr_cb);
            HYDU_ERR_POP(status, "demux returned error registering fd\n");
        }

        if (proxy->in != -1) {
            status =
                HYDT_dmx_register_fd(1, &proxy->in, HYD_STDIN, NULL, HYD_uii_mpx_stdin_cb);
            HYDU_ERR_POP(status, "demux returned error registering fd\n");
        }
    }

    /* Wait for their completion */
    status = HYD_pmci_wait_for_completion();
    HYDU_ERR_POP(status, "process manager error waiting for completion\n");

    /* Check for the exit status for all the processes */
    if (HYD_handle.print_all_exitcodes)
        HYDU_dump(stdout, "Exit codes: ");
    exit_status = 0;
    FORALL_ACTIVE_PROXIES(proxy, HYD_handle.proxy_list) {
        proc_count = 0;
        for (exec = proxy->exec_list; exec; exec = exec->next)
            proc_count += exec->proc_count;
        for (i = 0; i < proc_count; i++) {
            if (HYD_handle.print_all_exitcodes) {
                HYDU_dump_noprefix(stdout, "[%d]",
                                   HYDU_local_to_global_id(i, proxy->start_pid,
                                                           proxy->info.core_count,
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

    /* Free the mpiexec params */
    HYD_uiu_free_params();

  fn_exit:
    HYDU_FUNC_EXIT();
    if (status == HYD_GRACEFUL_ABORT)
        return 0;
    else if (status != HYD_SUCCESS)
        return -1;
    else {
        if (WIFSIGNALED(exit_status))
            printf("%s (signal %d)\n", strsignal(WTERMSIG(exit_status)),
                   WTERMSIG(exit_status));
        else if (WIFEXITED(exit_status))
            return (WEXITSTATUS(exit_status));
        else if (WIFSTOPPED(exit_status))
            return (WSTOPSIG(exit_status));
        return exit_status;
    }

  fn_fail:
    goto fn_exit;
}
