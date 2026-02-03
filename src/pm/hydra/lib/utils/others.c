/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"

int HYDU_dceil(int x, int y)
{
    int z;

    z = x / y;

    if (z * y == x)
        return z;
    else
        return z + 1;
}

HYD_status HYDU_add_to_node_list(const char *hostname, int num_procs, struct HYD_node ** node_list)
{
    struct HYD_node *node;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (*node_list == NULL) {
        status = HYDU_alloc_node(node_list);
        HYDU_ERR_POP(status, "unable to allocate node\n");

        (*node_list)->hostname = MPL_strdup(hostname);
        (*node_list)->core_count = num_procs;
        (*node_list)->node_id = 0;
    } else {
        for (node = *node_list; node->next; node = node->next);

        if (strcmp(node->hostname, hostname)) {
            /* If the hostname does not match, create a new node */
            status = HYDU_alloc_node(&node->next);
            HYDU_ERR_POP(status, "unable to allocate node\n");

            node->next->node_id = node->node_id + 1;

            node = node->next;
            node->hostname = MPL_strdup(hostname);
        }

        node->core_count += num_procs;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* user may spawn with a separate host list (potentially with different core count).
 * We create a separate user_node_list and use it to generate rankmap, but also need
 * merge with the main node list to avoid duplicate proxies.
 *
 * This is a variation of HYDU_add_to_node_list. It checks for existing nodes. If exist,
 * update the core_count to the max of both; Otherwise, add to existing nodes. It returns
 * the ndoe id from the node list.
 */
static HYD_status merge_to_node_list(const char *hostname, int num_procs,
                                     struct HYD_node **node_list, int *new_node_id)
{
    HYD_status status = HYD_SUCCESS;

    struct HYD_node *node = NULL;
    struct HYD_node *last_node = NULL;

    for (node = *node_list; node; node = node->next) {
        if (strcmp(node->hostname, hostname) == 0) {
            break;
        }
        last_node = node;
    }

    if (node) {
        /* if found, update core_count */
        if (num_procs > node->core_count) {
            node->core_count = num_procs;
        }
    } else {
        /* not found, add to node_list */
        status = HYDU_alloc_node(&node);
        HYDU_ERR_POP(status, "unable to allocate node\n");

        node->hostname = MPL_strdup(hostname);
        node->core_count = num_procs;

        if (last_node) {
            last_node->next = node;
            node->node_id = last_node->node_id + 1;
        } else {
            node->node_id = 0;
            *node_list = node;
        }
    }

    *new_node_id = node->node_id;

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_merge_user_node_list(struct HYD_node *user_node_list, struct HYD_node **node_list_p)
{
    HYD_status status = HYD_SUCCESS;

    for (struct HYD_node * node = user_node_list; node; node = node->next) {
        status = merge_to_node_list(node->hostname, node->core_count, node_list_p, &node->node_id);
        HYDU_ERR_POP(status, "unable to allocate node\n");
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

void HYDU_delay(unsigned long delay)
{
    struct timeval start, end;

    if (delay == 0)
        return;

    gettimeofday(&start, NULL);
    while (1) {
        gettimeofday(&end, NULL);
        if ((1000000.0 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)) > delay)
            break;
    }
}
