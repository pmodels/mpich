/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpid_ucc.h"
#include "mpid_ucc_collops.h"
#include "mpid_ucc_dtypes.h"

static inline ucc_status_t mpidi_ucc_scatterv_init(const void *sbuf, const MPI_Aint scounts[],
                                                   const MPI_Aint sdispls[], MPI_Datatype sdtype,
                                                   void *rbuf, MPI_Aint rcount, MPI_Datatype rdtype,
                                                   int root, MPIR_Comm * comm_ptr,
                                                   MPIDI_common_ucc_req_t * req)
{
    int comm_rank = MPIR_Comm_rank(comm_ptr);
    int comm_size = MPIR_Comm_size(comm_ptr);
    bool is_root = (comm_rank == root);
    bool is_inplace = (rbuf == MPI_IN_PLACE);
    bool is_large_counts = (sizeof(MPI_Aint) * 8 == 64);

    ucc_datatype_t ucc_sdt = MPIDI_COMMON_UCC_DTYPE_NULL;
    ucc_datatype_t ucc_rdt = MPIDI_COMMON_UCC_DTYPE_NULL;

    uint64_t flags = 0;

    if (is_root) {
        ucc_sdt = mpidi_mpi_dtype_to_ucc_dtype(sdtype);
        if (!is_inplace) {
            ucc_rdt = mpidi_mpi_dtype_to_ucc_dtype(rdtype);
        }
    } else {
        ucc_rdt = mpidi_mpi_dtype_to_ucc_dtype(rdtype);
    }

    if (ucc_sdt == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED) {
        MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_TRY_S(scatterv);
        ucc_sdt = mpidi_ucc_dtype_packing_sendv(sbuf, scounts, sdispls, comm_size, sdtype, req);
        MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_RES(scatterv, ucc_sdt);
    }

    if (ucc_rdt == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED) {
        MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_TRY_R(scatter);
        ucc_rdt =
            mpidi_ucc_dtype_packing_recv_prep(rbuf, rcount, rdtype, 1 /* single recv chunk */ ,
                                              req);
        MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_RES(scatter, ucc_rdt);
    }

    if ((ucc_sdt == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED) ||
        (ucc_rdt == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED)) {
        MPIDI_COMMON_UCC_VERBOSE_DTYPE_UNSUPPORTED(scatterv);
        goto fallback;
    }

    if (is_root && is_inplace) {
        flags |= UCC_COLL_ARGS_FLAG_IN_PLACE;
    }

    if (is_large_counts) {
        flags |= UCC_COLL_ARGS_FLAG_COUNT_64BIT;
        flags |= UCC_COLL_ARGS_FLAG_DISPLACEMENTS_64BIT;
    }

    ucc_coll_args_t coll = {
        .mask = flags ? UCC_COLL_ARGS_FIELD_FLAGS : 0,
        .flags = flags,
        .coll_type = UCC_COLL_TYPE_SCATTERV,
        .root = root,
        .src.info_v = {
                       .buffer = req->sbuf_tmp ? req->sbuf_tmp : (void *) sbuf,
                       .counts = (ucc_count_t *) (req->scounts_tmp ? req->scounts_tmp : scounts),
                       .displacements =
                       (ucc_aint_t *) (req->sdispls_tmp ? req->sdispls_tmp : sdispls),
                       .datatype = ucc_sdt,
                       .mem_type = UCC_MEMORY_TYPE_UNKNOWN,
                       }
        ,
        .dst.info = {
                     .buffer = req->rbuf_tmp ? req->rbuf_tmp : rbuf,
                     .count = req->rcounts_tmp ? req->rcounts_tmp[0] : rcount,
                     .datatype = ucc_rdt,
                     .mem_type = UCC_MEMORY_TYPE_UNKNOWN,
                     }
    };

    if (is_root) {
        if (is_inplace) {
            MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(scatterv, "comm %p, comm_id %u, comm_size %d"
                                                     ", INPLACE, sdtype %s, root %d (me)",
                                                     (void *) comm_ptr, comm_ptr->context_id,
                                                     comm_size, mpidi_ucc_dtype_to_str(ucc_sdt),
                                                     root);
        } else {
            MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(scatterv, "comm %p, comm_id %u, comm_size %d"
                                                     ", sdtype %s, rcount %zu, rdtype %s"
                                                     ", root %d (me)",
                                                     (void *) comm_ptr, comm_ptr->context_id,
                                                     comm_size,
                                                     mpidi_ucc_dtype_to_str(ucc_sdt), rcount,
                                                     mpidi_ucc_dtype_to_str(ucc_rdt), root);
        }
    } else {
        MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(scatterv, "comm %p, comm_id %u, comm_size %d"
                                                 ", rcount %zu, rdtype %s, root %d",
                                                 (void *) comm_ptr, comm_ptr->context_id,
                                                 comm_size, rcount,
                                                 mpidi_ucc_dtype_to_str(ucc_rdt), root);
    }

    MPIDI_COMMON_UCC_REQ_INIT(req, coll, comm_ptr);

    return UCC_OK;
  fallback:
    return UCC_ERR_NOT_SUPPORTED;
}

int MPIDI_common_ucc_scatterv(const void *sbuf, const MPI_Aint scounts[], const MPI_Aint sdispls[],
                              MPI_Datatype sdtype, void *rbuf, MPI_Aint rcount, MPI_Datatype rdtype,
                              int root, MPIR_Comm * comm_ptr)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    MPIDI_common_ucc_req_t req = { 0 };

    MPIDI_COMMON_UCC_WRAPPER_ENTER(scatterv);

    MPIDI_COMMON_UCC_WRAPPER_EXECUTE(scatterv, sbuf, scounts, sdispls, sdtype, rbuf, rcount, rdtype,
                                     root, comm_ptr, &req);

    mpidi_ucc_dtype_packing_recv_done(rbuf, rcount, rdtype, 1 /* single recv chunk */ , &req);

    MPIDI_COMMON_UCC_WRAPPER_EXIT(scatterv);
}
