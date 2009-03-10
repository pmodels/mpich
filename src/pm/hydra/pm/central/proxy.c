/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_mem.h"
#include "hydra_sock.h"
#include "demux.h"
#include "proxy.h"

struct HYD_Proxy_params HYD_Proxy_params;
int HYD_Proxy_listenfd;

int main(int argc, char **argv)
{
    int i, j, arg, high_port, low_port, port, sockets_open;
    int HYD_Proxy_listenfd;
    char *port_range, *port_str, *str;
    HYD_Env_t *env, pmi;
    char *client_args[HYD_EXEC_ARGS];
    HYD_Status status = HYD_SUCCESS;

    status = HYD_Proxy_get_params(argc, argv);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("Bad parameters passed to the proxy\n");
        goto fn_fail;
    }

    /* Check if the user wants us to use a port within a certain
     * range. */
    port_range = getenv("MPIEXEC_PORTRANGE");
    if (!port_range)
        port_range = getenv("MPIEXEC_PORT_RANGE");
    if (!port_range)
        port_range = getenv("MPICH_PORT_RANGE");

    /* Listen on a port in the port range */
    status = HYDU_Sock_listen(&HYD_Proxy_listenfd, port_range, &port);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("sock utils returned listen error\n");
        goto fn_fail;
    }

    /* Register the listening socket with the demux engine */
    status = HYD_DMX_Register_fd(1, &HYD_Proxy_listenfd, HYD_STDOUT, HYD_Proxy_listen_cb);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("demux engine returned error for registering fd\n");
        goto fn_fail;
    }

    /* FIXME: We do not use the bootstrap server right now, as the
     * current implementation of the bootstrap launch directly reads
     * the executable information from the HYD_Handle structure. Since
     * we are a different process, we don't share this
     * structure. Without the bootstrap server, we can only launch
     * local processes. That is, we can only have a single-level
     * hierarchy of proxies. */

    HYDU_MALLOC(HYD_Proxy_params.out, int *,
                HYD_Proxy_params.proc_count * sizeof(int), status);
    HYDU_MALLOC(HYD_Proxy_params.err, int *,
                HYD_Proxy_params.proc_count * sizeof(int), status);
    HYDU_MALLOC(HYD_Proxy_params.pid, int *,
                HYD_Proxy_params.proc_count * sizeof(int), status);

    /* Spawn the processes */
    for (i = 0; i < HYD_Proxy_params.proc_count; i++) {

        pmi.env_name = MPIU_Strdup("PMI_ID");
        HYDU_Int_to_str(HYD_Proxy_params.pmi_id + i, str, status);
        pmi.env_value = MPIU_Strdup(str);
        pmi.env_type = HYD_ENV_STATIC;
        pmi.start_val = 0;
        pmi.next = NULL;

        /* Update the PMI_ID value with this one */
        status = HYDU_Env_add_to_list(&HYD_Proxy_params.env_list, pmi);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("unable to add env to list\n");
            goto fn_fail;
        }

        /* Set the environment with the current values */
        for (env = HYD_Proxy_params.env_list; env; env = env->next) {
            status = HYDU_Env_putenv(*env);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("unable to putenv\n");
                goto fn_fail;
            }
        }

        for (j = 0, arg = 0; HYD_Proxy_params.args[j]; j++)
            client_args[arg++] = MPIU_Strdup(HYD_Proxy_params.args[j]);
        client_args[arg++] = NULL;

        if ((i & HYD_Proxy_params.pmi_id) == 0) {
            status = HYDU_Create_process(client_args, &HYD_Proxy_params.in,
                                         &HYD_Proxy_params.out[i], &HYD_Proxy_params.err[i],
                                         &HYD_Proxy_params.pid[i]);
        }
        else {
            status = HYDU_Create_process(client_args, NULL,
                                         &HYD_Proxy_params.out[i],
                                         &HYD_Proxy_params.err[i], &HYD_Proxy_params.pid[i]);
        }
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("spawn process returned error\n");
            goto fn_fail;
        }
    }

    /* Everything is spawned, now wait for I/O */
    status = HYD_DMX_Register_fd(HYD_Proxy_params.proc_count, HYD_Proxy_params.out,
                                 HYD_STDOUT, HYD_Proxy_stdout_cb);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("demux engine returned error when registering fd\n");
        goto fn_fail;
    }
    status = HYD_DMX_Register_fd(HYD_Proxy_params.proc_count, HYD_Proxy_params.err,
                                 HYD_STDOUT, HYD_Proxy_stderr_cb);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("demux engine returned error when registering fd\n");
        goto fn_fail;
    }

    /* FIXME: Handle stdin */
    HYD_Proxy_params.stdin_buf_offset = 0;
    HYD_Proxy_params.stdin_buf_count = 0;

    while (1) {
        /* Wait for some event to occur */
        status = HYD_DMX_Wait_for_event();
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("demux engine returned error when waiting for event\n");
            goto fn_fail;
        }

        /* Check to see if there's any open read socket left; if there
         * are, we will just wait for more events. */
        sockets_open = 0;
        for (i = 0; i < HYD_Proxy_params.proc_count; i++) {
            if (HYD_Proxy_params.out[i] != -1 || HYD_Proxy_params.err[i] != -1) {
                sockets_open++;
                break;
            }
        }

        /* We are done */
        if (!sockets_open)
            break;
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
