/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef XPMEM_SEND_H_INCLUDED
#define XPMEM_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "shm_control.h"
#include "xpmem_pre.h"
#include "xpmem_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_lmt_isend(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank, int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    size_t data_sz;
    MPI_Aint true_lb;
    bool is_contig ATTRIBUTE((unused)) = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_LMT_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_LMT_ISEND);

    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
    MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;
    MPIDIG_REQUEST(sreq, buffer) = (void *) buf;
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, rank) = rank;
    MPIDIG_REQUEST(sreq, count) = count;
    MPIDIG_REQUEST(sreq, context_id) = comm->context_id + context_offset;
    MPIDI_XPMEM_REQUEST(sreq, counter_ptr) = NULL;

    MPIDI_Datatype_check_contig_size_lb(datatype, count, is_contig, data_sz, true_lb);

    MPIR_Assert(is_contig && data_sz > 0);

    mpi_errno = MPIDI_XPMEM_send_rts(rank, comm, context_offset,
                                     (char *) buf + true_lb, data_sz, tag, sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_LMT_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* XPMEM_SEND_H_INCLUDED */
