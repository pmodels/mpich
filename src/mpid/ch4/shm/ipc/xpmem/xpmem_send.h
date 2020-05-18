/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_SEND_H_INCLUDED
#define XPMEM_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "shm_control.h"
#include "xpmem_pre.h"
#include "xpmem_impl.h"
#include "ipcg_p2p.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_xpmem_mpi_isend(const void *buf, MPI_Aint count,
                                                       MPI_Datatype datatype, int rank, int tag,
                                                       MPIR_Comm * comm, int context_offset,
                                                       MPIDI_av_entry_t * addr,
                                                       MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_XPMEM_MPI_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_XPMEM_MPI_ISEND);

    mpi_errno = MPIDI_IPCG_send_contig_lmt(buf, count, datatype, rank, tag, comm,
                                           context_offset, addr, MPIDI_SHM_IPC_TYPE__XPMEM,
                                           request);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_IPC_XPMEM_REQUEST(*request, counter_ptr) = NULL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_XPMEM_MPI_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* XPMEM_SEND_H_INCLUDED */
