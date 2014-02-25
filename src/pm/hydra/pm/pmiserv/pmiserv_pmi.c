/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_server.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_handle = { 0 };

struct HYD_pmcd_pmi_publish *HYD_pmcd_pmi_publish_list = NULL;

struct HYD_proxy *HYD_pmcd_pmi_find_proxy(int fd)
{
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;

    for (pg = &HYD_server_info.pg_list; pg; pg = pg->next)
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next)
            if (proxy->control_fd == fd)
                return proxy;

    return NULL;
}

HYD_status HYD_pmcd_pmi_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYD_pmcd_pmi_free_publish(struct HYD_pmcd_pmi_publish * publish)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_FREE(publish->name);
    HYDU_FREE(publish->port);

    for (i = 0; i < publish->infokeycount; i++) {
        HYDU_FREE(publish->info_keys[i].key);
        HYDU_FREE(publish->info_keys[i].val);
    }
    if (publish->info_keys)
        HYDU_FREE(publish->info_keys);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_publish(char *name, char *port, int *success)
{
    struct HYD_pmcd_pmi_publish *r, *publish;
    struct HYD_string_stash stash;
    char *ns, *ns_host, *ns_port_str;
    int ns_port, ns_fd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_server_info.nameserver == NULL) {
        /* no external nameserver available */
        for (r = HYD_pmcd_pmi_publish_list; r; r = r->next)
            if (!strcmp(r->name, name))
                break;
        if (r) {
            *success = 0;
            goto fn_exit;
        }
        *success = 1;

        HYDU_MALLOC(publish, struct HYD_pmcd_pmi_publish *,
                    sizeof(struct HYD_pmcd_pmi_publish), status);
        publish->name = HYDU_strdup(name);
        publish->port = HYDU_strdup(port);
        publish->infokeycount = 0;
        publish->info_keys = NULL;
        publish->next = NULL;

        if (HYD_pmcd_pmi_publish_list == NULL)
            HYD_pmcd_pmi_publish_list = publish;
        else {
            for (r = HYD_pmcd_pmi_publish_list; r->next; r = r->next);
            r->next = publish;
        }
    }
    else {
        int len, recvd, closed;
        char *resp;

        /* connect to the external nameserver and store the
         * information there */
        ns = HYDU_strdup(HYD_server_info.nameserver);

        ns_host = strtok(ns, ":");
        HYDU_ASSERT(ns_host, status);

        ns_port_str = strtok(NULL, ":");
        if (ns_port_str)
            ns_port = atoi(ns_port_str);
        else
            ns_port = HYDRA_NAMESERVER_DEFAULT_PORT;

        status = HYDU_sock_connect(ns_host, (uint16_t) ns_port, &ns_fd, 0, HYD_CONNECT_DELAY);
        HYDU_ERR_POP(status, "error connecting to the nameserver\n");

        HYD_STRING_STASH_INIT(stash);
        HYD_STRING_STASH(stash, HYDU_strdup("PUBLISH"), status);
        HYD_STRING_STASH(stash, HYDU_strdup(name), status);
        HYD_STRING_STASH(stash, HYDU_strdup(port), status);

        status = HYDU_send_strlist(ns_fd, stash.strlist);
        HYDU_ERR_POP(status, "error sending string list\n");
        HYD_STRING_STASH_FREE(stash);

        status = HYDU_sock_read(ns_fd, &len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading from nameserver\n");
        HYDU_ASSERT(!closed, status);

        HYDU_MALLOC(resp, char *, len, status);
        status = HYDU_sock_read(ns_fd, resp, len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading from nameserver\n");
        HYDU_ASSERT(len == recvd, status);

        close(ns_fd);

        if (!strcmp(resp, "SUCCESS"))
            *success = 1;
        else
            *success = 0;

        HYDU_FREE(resp);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_unpublish(char *name, int *success)
{
    struct HYD_pmcd_pmi_publish *r, *publish;
    struct HYD_string_stash stash;
    char *ns, *ns_host, *ns_port_str;
    int ns_port, ns_fd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *success = 0;
    if (HYD_server_info.nameserver == NULL) {
        /* no external nameserver available */

        if (!HYD_pmcd_pmi_publish_list) {
            /* nothing is published yet */
            goto fn_exit;
        }

        if (!strcmp(HYD_pmcd_pmi_publish_list->name, name)) {
            publish = HYD_pmcd_pmi_publish_list;
            HYD_pmcd_pmi_publish_list = HYD_pmcd_pmi_publish_list->next;
            publish->next = NULL;

            HYD_pmcd_pmi_free_publish(publish);
            HYDU_FREE(publish);
            *success = 1;
        }
        else {
            publish = HYD_pmcd_pmi_publish_list;
            do {
                if (publish->next == NULL)
                    break;
                else if (!strcmp(publish->next->name, name)) {
                    r = publish->next;
                    publish->next = r->next;
                    r->next = NULL;

                    HYD_pmcd_pmi_free_publish(r);
                    HYDU_FREE(r);
                    *success = 1;
                }
                else
                    publish = publish->next;
            } while (1);
        }
    }
    else {
        int len, recvd, closed;
        char *resp;

        /* connect to the external nameserver and get the information
         * from there */
        ns = HYDU_strdup(HYD_server_info.nameserver);

        ns_host = strtok(ns, ":");
        HYDU_ASSERT(ns_host, status);

        ns_port_str = strtok(NULL, ":");
        if (ns_port_str)
            ns_port = atoi(ns_port_str);
        else
            ns_port = HYDRA_NAMESERVER_DEFAULT_PORT;

        status = HYDU_sock_connect(ns_host, (uint16_t) ns_port, &ns_fd, 0, HYD_CONNECT_DELAY);
        HYDU_ERR_POP(status, "error connecting to the nameserver\n");

        HYD_STRING_STASH_INIT(stash);
        HYD_STRING_STASH(stash, HYDU_strdup("UNPUBLISH"), status);
        HYD_STRING_STASH(stash, HYDU_strdup(name), status);

        status = HYDU_send_strlist(ns_fd, stash.strlist);
        HYDU_ERR_POP(status, "error sending string list\n");
        HYD_STRING_STASH_FREE(stash);

        status = HYDU_sock_read(ns_fd, &len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading from nameserver\n");
        HYDU_ASSERT(!closed, status);

        HYDU_MALLOC(resp, char *, len, status);
        status = HYDU_sock_read(ns_fd, resp, len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading from nameserver\n");
        HYDU_ASSERT(len == recvd, status);

        close(ns_fd);

        if (!strcmp(resp, "SUCCESS"))
            *success = 1;
        HYDU_FREE(resp);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_lookup(char *name, char **value)
{
    struct HYD_pmcd_pmi_publish *publish;
    struct HYD_string_stash stash;
    char *ns, *ns_host, *ns_port_str;
    int ns_port, ns_fd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *value = NULL;

    if (HYD_server_info.nameserver == NULL) {
        /* no external nameserver available */
        for (publish = HYD_pmcd_pmi_publish_list; publish; publish = publish->next)
            if (!strcmp(publish->name, name))
                break;

        if (publish)
            *value = HYDU_strdup(publish->port);
    }
    else {
        int len, recvd, closed;
        char *resp = NULL;

        /* connect to the external nameserver and get the information
         * from there */
        ns = HYDU_strdup(HYD_server_info.nameserver);

        ns_host = strtok(ns, ":");
        HYDU_ASSERT(ns_host, status);

        ns_port_str = strtok(NULL, ":");
        if (ns_port_str)
            ns_port = atoi(ns_port_str);
        else
            ns_port = HYDRA_NAMESERVER_DEFAULT_PORT;

        status = HYDU_sock_connect(ns_host, (uint16_t) ns_port, &ns_fd, 0, HYD_CONNECT_DELAY);
        HYDU_ERR_POP(status, "error connecting to the nameserver\n");

        HYD_STRING_STASH_INIT(stash);
        HYD_STRING_STASH(stash, HYDU_strdup("LOOKUP"), status);
        HYD_STRING_STASH(stash, HYDU_strdup(name), status);

        status = HYDU_send_strlist(ns_fd, stash.strlist);
        HYDU_ERR_POP(status, "error sending string list\n");
        HYD_STRING_STASH_FREE(stash);

        status = HYDU_sock_read(ns_fd, &len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading from nameserver\n");
        HYDU_ASSERT(!closed, status);

        if (len) {
            HYDU_MALLOC(resp, char *, len, status);
            status = HYDU_sock_read(ns_fd, resp, len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "error reading from nameserver\n");
            HYDU_ASSERT(len == recvd, status);
        }

        close(ns_fd);

        *value = resp;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
