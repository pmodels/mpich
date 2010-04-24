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

static HYD_status cleanup_procs(void)
{
    static int user_abort_signal = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();


    /* Sent kill signals to the processes. Wait for all processes to
     * exit. If they do not exit, allow the application to just force
     * kill the spawned processes. */
    if (user_abort_signal == 0) {
        HYDU_dump_noprefix(stdout, "Ctrl-C caught... cleaning up processes\n");

        status = HYD_pmcd_pmiserv_cleanup();
        HYDU_ERR_POP(status, "cleanup of processes failed\n");

        HYDU_dump_noprefix(stdout, "[press Ctrl-C again to force abort]\n");

        user_abort_signal = 1;
    }
    else {
        HYDU_dump_noprefix(stdout, "Ctrl-C caught... forcing cleanup\n");

        status = HYDT_bsci_cleanup_procs();
        HYDU_ERR_POP(status, "error cleaning up processes\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status ckpoint(void)
{
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    enum HYD_pmcd_pmi_cmd cmd;
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* FIXME: Instead of doing this from this process itself, fork a
     * bunch of processes to do this. */
    /* Connect to all proxies and send the checkpoint command */
    for (pg = &HYD_handle.pg_list; pg; pg = pg->next) {
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
            cmd = CKPOINT;
            status = HYDU_sock_write(proxy->control_fd, &cmd, sizeof(enum HYD_pmcd_pmi_cmd),
                                     &sent, &closed);
            HYDU_ERR_POP(status, "unable to send checkpoint message\n");
            HYDU_ASSERT(!closed, status);
        }
    }

    HYDU_FUNC_EXIT();

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status ui_cmd_cb(int fd, HYD_event_t events, void *userp)
{
    enum HYD_cmd cmd;
    int count, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_read(fd, &cmd, sizeof(cmd), &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "read error\n");
    HYDU_ASSERT(!closed, status);

    if (cmd == HYD_CLEANUP) {
        status = cleanup_procs();
        HYDU_ERR_POP(status, "error cleaning up processes\n");
    }
    else if (cmd == HYD_CKPOINT) {
        status = ckpoint();
        HYDU_ERR_POP(status, "error checkpointing processes\n");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status stdout_cb(void *buf, int buflen)
{
    struct HYD_pmcd_stdio_hdr *hdr;
    static char *storage = NULL;
    static int storage_len = 0;
    char *rbuf, *tmp, str[HYD_TMPBUF_SIZE];
    int rlen;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    rbuf = buf;
    rlen = buflen;

    while (1) {
        hdr = (struct HYD_pmcd_stdio_hdr *) rbuf;

        if (rlen < sizeof(struct HYD_pmcd_stdio_hdr) + hdr->buflen) {
            /* We don't have the entire data; store it and wait for
             * the next event */
            HYDU_MALLOC(tmp, char *, rlen + storage_len, status);
            if (storage_len)
                memcpy(tmp, storage, storage_len);
            memcpy(tmp + storage_len, buf, rlen);
            if (storage)
                HYDU_FREE(storage);
            storage = tmp;

            break;
        }
        else {
            rbuf += sizeof(struct HYD_pmcd_stdio_hdr);
            rlen -= sizeof(struct HYD_pmcd_stdio_hdr);

            if (HYD_handle.prepend_rank) {
                HYDU_snprintf(str, HYD_TMPBUF_SIZE, "[%d] ", hdr->rank);
                status = HYD_handle.stdout_cb(str, strlen(str));
                HYDU_ERR_POP(status, "error in the UI defined stdout callback\n");
            }

            status = HYD_handle.stdout_cb(rbuf, hdr->buflen);
            HYDU_ERR_POP(status, "error in the UI defined stdout callback\n");

            rbuf += hdr->buflen;
            rlen -= hdr->buflen;
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status stderr_cb(void *buf, int buflen)
{
    struct HYD_pmcd_stdio_hdr *hdr;
    static char *storage = NULL;
    static int storage_len = 0;
    char *rbuf, *tmp, str[HYD_TMPBUF_SIZE];
    int rlen;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    rbuf = buf;
    rlen = buflen;

    while (1) {
        hdr = (struct HYD_pmcd_stdio_hdr *) rbuf;

        if (rlen < sizeof(struct HYD_pmcd_stdio_hdr) + hdr->buflen) {
            /* We don't have the entire data; store it and wait for
             * the next event */
            HYDU_MALLOC(tmp, char *, rlen + storage_len, status);
            if (storage_len)
                memcpy(tmp, storage, storage_len);
            memcpy(tmp + storage_len, buf, rlen);
            if (storage)
                HYDU_FREE(storage);
            storage = tmp;

            break;
        }
        else {
            rbuf += sizeof(struct HYD_pmcd_stdio_hdr);
            rlen -= sizeof(struct HYD_pmcd_stdio_hdr);

            if (HYD_handle.prepend_rank) {
                HYDU_snprintf(str, HYD_TMPBUF_SIZE, "[%d] ", hdr->rank);
                status = HYD_handle.stdout_cb(str, strlen(str));
                HYDU_ERR_POP(status, "error in the UI defined stdout callback\n");
            }

            status = HYD_handle.stderr_cb(rbuf, hdr->buflen);
            HYDU_ERR_POP(status, "error in the UI defined stderr callback\n");

            rbuf += hdr->buflen;
            rlen -= hdr->buflen;
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmci_launch_procs(void)
{
    struct HYD_proxy *proxy;
    struct HYD_node *node_list = NULL, *node, *tnode;
    char *proxy_args[HYD_NUM_TMP_STRINGS] = { NULL }, *control_port = NULL;
    char *pmi_fd = NULL;
    int pmi_rank = -1, enable_stdin, ret;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_dmx_register_fd(1, &HYD_handle.cleanup_pipe[0], POLLIN, NULL, ui_cmd_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    /* Initialize PMI */
    ret = MPL_env2str("PMI_FD", (const char **) &pmi_fd);
    if (ret) {  /* PMI_FD already set */
        if (HYD_handle.user_global.debug)
            HYDU_dump(stdout, "someone else already set PMI FD\n");
        pmi_fd = HYDU_strdup(pmi_fd);

        ret = MPL_env2int("PMI_RANK", &pmi_rank);
        if (!ret)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PMI_FD set but not PMI_RANK\n");
    }
    else {
        pmi_rank = -1;
    }
    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "PMI FD: %s; PMI ID: %d\n", pmi_fd, pmi_rank);

    status = HYD_pmcd_pmi_alloc_pg_scratch(&HYD_handle.pg_list);
    HYDU_ERR_POP(status, "error allocating pg scratch space\n");

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

    status = HYD_pmcd_pmi_fill_in_exec_launch_info(pmi_fd, pmi_rank, &HYD_handle.pg_list);
    HYDU_ERR_POP(status, "unable to fill in executable arguments\n");

    status = HYDT_dmx_stdin_valid(&enable_stdin);
    HYDU_ERR_POP(status, "unable to check if stdin is valid\n");

    status = HYDT_bsci_launch_procs(proxy_args, node_list, enable_stdin, stdout_cb,
                                    stderr_cb);
    HYDU_ERR_POP(status, "bootstrap server cannot launch processes\n");

  fn_exit:
    if (pmi_fd)
        HYDU_FREE(pmi_fd);
    if (control_port)
        HYDU_FREE(control_port);
    HYDU_free_strlist(proxy_args);
    HYDU_free_node_list(node_list);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmci_wait_for_completion(int timeout)
{
    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_bsci_wait_for_completion(timeout);
    if (status == HYD_TIMED_OUT) {
        status = HYD_pmcd_pmiserv_cleanup();
        HYDU_ERR_POP(status, "cleanup of processes failed\n");
    }
    HYDU_ERR_POP(status, "bootstrap server returned error waiting for completion\n");

    /* Wait for the processes to terminate */
    status = HYDT_bsci_wait_for_completion(-1);
    HYDU_ERR_POP(status, "bootstrap server returned error waiting for completion\n");

    /* If we didn't get a user abort signal yet, wait for the exit
     * status'es */
    for (pg = &HYD_handle.pg_list; pg; pg = pg->next) {
        pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

        while (pg_scratch->control_listen_fd != -1) {
            status = HYDT_dmx_wait_for_event(-1);
            HYDU_ERR_POP(status, "error waiting for event\n");
        }

        if (pg_scratch) {
            HYD_pmcd_free_pmi_kvs_list(pg_scratch->kvs);
            HYDU_FREE(pg_scratch);
            pg_scratch = NULL;
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
