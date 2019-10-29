/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PIP_SEND_H_INCLUDED
#define PIP_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "shm_control.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_PIP_lmt_rts_isend(const void *buf, MPI_Aint count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIDI_av_entry_t * addr,
                                                     MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    size_t data_sz;
    MPI_Aint true_lb;
    bool is_contig;
    MPIDI_SHM_ctrl_hdr_t ctrl_hdr;
    MPIDI_SHM_ctrl_pip_send_lmt_rts_t *slmt_rts_hdr = &ctrl_hdr.pip_slmt_rts;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PIP_LMT_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PIP_LMT_ISEND);

    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
    MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;

    MPIDI_Datatype_check_contig_size_lb(datatype, count, is_contig, data_sz, true_lb);

    MPIR_Assert(is_contig && data_sz > 0);

    /* XPMEM internal info */
    slmt_rts_hdr->src_offset = (uint64_t) buf + true_lb;
    slmt_rts_hdr->data_sz = data_sz;
    slmt_rts_hdr->sreq_ptr = (uint64_t) sreq;
    slmt_rts_hdr->src_lrank = MPIDI_PIP_global.local_rank;

    /* message matching info */
    slmt_rts_hdr->src_rank = comm->rank;
    slmt_rts_hdr->tag = tag;
    slmt_rts_hdr->context_id = comm->context_id + context_offset;

    PIP_TRACE("pip_lmt_isend: shm ctrl_id %d, src_offset 0x%lx, data_sz 0x%lx, sreq_ptr 0x%lx, "
              "src_lrank %d, match info[dest %d, src_rank %d, tag %d, context_id 0x%x]\n",
              MPIDI_SHM_PIP_SEND_LMT_RTS, slmt_rts_hdr->src_offset,
              slmt_rts_hdr->data_sz, slmt_rts_hdr->sreq_ptr, slmt_rts_hdr->src_lrank,
              rank, slmt_rts_hdr->src_rank, slmt_rts_hdr->tag, slmt_rts_hdr->context_id);

    mpi_errno = MPIDI_SHM_do_ctrl_send(rank, comm, MPIDI_SHM_PIP_SEND_LMT_RTS, &ctrl_hdr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PIP_LMT_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_PIP_lmt_cts_isend(const void *buf, MPI_Aint count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIDI_av_entry_t * addr,
                                                     MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PIP_LMT_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PIP_LMT_ISEND);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PIP_LMT_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* PIP_SEND_H_INCLUDED */
