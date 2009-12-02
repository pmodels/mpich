/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_status HYDU_merge_proxy_segment(char *hostname, int start_pid, int core_count,
                                    struct HYD_proxy **proxy_list)
{
    struct HYD_proxy *proxy;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (*proxy_list == NULL) {
        status = HYDU_alloc_proxy(proxy_list);
        HYDU_ERR_POP(status, "Unable to alloc proxy\n");
        (*proxy_list)->node.hostname = HYDU_strdup(hostname);
        (*proxy_list)->node.core_count = core_count;
        (*proxy_list)->start_pid = start_pid;
    }
    else {
        /* Run to the last proxy */
        for (proxy = *proxy_list; proxy->next; proxy = proxy->next);

        if (strcmp(proxy->node.hostname, hostname)) {        /* hostname doesn't match */
            status = HYDU_alloc_proxy(&proxy->next);
            HYDU_ERR_POP(status, "unable to alloc proxy\n");
            proxy->next->node.hostname = HYDU_strdup(hostname);
            proxy->next->start_pid = start_pid;
            proxy = proxy->next;
        }
        proxy->node.core_count += core_count;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

int HYDU_local_to_global_id(int local_id, int start_pid, int core_count, int global_core_count)
{
    return ((local_id / core_count) * global_core_count) + (local_id % core_count) + start_pid;
}

HYD_status HYDU_add_to_node_list(char *hostname, int num_procs, struct HYD_node ** node_list)
{
    struct HYD_node *node;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (*node_list == NULL) {
        status = HYDU_alloc_node(node_list);
        HYDU_ERR_POP(status, "unable to allocate node\n");

        (*node_list)->hostname = HYDU_strdup(hostname);
        (*node_list)->core_count = num_procs;
    }
    else {
        for (node = *node_list; node->next; node = node->next);

        if (strcmp(node->hostname, hostname)) {
            /* If the hostname does not match, create a new node */
            status = HYDU_alloc_node(&node->next);
            HYDU_ERR_POP(status, "unable to allocate node\n");

            node = node->next;
            node->hostname = HYDU_strdup(hostname);
        }

        node->core_count += num_procs;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
