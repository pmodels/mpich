/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

static HYD_status builtin_publish(char *name, char *port, int *success);
static HYD_status builtin_unpublish(char *name, int *success);
static HYD_status builtin_lookup(char *name, char **value);
static HYD_status server_publish(char *name, char *port, int *success);
static HYD_status server_unpublish(char *name, int *success);
static HYD_status server_lookup(char *name, char **value);

HYD_status HYD_pmcd_pmi_publish(char *name, char *port, int *success)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_server_info.nameserver == NULL) {
        status = builtin_publish(name, port, success);
    } else {
        status = server_publish(name, port, success);
    }

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYD_pmcd_pmi_unpublish(char *name, int *success)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_server_info.nameserver == NULL) {
        status = builtin_unpublish(name, success);
    } else {
        status = server_unpublish(name, success);
    }

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYD_pmcd_pmi_lookup(char *name, char **value)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_server_info.nameserver == NULL) {
        status = builtin_lookup(name, value);
    } else {
        status = server_lookup(name, value);
    }

    HYDU_FUNC_EXIT();
    return status;
}

/* ---- internal routines using builtin list ---- */

struct builtin_publish {
    char *name;
    char *port;
    int infokeycount;

    struct builtin_info_keys {
        char *key;
        char *val;
    } *info_keys;

    struct builtin_publish *next;
};

static struct builtin_publish *builtin_publish_list = NULL;

static HYD_status builtin_free_publish(struct builtin_publish *publish)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    MPL_free(publish->name);
    MPL_free(publish->port);

    for (i = 0; i < publish->infokeycount; i++) {
        MPL_free(publish->info_keys[i].key);
        MPL_free(publish->info_keys[i].val);
    }
    MPL_free(publish->info_keys);

    HYDU_FUNC_EXIT();
    return status;
}

static HYD_status builtin_publish(char *name, char *port, int *success)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct builtin_publish *r;
    /* no external nameserver available */
    for (r = builtin_publish_list; r; r = r->next)
        if (!strcmp(r->name, name))
            break;
    if (r) {
        *success = 0;
        goto fn_exit;
    }
    *success = 1;

    struct builtin_publish *publish;
    HYDU_MALLOC_OR_JUMP(publish, struct builtin_publish *, sizeof(struct builtin_publish), status);
    publish->name = MPL_strdup(name);
    publish->port = MPL_strdup(port);
    publish->infokeycount = 0;
    publish->info_keys = NULL;
    publish->next = NULL;

    if (builtin_publish_list == NULL)
        builtin_publish_list = publish;
    else {
        for (r = builtin_publish_list; r->next; r = r->next);
        r->next = publish;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status builtin_unpublish(char *name, int *success)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *success = 0;
    if (!builtin_publish_list) {
        /* nothing is published yet */
        goto fn_exit;
    }

    struct builtin_publish *r, *publish;
    if (!strcmp(builtin_publish_list->name, name)) {
        publish = builtin_publish_list;
        builtin_publish_list = builtin_publish_list->next;
        publish->next = NULL;

        builtin_free_publish(publish);
        MPL_free(publish);
        *success = 1;
    } else {
        publish = builtin_publish_list;
        do {
            if (publish->next == NULL)
                break;
            else if (!strcmp(publish->next->name, name)) {
                r = publish->next;
                publish->next = r->next;
                r->next = NULL;

                builtin_free_publish(r);
                MPL_free(r);
                *success = 1;
            } else
                publish = publish->next;
        } while (1);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
}

static HYD_status builtin_lookup(char *name, char **value)
{
    HYDU_FUNC_ENTER();

    *value = NULL;

    struct builtin_publish *publish;
    for (publish = builtin_publish_list; publish; publish = publish->next)
        if (!strcmp(publish->name, name))
            break;

    if (publish)
        *value = MPL_strdup(publish->port);

    return HYD_SUCCESS;
}

/* ---- internal routines using name server ---- */

static HYD_status server_publish(char *name, char *port, int *success)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    char *ns, *ns_host, *ns_port_str;
    int ns_port, ns_fd = -1;
    /* connect to the external nameserver and store the
     * information there */
    ns = MPL_strdup(HYD_server_info.nameserver);

    ns_host = strtok(ns, ":");
    HYDU_ASSERT(ns_host, status);

    ns_port_str = strtok(NULL, ":");
    if (ns_port_str)
        ns_port = atoi(ns_port_str);
    else
        ns_port = HYDRA_NAMESERVER_DEFAULT_PORT;

    status = HYDU_sock_connect(ns_host, (uint16_t) ns_port, &ns_fd, 0, HYD_CONNECT_DELAY);
    HYDU_ERR_POP(status, "error connecting to the nameserver\n");

    struct HYD_string_stash stash;
    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, MPL_strdup("PUBLISH"), status);
    HYD_STRING_STASH(stash, MPL_strdup(name), status);
    HYD_STRING_STASH(stash, MPL_strdup(port), status);

    status = HYDU_send_strlist(ns_fd, stash.strlist);
    HYDU_ERR_POP(status, "error sending string list\n");
    HYD_STRING_STASH_FREE(stash);

    int len, recvd, closed;
    status = HYDU_sock_read(ns_fd, &len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading from nameserver\n");
    HYDU_ASSERT(!closed, status);

    char *resp;
    HYDU_MALLOC_OR_JUMP(resp, char *, len, status);
    status = HYDU_sock_read(ns_fd, resp, len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading from nameserver\n");
    HYDU_ASSERT(len == recvd, status);

    close(ns_fd);

    if (!strcmp(resp, "SUCCESS"))
        *success = 1;
    else
        *success = 0;

    MPL_free(resp);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    if (-1 != ns_fd)
        close(ns_fd);
    goto fn_exit;
}

static HYD_status server_unpublish(char *name, int *success)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    char *ns, *ns_host, *ns_port_str;
    int ns_port, ns_fd = -1;
    /* connect to the external nameserver and get the information
     * from there */
    ns = MPL_strdup(HYD_server_info.nameserver);

    ns_host = strtok(ns, ":");
    HYDU_ASSERT(ns_host, status);

    ns_port_str = strtok(NULL, ":");
    if (ns_port_str)
        ns_port = atoi(ns_port_str);
    else
        ns_port = HYDRA_NAMESERVER_DEFAULT_PORT;

    status = HYDU_sock_connect(ns_host, (uint16_t) ns_port, &ns_fd, 0, HYD_CONNECT_DELAY);
    HYDU_ERR_POP(status, "error connecting to the nameserver\n");

    struct HYD_string_stash stash;
    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, MPL_strdup("UNPUBLISH"), status);
    HYD_STRING_STASH(stash, MPL_strdup(name), status);

    status = HYDU_send_strlist(ns_fd, stash.strlist);
    HYDU_ERR_POP(status, "error sending string list\n");
    HYD_STRING_STASH_FREE(stash);

    int len, recvd, closed;
    status = HYDU_sock_read(ns_fd, &len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading from nameserver\n");
    HYDU_ASSERT(!closed, status);

    char *resp;
    HYDU_MALLOC_OR_JUMP(resp, char *, len, status);
    status = HYDU_sock_read(ns_fd, resp, len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading from nameserver\n");
    HYDU_ASSERT(len == recvd, status);

    close(ns_fd);

    if (!strcmp(resp, "SUCCESS")) {
        *success = 1;
    } else {
        *success = 0;
    }
    MPL_free(resp);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    if (-1 != ns_fd)
        close(ns_fd);
    goto fn_exit;
}

static HYD_status server_lookup(char *name, char **value)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    char *ns, *ns_host, *ns_port_str;
    int ns_port, ns_fd = -1;
    /* connect to the external nameserver and get the information
     * from there */
    ns = MPL_strdup(HYD_server_info.nameserver);

    ns_host = strtok(ns, ":");
    HYDU_ASSERT(ns_host, status);

    ns_port_str = strtok(NULL, ":");
    if (ns_port_str)
        ns_port = atoi(ns_port_str);
    else
        ns_port = HYDRA_NAMESERVER_DEFAULT_PORT;

    status = HYDU_sock_connect(ns_host, (uint16_t) ns_port, &ns_fd, 0, HYD_CONNECT_DELAY);
    HYDU_ERR_POP(status, "error connecting to the nameserver\n");

    struct HYD_string_stash stash;
    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, MPL_strdup("LOOKUP"), status);
    HYD_STRING_STASH(stash, MPL_strdup(name), status);

    status = HYDU_send_strlist(ns_fd, stash.strlist);
    HYDU_ERR_POP(status, "error sending string list\n");
    HYD_STRING_STASH_FREE(stash);

    int len, recvd, closed;
    status = HYDU_sock_read(ns_fd, &len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading from nameserver\n");
    HYDU_ASSERT(!closed, status);

    char *resp = NULL;
    if (len) {
        HYDU_MALLOC_OR_JUMP(resp, char *, len, status);
        status = HYDU_sock_read(ns_fd, resp, len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading from nameserver\n");
        HYDU_ASSERT(len == recvd, status);
    }

    close(ns_fd);

    *value = resp;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    if (-1 != ns_fd)
        close(ns_fd);
    goto fn_exit;
}
