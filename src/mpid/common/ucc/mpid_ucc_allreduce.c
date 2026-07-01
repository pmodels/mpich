/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpid_ucc.h"
#include "mpid_ucc_collops.h"
#include "mpid_ucc_dtypes.h"

static inline ucc_status_t mpidi_ucc_allreduce_init(const void *sbuf, void *rbuf, MPI_Aint count,
                                                    MPI_Datatype dtype, MPI_Op op,
                                                    MPIR_Comm * comm_ptr,
                                                    MPIDI_common_ucc_req_t * req)
{
    bool is_inplace = (sbuf == MPI_IN_PLACE);
    int comm_size = MPIR_Comm_size(comm_ptr);

    ucc_datatype_t ucc_dt = mpidi_mpi_dtype_to_ucc_dtype(dtype);
    ucc_reduction_op_t ucc_op = mpidi_mpi_op_to_ucc_reduction_op(op);

    if (ucc_dt == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED) {
        MPIDI_COMMON_UCC_VERBOSE_DTYPE_UNSUPPORTED(allreduce);
        goto fallback;
    }

    if (ucc_op == MPIDI_COMMON_UCC_REDUCTION_OP_UNSUPPORTED) {
        MPIDI_COMMON_UCC_VERBOSE_REDUCTION_OP_UNSUPPORTED(allreduce);
        goto fallback;
    }

    ucc_coll_args_t coll = {
        .mask = 0,
        .flags = 0,
        .coll_type = UCC_COLL_TYPE_ALLREDUCE,
        .src.info = {
                     .buffer = (void *) sbuf,
                     .count = count,
                     .datatype = ucc_dt,
                     .mem_type = UCC_MEMORY_TYPE_UNKNOWN,
                     }
        ,
        .dst.info = {
                     .buffer = rbuf,
                     .count = count,
                     .datatype = ucc_dt,
                     .mem_type = UCC_MEMORY_TYPE_UNKNOWN,
                     },
        .op = ucc_op,
    };

    if (is_inplace) {

        coll.mask = UCC_COLL_ARGS_FIELD_FLAGS;
        coll.flags = UCC_COLL_ARGS_FLAG_IN_PLACE;

        MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(allreduce, "comm %p, comm_id %u, comm_size %d"
                                                 ", INPLACE, count %zu, dtype %s, op %s",
                                                 (void *) comm_ptr, comm_ptr->context_id,
                                                 comm_size, count, mpidi_ucc_dtype_to_str(ucc_dt),
                                                 mpidi_ucc_reduction_op_to_str(ucc_op));
    } else {
        MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(allreduce, "comm %p, comm_id %u, comm_size %d"
                                                 ", count %zu, dtype %s, op %s",
                                                 (void *) comm_ptr, comm_ptr->context_id,
                                                 comm_size, count, mpidi_ucc_dtype_to_str(ucc_dt),
                                                 mpidi_ucc_reduction_op_to_str(ucc_op));
    }

    MPIDI_COMMON_UCC_REQ_INIT(req, coll, comm_ptr);

    return UCC_OK;
  fallback:
    return UCC_ERR_NOT_SUPPORTED;
}

int MPIDI_common_ucc_allreduce(const void *sbuf, void *rbuf, MPI_Aint count,
                               MPI_Datatype dtype, MPI_Op op, MPIR_Comm * comm_ptr)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    MPIDI_common_ucc_req_t req = { 0 };

    MPIDI_COMMON_UCC_WRAPPER_ENTER(allreduce);

    MPIDI_COMMON_UCC_WRAPPER_EXECUTE(allreduce, sbuf, rbuf, count, dtype, op, comm_ptr, &req);

    MPIDI_COMMON_UCC_WRAPPER_EXIT(allreduce);
}
