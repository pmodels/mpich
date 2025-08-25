/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "coll_impl.h"
#include "mpl.h"
#include "coll_csel.h"

static void parse_container_params(struct json_object *obj, MPII_Csel_container_s * cnt)
{
    MPIR_Assert(obj != NULL);
    char *ckey;

    switch (cnt->id) {
            MPIR_COLL_CONTAINER_PARSE_PARAMS();
        default:
            /* Algorithm does not have parameters */
            break;
    }
}

void *MPII_Create_container(struct json_object *obj)
{
    MPII_Csel_container_s *cnt = MPL_malloc(sizeof(MPII_Csel_container_s), MPL_MEM_COLL);

    json_object_object_foreach(obj, key, val) {
        char *ckey = MPL_strdup_no_spaces(key);

        MPIR_COLL_SET_CONTAINER_ID();

        MPL_free(ckey);
    }

    /* process algorithm parameters */
    parse_container_params(json_object_object_get(obj, key), cnt);

    return (void *) cnt;
}
