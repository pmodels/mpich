/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

void MPIR_Coll_owned_init(MPIR_Coll_owned_t * o)
{
    utarray_init(&o->buffers, &ut_ptr_icd);
}

void MPIR_Coll_owned_add_buffer(MPIR_Coll_owned_t * o, void *buf)
{
    utarray_push_back(&o->buffers, &buf, MPL_MEM_COLL);
}

void MPIR_Coll_owned_free(MPIR_Coll_owned_t * o)
{
    for (void **p = (void **)utarray_front(&o->buffers); p;
         p = (void **) utarray_next(&o->buffers, p)) {
        MPL_free(*p);
    }
    utarray_done(&o->buffers);
}
