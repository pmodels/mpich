/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
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

HYD_status HYDU_add_to_node_list(const char *hostname, int num_procs,
                                 struct HYD_node ** node_list)
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

static char local_hostname[MAX_HOSTNAME_LEN] = { 0 };

HYD_status HYDU_gethostname(char *hostname)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (strcmp(local_hostname, "")) {
        HYDU_snprintf(hostname, MAX_HOSTNAME_LEN, "%s", local_hostname);
        goto fn_exit;
    }

    if (gethostname(hostname, MAX_HOSTNAME_LEN) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR,
                            "gethostname error (hostname: %s; errno: %d)\n", hostname, errno);

    HYDU_snprintf(local_hostname, MAX_HOSTNAME_LEN, "%s", hostname);

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
