/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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

        (*node_list)->hostname = HYDU_strdup(hostname);
        (*node_list)->core_count = num_procs;
        (*node_list)->node_id = 0;
    }
    else {
        for (node = *node_list; node->next; node = node->next);

        if (strcmp(node->hostname, hostname)) {
            /* If the hostname does not match, create a new node */
            status = HYDU_alloc_node(&node->next);
            HYDU_ERR_POP(status, "unable to allocate node\n");

            node->next->node_id = node->node_id + 1;

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
