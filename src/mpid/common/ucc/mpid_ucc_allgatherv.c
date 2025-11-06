/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_ucc.h"
#include "mpid_ucc_collops.h"
#include "mpid_ucc_dtypes.h"

static inline ucc_status_t mpidi_ucc_allgatherv_init(const void *sbuf, MPI_Aint scount,
                                                     MPI_Datatype sdtype, void *rbuf,
                                                     const MPI_Aint rcounts[],
                                                     const MPI_Aint rdispls[], MPI_Datatype rdtype,
                                                     MPIR_Comm * comm_ptr, ucc_coll_req_h * req,
                                                     MPIR_Request * coll_req)
{
    bool is_inplace = (sbuf == MPI_IN_PLACE);
    int comm_size = MPIR_Comm_size(comm_ptr);
    bool is_large_counts = (sizeof(MPI_Aint) * 8 == 64);

    ucc_datatype_t ucc_sdt = MPIDI_COMMON_UCC_DTYPE_NULL;
    ucc_datatype_t ucc_rdt = mpidi_mpi_dtype_to_ucc_dtype(rdtype);

    uint64_t flags = 0;

    if (!is_inplace) {
        ucc_sdt = mpidi_mpi_dtype_to_ucc_dtype(sdtype);
    }

    if ((ucc_sdt == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED) ||
        (ucc_rdt == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED)) {
        MPIDI_COMMON_UCC_VERBOSE_DTYPE_UNSUPPORTED(allgatherv);
        goto fallback;
    }

    if (is_inplace) {
        flags |= UCC_COLL_ARGS_FLAG_IN_PLACE;
    }

    if (is_large_counts) {
        flags |= UCC_COLL_ARGS_FLAG_COUNT_64BIT;
        flags |= UCC_COLL_ARGS_FLAG_DISPLACEMENTS_64BIT;
    }

    ucc_coll_args_t coll = {
        .mask = flags ? UCC_COLL_ARGS_FIELD_FLAGS : 0,
        .flags = flags,
        .coll_type = UCC_COLL_TYPE_ALLGATHERV,
        .src.info = {
                     .buffer = (void *) sbuf,
                     .count = scount,
                     .datatype = ucc_sdt,
                     .mem_type = UCC_MEMORY_TYPE_UNKNOWN,
                     }
        ,
        .dst.info_v = {
                       .buffer = rbuf,
                       .counts = (ucc_count_t *) rcounts,
                       .displacements = (ucc_aint_t *) rdispls,
                       .datatype = ucc_rdt,
                       .mem_type = UCC_MEMORY_TYPE_UNKNOWN,
                       }
    };

    if (is_inplace) {
        MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(allgatherv, "comm %p, comm_id %u, comm_size %d"
                                                 ", INPLACE, rdtype %s",
                                                 (void *) comm_ptr, comm_ptr->context_id,
                                                 comm_size, mpidi_ucc_dtype_to_str(ucc_rdt));
    } else {
        MPIDI_COMMON_UCC_VERBOSE_COLLOP_POST_REQ(allgatherv, "comm %p, comm_id %u, comm_size %d"
                                                 ", scount %zu, sdtype %s, rdtype %s",
                                                 (void *) comm_ptr, comm_ptr->context_id,
                                                 comm_size, scount,
                                                 mpidi_ucc_dtype_to_str(ucc_sdt),
                                                 mpidi_ucc_dtype_to_str(ucc_rdt));
    }

    MPIDI_COMMON_UCC_REQ_INIT(coll_req, req, coll, comm_ptr);
    return UCC_OK;
  fallback:
    return UCC_ERR_NOT_SUPPORTED;
}

int MPIDI_common_ucc_allgatherv(const void *sbuf, MPI_Aint scount, MPI_Datatype sdtype,
                                void *rbuf, const MPI_Aint rcounts[],
                                const MPI_Aint rdispls[], MPI_Datatype rdtype, MPIR_Comm * comm_ptr)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    ucc_coll_req_h req;

    MPIDI_COMMON_UCC_CHECK_ENABLED(comm_ptr, allgatherv);

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_TRY_TO_RUN(allgatherv);

    MPIDI_COMMON_UCC_CALL_AND_CHECK(mpidi_ucc_allgatherv_init
                                    (sbuf, scount, sdtype, rbuf, rcounts, rdispls, rdtype,
                                     comm_ptr, &req, NULL));
    MPIDI_COMMON_UCC_POST_AND_CHECK(req);
    MPIDI_COMMON_UCC_WAIT_AND_CHECK(req);

    MPIDI_COMMON_UCC_VERBOSE_COLLOP_DONE_SUCCESS(allgatherv);

    return MPIDI_COMMON_UCC_RETVAL_SUCCESS;

  fallback:
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_FALLBACK(allgatherv);
    return MPIDI_COMMON_UCC_RETVAL_FALLBACK;
  disabled:
    MPIDI_COMMON_UCC_VERBOSE_COLLOP_DISABLED(allgatherv);
    goto fallback;
}
