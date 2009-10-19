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
        (*proxy_list)->hostname = HYDU_strdup(hostname);
        (*proxy_list)->start_pid = start_pid;
        (*proxy_list)->proxy_core_count = core_count;
    }
    else {
        /* Run to the last proxy */
        for (proxy = *proxy_list; proxy->next; proxy = proxy->next);

        if (strcmp(proxy->hostname, hostname)) {        /* hostname doesn't match */
            status = HYDU_alloc_proxy(&proxy->next);
            HYDU_ERR_POP(status, "unable to alloc proxy\n");
            proxy->next->hostname = HYDU_strdup(hostname);
            proxy->next->start_pid = start_pid;
            proxy = proxy->next;
        }
        proxy->proxy_core_count += core_count;
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

HYD_status HYDU_add_to_proxy_list(char *hostname, int num_procs,
                                  struct HYD_proxy **proxy_list)
{
    static int pid = 0;
    struct HYD_proxy *proxy;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (*proxy_list == NULL) {
        status = HYDU_alloc_proxy(proxy_list);
        HYDU_ERR_POP(status, "unable to allocate proxy\n");

        (*proxy_list)->hostname = HYDU_strdup(hostname);

        (*proxy_list)->start_pid = 0;
        (*proxy_list)->proxy_core_count = num_procs;
    }
    else {
        for (proxy = *proxy_list; proxy->next; proxy = proxy->next);

        if (strcmp(proxy->hostname, hostname)) {
            /* If the hostname does not match, create a new proxy */
            status = HYDU_alloc_proxy(&proxy->next);
            HYDU_ERR_POP(status, "unable to allocate proxy\n");

            proxy = proxy->next;

            proxy->hostname = HYDU_strdup(hostname);

            proxy->start_pid = pid;
        }

        proxy->proxy_core_count += num_procs;
    }
    pid += num_procs;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
