/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_ucc.h"
#include "mpid_ucc_collops.h"
#include "mpid_ucc_dtypes.h"

static inline ucc_status_t mpidi_ucc_reduce_scatter_block_init(const void *sbuf, void *rbuf,
                                                               MPI_Aint rcount, MPI_Datatype dtype,
                                                               MPI_Op op, MPIR_Comm * comm_ptr,
                                                               ucc_coll_req_h * req,
                                                               MPIR_Request * coll_req)
{
    bool is_inplace = (sbuf == MPI_IN_PLACE);
    int comm_size = MPIR_Comm_size(comm_ptr);

    ucc_datatype_t ucc_dt = mpidi_mpi_dtype_to_ucc_dtype(dtype);
    ucc_reduction_op_t ucc_op = mpidi_mpi_op_to_ucc_reduction_op(op);

    if (is_inplace) {
        MPIDI_COMMON_UCC_VERBOSE_INPLACE_UNSUPPORTED(reduce_scatter_block);
        goto fallback;
    }

    if (ucc_dt == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED) {
        MPIDI_COMMON_UCC_VERBOSE_DTYPE_UNSUPPORTED(reduce_scatter_block);
        goto fallback;
    }

    if (ucc_op == MPIDI_COMMON_UCC_REDUCTION_OP_UNSUPPORTED) {
        MPIDI_COMMON_UCC_VERBOSE_REDUCTION_OP_UNSUPPORTED(reduce_scatter_block);
        goto fallback;
    }

    ucc_coll_args_t coll = {
        .mask = 0,
        .flags = 0,
        .coll_type = UCC_COLL_TYPE_REDUCE_SCATTER,
        .src.info = {
                     .buffer = (void *) sbuf,
                     .count = rcount * comm_size,
                     .datatype = ucc_dt,
                     .mem_type = UCC_MEMORY_TYPE_UNKNOWN,
                     }
        ,
        .dst.info = {
                     .buffer = rbuf,
                     .count = rcount,
                     .datatype = ucc_dt,
                     .mem_type = UCC_MEMORY_TYPE_UNKNOWN,
                     },
        .op = ucc_op,
    };

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(reduce_scatter_block,
                                             "comm %p, comm_id %u, comm_size %d"
                                             ", rcount %zu, dtype %s, op %s", (void *) comm_ptr,
                                             comm_ptr->context_id, comm_size, rcount,
                                             mpidi_ucc_dtype_to_str(ucc_dt),
                                             mpidi_ucc_reduction_op_to_str(ucc_op));

    MPIDI_COMMON_UCC_REQ_INIT(coll_req, req, coll, comm_ptr);
    return UCC_OK;
  fallback:
    return UCC_ERR_NOT_SUPPORTED;
}

int MPIDI_common_ucc_reduce_scatter_block(const void *sbuf, void *rbuf, MPI_Aint rcount,
                                          MPI_Datatype dtype, MPI_Op op, MPIR_Comm * comm_ptr)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    ucc_coll_req_h req;

    MPIDI_COMMON_UCC_CHECK_ENABLED(comm_ptr, reduce_scatter_block);

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_TRY_TO_RUN(reduce_scatter_block);

    MPIDI_COMMON_UCC_CALL_AND_CHECK(mpidi_ucc_reduce_scatter_block_init
                                    (sbuf, rbuf, rcount, dtype, op, comm_ptr, &req, NULL));
    MPIDI_COMMON_UCC_POST_AND_CHECK(req);
    MPIDI_COMMON_UCC_WAIT_AND_CHECK(req);

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_DONE_SUCCESS(reduce_scatter_block);

    return MPIDI_COMMON_UCC_RETVAL_SUCCESS;

  fallback:
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_FALLBACK(reduce_scatter_block);
    return MPIDI_COMMON_UCC_RETVAL_FALLBACK;
  disabled:
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_DISABLED(reduce_scatter_block);
    goto fallback;
}
