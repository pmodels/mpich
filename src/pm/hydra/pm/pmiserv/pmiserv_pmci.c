/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmci.h"
#include "pmiserv_pmi.h"
#include "bsci.h"
#include "pmiserv.h"
#include "pmiserv_utils.h"

int HYD_pmcd_pmiserv_abort = 0;
int HYD_pmcd_pmiserv_pipe[2];

HYD_status HYD_pmci_launch_procs(void)
{
    struct HYD_proxy *proxy;
    struct HYD_node *node_list = NULL, *node, *tnode;
    char *proxy_args[HYD_NUM_TMP_STRINGS] = { NULL }, *control_port = NULL;
    char *pmi_port = NULL, *mapping;
    int pmi_id = -1, enable_stdin, ret;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_set_common_signals(HYD_pmcd_pmiserv_signal_cb);
    HYDU_ERR_POP(status, "unable to set signal\n");

    /* Initialize PMI */
    ret = MPL_env2str("PMI_PORT", (const char **) &pmi_port);
    if (ret) {  /* PMI_PORT already set */
        if (HYD_handle.user_global.debug)
            HYDU_dump(stdout, "someone else already set PMI port\n");
        pmi_port = HYDU_strdup(pmi_port);

        ret = MPL_env2int("PMI_ID", &pmi_id);
        if (!ret)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PMI_PORT set but not PMI_ID\n");
    }
    else {
        pmi_id = -1;
    }
    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "PMI port: %s; PMI ID: %d\n", pmi_port, pmi_id);

    status = HYD_pmcd_pmi_alloc_pg_scratch(&HYD_handle.pg_list);
    HYDU_ERR_POP(status, "error allocating pg scratch space\n");

    status = HYD_pmcd_pmi_process_mapping(&mapping);
    HYDU_ERR_POP(status, "Unable to get process mapping information\n");

    /* Make sure the mapping is within the size allowed by PMI */
    if (strlen(mapping) <= MAXVALLEN) {
        pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) HYD_handle.pg_list.pg_scratch;
        status = HYD_pmcd_pmi_add_kvs("PMI_process_mapping", mapping, pg_scratch->kvs, &ret);
        HYDU_ERR_POP(status, "unable to add process_mapping to KVS\n");
    }
    HYDU_FREE(mapping);

    /* Copy the host list to pass to the bootstrap server */
    node_list = NULL;
    for (proxy = HYD_handle.pg_list.proxy_list; proxy; proxy = proxy->next) {
        HYDU_alloc_node(&node);
        node->hostname = HYDU_strdup(proxy->node.hostname);
        node->core_count = proxy->node.core_count;
        node->next = NULL;

        if (node_list == NULL) {
            node_list = node;
        }
        else {
            for (tnode = node_list; tnode->next; tnode = tnode->next);
            tnode->next = node;
        }
    }

    status = HYDU_sock_create_and_listen_portstr(HYD_handle.user_global.iface,
                                                 HYD_handle.port_range, &control_port,
                                                 HYD_pmcd_pmiserv_control_listen_cb,
                                                 (void *) (size_t) 0);
    HYDU_ERR_POP(status, "unable to create PMI port\n");
    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "Got a control port string of %s\n", control_port);

    status = HYD_pmcd_pmi_fill_in_proxy_args(proxy_args, control_port, 0);
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");

    status = HYD_pmcd_pmi_fill_in_exec_launch_info(pmi_port, pmi_id, &HYD_handle.pg_list);
    HYDU_ERR_POP(status, "unable to fill in executable arguments\n");

    status = HYDT_dmx_stdin_valid(&enable_stdin);
    HYDU_ERR_POP(status, "unable to check if stdin is valid\n");

    status = HYDT_bsci_launch_procs(proxy_args, node_list, enable_stdin, HYD_handle.stdout_cb,
                                    HYD_handle.stderr_cb);
    HYDU_ERR_POP(status, "bootstrap server cannot launch processes\n");

  fn_exit:
    if (pmi_port)
        HYDU_FREE(pmi_port);
    if (control_port)
        HYDU_FREE(control_port);
    HYDU_free_strlist(proxy_args);
    HYDU_free_node_list(node_list);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static int user_abort_signal = 0;

static HYD_status cleanup_procs(int fd, HYD_event_t events, void *userp)
{
    char c;
    int count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_read(fd, &c, 1, &count, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "read error\n");

    /* This is an abort signal received from the user. This is
     * different from a timeout. In a timeout, we wait for the proxies
     * to connect back. If the user explicitly killed the job, then we
     * don't wait for this. */
    if (user_abort_signal == 0) {
        HYDU_dump_noprefix(stdout, "Ctrl-C caught: waiting for a clean abort\n");
        HYDU_dump_noprefix(stdout, "[press Ctrl-C again to force abort]\n");
        user_abort_signal = 1;
    }
    else
        HYD_pmcd_pmiserv_abort = 1;

    status = HYD_pmcd_pmiserv_cleanup();
    HYDU_ERR_POP(status, "cleanup of processes failed\n");

    /* Once the cleanup signal has been sent, wait for the proxies to
     * get back with the exit status */
    status = HYDT_bsci_wait_for_completion(-1);
    HYDU_ERR_POP(status, "bootstrap server returned error waiting for completion\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmci_wait_for_completion(int timeout)
{
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    int not_complete;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (pipe(HYD_pmcd_pmiserv_pipe) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "pipe error\n");

    status = HYDT_dmx_register_fd(1, &HYD_pmcd_pmiserv_pipe[0], POLLIN, NULL, cleanup_procs);
    HYDU_ERR_POP(status, "unable to register fd\n");

    status = HYDT_bsci_wait_for_completion(timeout);
    if (status == HYD_TIMED_OUT) {
        status = HYD_pmcd_pmiserv_cleanup();
        HYDU_ERR_POP(status, "cleanup of processes failed\n");
    }
    HYDU_ERR_POP(status, "bootstrap server returned error waiting for completion\n");

    /* Once the cleanup signal has been sent, wait for the proxies to
     * get back with the exit status */
    status = HYDT_bsci_wait_for_completion(-1);
    HYDU_ERR_POP(status, "bootstrap server returned error waiting for completion\n");

    /* Make sure all the proxies have sent their exit status'es */
    not_complete = 1;
    while (not_complete) {
        not_complete = 0;
        for (pg = &HYD_handle.pg_list; pg; pg = pg->next) {
            for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
                if (proxy->exit_status == NULL) {
                    not_complete = 1;
                    break;
                }
            }
            if (not_complete)
                break;
        }
        if (not_complete) {
            status = HYDT_dmx_wait_for_event(-1);
            HYDU_ERR_POP(status, "error waiting for demux event\n");
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmci_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_finalize();
    HYDU_ERR_POP(status, "unable to finalize process manager utils\n");

    status = HYDT_bsci_finalize();
    HYDU_ERR_POP(status, "unable to finalize bootstrap server\n");

    status = HYDT_dmx_finalize();
    HYDU_ERR_POP(status, "error returned from demux finalize\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
