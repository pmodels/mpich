/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_SEND_H_INCLUDED
#define IPC_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "mpidimpl.h"
#include "shm_control.h"
#include "ipc_pre.h"
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
        mpi_errno = MPIDI_IPC_xpmem_mpi_isend(buf, count, datatype, rank, tag, comm,
                                              context_offset, addr, request);
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
