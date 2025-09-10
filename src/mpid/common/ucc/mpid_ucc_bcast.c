/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_ucc.h"
#include "mpid_ucc_collops.h"
#include "mpid_ucc_dtypes.h"

static inline ucc_status_t mpidi_ucc_bcast_init(void *buf, MPI_Aint count,
                                                MPI_Datatype dtype, int root,
                                                MPIR_Comm * comm_ptr, ucc_coll_req_h * req,
                                                MPIR_Request * coll_req)
{
    ucc_datatype_t ucc_dt = mpidi_mpi_dtype_to_ucc_dtype(dtype);

    if (ucc_dt == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED) {
        MPIDI_COMMON_UCC_VERBOSE_DTYPE_UNSUPPORTED(bcast);
        goto fallback;
    }

    ucc_coll_args_t coll = {
        .mask = 0,
        .flags = 0,
        .coll_type = UCC_COLL_TYPE_BCAST,
        .root = root,
        .src.info = {
                     .buffer = buf,
                     .count = count,
                     .datatype = ucc_dt,
                     .mem_type = UCC_MEMORY_TYPE_UNKNOWN,
                     }
    };

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(bcast, "comm %p, comm_id %u, comm_size %d"
                                             ", count %zu, dtype %s",
                                             (void *) comm_ptr, comm_ptr->context_id,
                                             MPIR_Comm_size(comm_ptr), count,
                                             mpidi_ucc_dtype_to_str(ucc_dt));

    MPIDI_COMMON_UCC_REQ_INIT(coll_req, req, coll, comm_ptr);
    return UCC_OK;
  fallback:
    return UCC_ERR_NOT_SUPPORTED;
}

int MPIDI_common_ucc_bcast(void *buf, MPI_Aint count, MPI_Datatype dtype, int root,
                           MPIR_Comm * comm_ptr)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    ucc_coll_req_h req;

    MPIDI_COMMON_UCC_CHECK_ENABLED(comm_ptr, bcast);

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_TRY_TO_RUN(bcast);

    MPIDI_COMMON_UCC_CALL_AND_CHECK(mpidi_ucc_bcast_init
                                    (buf, count, dtype, root, comm_ptr, &req, NULL));
    MPIDI_COMMON_UCC_POST_AND_CHECK(req);
    MPIDI_COMMON_UCC_WAIT_AND_CHECK(req);

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_DONE_SUCCESS(bcast);

    return MPIDI_COMMON_UCC_RETVAL_SUCCESS;

  fallback:
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_FALLBACK(bcast);
    return MPIDI_COMMON_UCC_RETVAL_FALLBACK;
  disabled:
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_DISABLED(bcast);
    goto fallback;
}
