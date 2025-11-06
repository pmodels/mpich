/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_ucc.h"
#include "mpid_ucc_collops.h"
#include "mpid_ucc_dtypes.h"

static inline ucc_status_t mpidi_ucc_barrier_init(MPIR_Comm * comm_ptr, ucc_coll_req_h * req,
                                                  MPIR_Request * coll_req)
{
    ucc_coll_args_t coll = {
        .mask = 0,
        .flags = 0,
        .coll_type = UCC_COLL_TYPE_BARRIER
    };

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(barrier, "comm %p, comm_id %u, comm_size %d",
                                             (void *) comm_ptr, comm_ptr->context_id,
                                             MPIR_Comm_size(comm_ptr));

    MPIDI_COMMON_UCC_REQ_INIT(coll_req, req, coll, comm_ptr);
    return UCC_OK;
  fallback:
    return UCC_ERR_NOT_SUPPORTED;
}

int MPIDI_common_ucc_barrier(MPIR_Comm * comm_ptr)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    ucc_coll_req_h req;

    MPIDI_COMMON_UCC_CHECK_ENABLED(comm_ptr, BARRIER);

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_TRY_TO_RUN(barrier);

    MPIDI_COMMON_UCC_CALL_AND_CHECK(mpidi_ucc_barrier_init(comm_ptr, &req, NULL));
    MPIDI_COMMON_UCC_POST_AND_CHECK(req);
    MPIDI_COMMON_UCC_WAIT_AND_CHECK(req);

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_DONE_SUCCESS(barrier);

    return MPIDI_COMMON_UCC_RETVAL_SUCCESS;

  fallback:
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_FALLBACK(barrier);
    return MPIDI_COMMON_UCC_RETVAL_FALLBACK;
  disabled:
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_DISABLED(barrier);
    goto fallback;
}
