/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "utarray.h"

UT_array *pg_list;

void HYDU_init_pg(void)
{
    static UT_icd pg_icd = { sizeof(struct HYD_pg), NULL, NULL, NULL };
    utarray_new(pg_list, &pg_icd, MPL_MEM_OTHER);

    int pgid = HYDU_alloc_pg();
    assert(pgid == 0);
}

int HYDU_alloc_pg(void)
{
    HYDU_FUNC_ENTER();

    int pgid = utarray_len(pg_list);
    utarray_extend_back(pg_list, 1);
    struct HYD_pg *pg = (struct HYD_pg *) utarray_eltptr(pg_list, pgid);
    pg->pgid = pgid;
    pg->is_active = true;

    HYDU_FUNC_EXIT();
    return pgid;
}

void HYDU_free_pg_list(void)
{
    struct HYD_pg *pg = NULL;

    while (pg = (struct HYD_pg *) utarray_next(pg_list, pg)) {
        if (pg->proxy_list)
            HYDU_free_proxy_list(pg->proxy_list);

        if (pg->user_node_list)
            HYDU_free_node_list(pg->user_node_list);
    }

    utarray_free(pg_list);
}

int HYDU_pg_max_id(void)
{
    return utarray_len(pg_list);
}

struct HYD_pg *HYDU_get_pg(int pgid)
{
    if (pgid >= 0 && pgid < utarray_len(pg_list)) {
        return (struct HYD_pg *) utarray_eltptr(pg_list, pgid);
    } else {
        return NULL;
    }
}
