/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_ucc.h"
#include "mpid_ucc_collops.h"
#include "mpid_ucc_dtypes.h"

static inline ucc_status_t mpidi_ucc_barrier_init(MPIR_Comm * comm_ptr,
                                                  MPIDI_common_ucc_req_t * req)
{
    ucc_coll_args_t coll = {
        .mask = 0,
        .flags = 0,
        .coll_type = UCC_COLL_TYPE_BARRIER
    };

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(barrier, "comm %p, comm_id %u, comm_size %d",
                                             (void *) comm_ptr, comm_ptr->context_id,
                                             MPIR_Comm_size(comm_ptr));

    MPIDI_COMMON_UCC_REQ_INIT(req, coll, comm_ptr);

    return UCC_OK;
  fallback:
    return UCC_ERR_NOT_SUPPORTED;
}

int MPIDI_common_ucc_barrier(MPIR_Comm * comm_ptr)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    MPIDI_common_ucc_req_t req = { 0 };

    MPIDI_COMMON_UCC_WRAPPER_ENTER(barrier);

    MPIDI_COMMON_UCC_WRAPPER_EXECUTE(barrier, comm_ptr, &req);

    MPIDI_COMMON_UCC_WRAPPER_EXIT(barrier);
}
