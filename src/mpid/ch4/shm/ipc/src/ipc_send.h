/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_SEND_H_INCLUDED
#define IPC_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "shm_control.h"
#include "ipc_pre.h"
#include "ipc_impl.h"
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
#include "../xpmem/xpmem_send.h"
#endif

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE
      category    : CH4
      type        : int
      default     : 4096
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If a send message size is greater than or equal to MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE (in
        bytes), then enable XPMEM-based single copy protocol for intranode communication. The
        environment variable is valid only when then XPMEM shmmod is enabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

typedef enum MPIDI_IPC_type {
    IPC__XPMEM,
} MPIDI_IPC_type_t;

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_lmt_isend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                 MPIDI_IPC_type_t ipc_type)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    MPI_Aint true_lb;
    bool is_contig ATTRIBUTE((unused)) = 0;
    MPIR_Request *sreq = NULL;
    MPIDI_SHM_ctrl_hdr_t ctrl_hdr;
    MPIDI_SHM_ctrl_ipc_send_lmt_rts_t *slmt_req_hdr = &ctrl_hdr.ipc_slmt_rts;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_LMT_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_LMT_ISEND);

    /* Create send request */
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
    MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;
    MPIDIG_REQUEST(sreq, buffer) = (void *) buf;
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, rank) = rank;
    MPIDIG_REQUEST(sreq, count) = count;
    MPIDIG_REQUEST(sreq, context_id) = comm->context_id + context_offset;

    MPIDI_Datatype_check_contig_size_lb(datatype, count, is_contig, data_sz, true_lb);
    MPIR_Assert(is_contig && data_sz > 0);

    slmt_req_hdr->src_lrank = MPIR_Process.local_rank;
    slmt_req_hdr->src_offset = (uint64_t) buf + true_lb;
    slmt_req_hdr->data_sz = data_sz;
    slmt_req_hdr->sreq_ptr = (uint64_t) sreq;

    /* TODO: move XPMEM specific code into submodule */
    MPIDI_IPC_XPMEM_REQUEST(sreq, counter_ptr) = NULL;

    /* message matching info */
    slmt_req_hdr->src_rank = comm->rank;
    slmt_req_hdr->tag = tag;
    slmt_req_hdr->context_id = comm->context_id + context_offset;

    IPC_TRACE("lmt_isend: shm ctrl_id %d, src_offset 0x%lx, data_sz 0x%lx, sreq_ptr 0x%lx, "
              "src_lrank %d, match info[dest %d, src_rank %d, tag %d, context_id 0x%x]\n",
              MPIDI_SHM_IPC_SEND_LMT_RTS, slmt_req_hdr->src_offset,
              slmt_req_hdr->data_sz, slmt_req_hdr->sreq_ptr, slmt_req_hdr->src_lrank,
              rank, slmt_req_hdr->src_rank, slmt_req_hdr->tag, slmt_req_hdr->context_id);

    mpi_errno = MPIDI_SHM_do_ctrl_send(rank, comm, MPIDI_SHM_IPC_SEND_LMT_RTS, &ctrl_hdr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_LMT_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_mpi_isend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_MPI_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_MPI_ISEND);

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    bool dt_contig;
    size_t data_sz;

    MPIDI_Datatype_check_contig_size(datatype, count, dt_contig, data_sz);

    if (MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE > -1 &&
        dt_contig && data_sz >= MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE && rank != comm->rank) {
        /* SHM only issues contig large message through XPMEM.
         * TODO: support noncontig send message */
        mpi_errno = MPIDI_IPC_lmt_isend(buf, count, datatype, rank, tag, comm,
                                        context_offset, addr, request, IPC__XPMEM);
        MPIR_ERR_CHECK(mpi_errno);

        goto fn_exit;
    }
#endif /* end of MPIDI_CH4_SHM_ENABLE_XPMEM */

    mpi_errno = MPIDI_POSIX_mpi_isend(buf, count, datatype, rank, tag, comm,
                                      context_offset, addr, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_MPI_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* IPC_SEND_H_INCLUDED */
