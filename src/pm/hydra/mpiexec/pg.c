/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"

void HYDU_init_pg(struct HYD_pg *pg, int pgid)
{
    pg->pgid = pgid;
    pg->is_active = true;
    pg->proxy_list = NULL;
    pg->proxy_count = 0;
    pg->pg_process_count = 0;
    pg->barrier_count = 0;
    pg->spawner_pg = NULL;
    pg->user_node_list = NULL;
    pg->pg_core_count = 0;
    pg->pg_scratch = NULL;
    pg->next = NULL;
}

HYD_status HYDU_alloc_pg(struct HYD_pg **pg, int pgid)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(*pg, struct HYD_pg *, sizeof(struct HYD_pg), status);
    HYDU_init_pg(*pg, pgid);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDU_free_pg_list(struct HYD_pg *pg_list)
{
    struct HYD_pg *pg, *tpg;

    pg = pg_list;
    while (pg) {
        tpg = pg->next;

        if (pg->proxy_list)
            HYDU_free_proxy_list(pg->proxy_list);

        if (pg->user_node_list)
            HYDU_free_node_list(pg->user_node_list);

        MPL_free(pg);

        pg = tpg;
    }
}

struct HYD_pg *HYDU_get_pg(int pgid)
{
    for (struct HYD_pg * pg = &HYD_server_info.pg_list; pg; pg = pg->next) {
        if (pg->pgid == pgid) {
            return pg;
        }
    }
    return NULL;
}
