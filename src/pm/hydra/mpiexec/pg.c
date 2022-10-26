/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "utarray.h"

static UT_array *pg_list;

static void pg_dtor(void *_elt)
{
    struct HYD_pg *pg = _elt;

    if (pg->rankmap) {
        MPL_free(pg->rankmap);
    }

    if (pg->proxy_list)
        HYDU_free_proxy_list(pg->proxy_list, pg->proxy_count);

    if (pg->user_node_list)
        HYDU_free_node_list(pg->user_node_list);

    if (pg->pg_scratch)
        HYD_pmcd_pmi_free_pg_scratch(pg);
}

void PMISERV_pg_init(void)
{
    static UT_icd pg_icd = { sizeof(struct HYD_pg), NULL, NULL, pg_dtor };
    utarray_new(pg_list, &pg_icd, MPL_MEM_OTHER);

    int pgid = PMISERV_pg_alloc();
    assert(pgid == 0);
}

int PMISERV_pg_alloc(void)
{
    HYDU_FUNC_ENTER();

    int pgid = utarray_len(pg_list);
    utarray_extend_back(pg_list, MPL_MEM_OTHER);
    struct HYD_pg *pg = (struct HYD_pg *) utarray_eltptr(pg_list, pgid);
    pg->pgid = pgid;
    pg->is_active = true;
    pg->spawner_pgid = -1;

    HYDU_FUNC_EXIT();
    return pgid;
}

void PMISERV_pg_finalize(void)
{
    utarray_free(pg_list);
}

int PMISERV_pg_max_id(void)
{
    return utarray_len(pg_list);
}

struct HYD_pg *PMISERV_pg_by_id(int pgid)
{
    if (pgid >= 0 && pgid < utarray_len(pg_list)) {
        return (struct HYD_pg *) utarray_eltptr(pg_list, pgid);
    } else {
        return NULL;
    }
}
