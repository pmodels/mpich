/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"

/* HYDU_gen_rankmap -
 *     when user doesn't specify a rankmap, we generate one using round-robin policy.
 */
static int get_max_overscribe(struct HYD_node *node_list);

HYD_status HYDU_gen_rankmap(int process_count, struct HYD_node *node_list, int **rankmap_out)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    int max_oversubscribe = get_max_overscribe(node_list);
    int rank = 0;

    int *map = MPL_malloc(process_count * sizeof(int), MPL_MEM_OTHER);
    HYDU_ASSERT(map, status);

    /* filler round */
    if (max_oversubscribe > 0) {
        for (struct HYD_node * node = node_list; node; node = node->next) {
            int c = node->core_count * max_oversubscribe - node->active_processes;
            for (int i = 0; i < c; i++) {
                map[rank++] = node->node_id;
                if (rank == process_count) {
                    goto fn_done;
                }
            }
        }
    }

    /* round-robin */
    while (true) {
        for (struct HYD_node * node = node_list; node; node = node->next) {
            int c = node->core_count;
            for (int i = 0; i < c; i++) {
                map[rank++] = node->node_id;
                if (rank == process_count) {
                    goto fn_done;
                }
            }
        }
    }

  fn_done:
    *rankmap_out = map;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

static int get_max_overscribe(struct HYD_node *node_list)
{
    int max_oversubscribe = 0;
    for (struct HYD_node * node = node_list; node; node = node->next) {
        int c = HYDU_dceil(node->active_processes, node->core_count);
        if (max_oversubscribe < c) {
            max_oversubscribe = c;
        }
    }

    bool has_spare = false;
    for (struct HYD_node * node = node_list; node; node = node->next) {
        if (node->core_count * max_oversubscribe - node->active_processes > 0) {
            has_spare = true;
            break;
        }
    }
    if (!has_spare) {
        max_oversubscribe = 0;
    }

    /* NOTE: returning 0 means we can skip the filler round */
    return max_oversubscribe;
}
