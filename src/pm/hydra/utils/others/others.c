/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_merge_proxy_segment(char *hostname, struct HYD_Proxy_segment *segment,
                                        struct HYD_Proxy **proxy_list)
{
    struct HYD_Proxy *proxy;
    struct HYD_Proxy_segment *s;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (*proxy_list == NULL) {
        status = HYDU_alloc_proxy(proxy_list);
        HYDU_ERR_POP(status, "Unable to alloc proxy\n");
        (*proxy_list)->segment_list = segment;
        (*proxy_list)->hostname = HYDU_strdup(hostname);
        (*proxy_list)->proxy_core_count += segment->proc_count;
    }
    else {
        proxy = *proxy_list;
        while (proxy) {
            if (strcmp(proxy->hostname, hostname) == 0) {
                if (proxy->segment_list == NULL)
                    proxy->segment_list = segment;
                else {
                    s = proxy->segment_list;
                    while (s->next)
                        s = s->next;
                    s->next = segment;
                }
                proxy->proxy_core_count += segment->proc_count;
                break;
            }
            else if (proxy->next == NULL) {
                status = HYDU_alloc_proxy(&proxy->next);
                HYDU_ERR_POP(status, "Unable to alloc proxy\n");
                proxy->next->segment_list = segment;
                proxy->next->hostname = HYDU_strdup(hostname);
                proxy->next->proxy_core_count += segment->proc_count;
                break;
            }
            else {
                proxy = proxy->next;
            }
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

int HYDU_local_to_global_id(int local_id, int core_count,
                            struct HYD_Proxy_segment *segment_list, int global_core_count)
{
    int global_id, rem;
    struct HYD_Proxy_segment *segment;

    global_id = ((local_id / core_count) * global_core_count);
    rem = (local_id % core_count);

    for (segment = segment_list; segment; segment = segment->next) {
        if (rem >= segment->proc_count)
            rem -= segment->proc_count;
        else {
            global_id += segment->start_pid + rem;
            break;
        }
    }

    return global_id;
}
