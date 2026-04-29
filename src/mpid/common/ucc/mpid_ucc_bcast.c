/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_ucc.h"
#include "mpid_ucc_collops.h"
#include "mpid_ucc_dtypes.h"

static inline ucc_status_t mpidi_ucc_bcast_init(void *buf, MPI_Aint count,
                                                MPI_Datatype dtype, int root,
                                                MPIR_Comm * comm_ptr, MPIDI_common_ucc_req_t * req)
{
    int comm_rank = MPIR_Comm_rank(comm_ptr);
    bool is_root = (comm_rank == root);
    ucc_datatype_t ucc_dt = mpidi_mpi_dtype_to_ucc_dtype(dtype);

    if (ucc_dt == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED) {
        MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_TRY(bcast);
        if (is_root) {
            ucc_dt =
                mpidi_ucc_dtype_packing_send(buf, count, 1 /* single send chunk */ , dtype, req);
        } else {
            ucc_dt =
                mpidi_ucc_dtype_packing_recv_prep(buf, count, dtype, 1 /* single recv chunk */ ,
                                                  req);
        }
        MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_RES(bcast, ucc_dt);
    }

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
                     .buffer =
                     req->sbuf_tmp ? req->sbuf_tmp : (req->rbuf_tmp ? req->rbuf_tmp : buf),
                     .count =
                     req->scounts_tmp ? req->scounts_tmp[0] : (req->
                                                               rcounts_tmp ? req->rcounts_tmp[0] :
                                                               count),
                     .datatype = ucc_dt,
                     .mem_type = UCC_MEMORY_TYPE_UNKNOWN,
                     }
    };

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(bcast, "comm %p, comm_id %u, comm_size %d"
                                             ", count %zu, dtype %s",
                                             (void *) comm_ptr, comm_ptr->context_id,
                                             MPIR_Comm_size(comm_ptr), count,
                                             mpidi_ucc_dtype_to_str(ucc_dt));

    MPIDI_COMMON_UCC_REQ_INIT(req, coll, comm_ptr);

    return UCC_OK;
  fallback:
    return UCC_ERR_NOT_SUPPORTED;
}

int MPIDI_common_ucc_bcast(void *buf, MPI_Aint count, MPI_Datatype dtype, int root,
                           MPIR_Comm * comm_ptr)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    MPIDI_common_ucc_req_t req = { 0 };

    MPIDI_COMMON_UCC_WRAPPER_ENTER(bcast);

    MPIDI_COMMON_UCC_WRAPPER_EXECUTE(bcast, buf, count, dtype, root, comm_ptr, &req);

    mpidi_ucc_dtype_packing_recv_done(buf, count, dtype, 1 /* single recv chunk */ , &req);

    MPIDI_COMMON_UCC_WRAPPER_EXIT(bcast);
}
