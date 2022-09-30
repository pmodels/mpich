/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"

struct HYD_pg *HYDU_get_pg(int pgid)
{
    for (struct HYD_pg * pg = &HYD_server_info.pg_list; pg; pg = pg->next) {
        if (pg->pgid == pgid) {
            return pg;
        }
    }
    return NULL;
}
