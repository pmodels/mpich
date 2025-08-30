/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef COLL_CSEL_H_INCLUDED
#define COLL_CSEL_H_INCLUDED

#include "json.h"

int MPIR_Csel_create_from_file(const char *json_file,
                               void *(*create_container) (struct json_object *), void **csel);
int MPIR_Csel_create_from_buf(const char *json,
                              void *(*create_container) (struct json_object *), void **csel);
int MPIR_Csel_free(void *csel);
int MPIR_Csel_prune(void *root_csel, MPIR_Comm * comm_ptr, void **comm_csel);
void *MPIR_Csel_search(void *csel, MPIR_Csel_coll_sig_s * coll_sig);

/* boolean csel checker_functions */
MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_size_ge_pof2(MPIR_Csel_coll_sig_s * coll_sig)
{
    switch (coll_sig->coll_type) {
        case MPIR_CSEL_COLL_TYPE__REDUCE:
        case MPIR_CSEL_COLL_TYPE__IREDUCE:
        case MPIR_CSEL_COLL_TYPE__ALLREDUCE:
        case MPIR_CSEL_COLL_TYPE__IALLREDUCE:
            return (coll_sig->reduce.count >= MPL_pof2(coll_sig->comm_ptr->local_size));
        default:
            return false;
    }
}

MPL_STATIC_INLINE_PREFIX bool MPIR_Csel_displs_ordered(MPIR_Csel_coll_sig_s * coll_sig)
{
    const MPI_Aint *counts = NULL;
    const MPI_Aint *displs = NULL;
    switch (coll_sig->coll_type) {
        case MPIR_CSEL_COLL_TYPE__ALLGATHERV:
        case MPIR_CSEL_COLL_TYPE__IALLGATHERV:
            counts = coll_sig->allgatherv.recvcounts;
            displs = coll_sig->allgatherv.displs;
            break;
        default:
            return false;
    }

    MPI_Aint pos = 0;
    for (int i = 0; i < coll_sig->comm_ptr->local_size; i++) {
        if (pos != displs[i])
            return false;
        pos += counts[i];
    }
    return true;
}

MPL_STATIC_INLINE_PREFIX void MPIR_init_coll_sig(MPIR_Csel_coll_sig_s * coll_sig)
{
    /* place holder for now */
}

#endif /* COLL_CSEL_H_INCLUDED */
