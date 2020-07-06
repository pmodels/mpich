/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_SEND_H_INCLUDED
#define IPC_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "mpidimpl.h"
#include "shm_control.h"
#include "ipc_types.h"
#include "ipc_mem.h"
#include "ipc_p2p.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_try_lmt_isend(const void *buf, MPI_Aint count,
                                                      MPI_Datatype datatype, int rank, int tag,
                                                      MPIR_Comm * comm, int context_offset,
                                                      MPIDI_av_entry_t * addr,
                                                      MPIR_Request ** request, bool * done)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPCI_TRY_LMT_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPCI_TRY_LMT_ISEND);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    bool dt_contig;
    MPI_Aint true_lb;
    uintptr_t data_sz;
    MPIDI_Datatype_check_contig_size_lb(datatype, count, dt_contig, data_sz, true_lb);

    void *vaddr = (char *) buf + true_lb;
    MPIDI_IPCI_ipc_attr_t ipc_attr;
    MPIR_GPU_query_pointer_attr(vaddr, &ipc_attr.gpu_attr);

    if (ipc_attr.gpu_attr.type == MPL_GPU_POINTER_DEV) {
        mpi_errno = MPIDI_GPU_get_ipc_attr(vaddr, rank, comm, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIDI_XPMEM_get_ipc_attr(vaddr, data_sz, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (rank != comm->rank && ipc_attr.ipc_type != MPIDI_IPCI_TYPE__NONE &&
        data_sz >= ipc_attr.threshold.send_lmt_sz) {
        if (dt_contig) {
            mpi_errno = MPIDI_IPCI_send_contig_lmt(buf, count, datatype, data_sz, rank, tag, comm,
                                                   context_offset, addr, ipc_attr, request);
            MPIR_ERR_CHECK(mpi_errno);
            *done = true;
        }
        /* TODO: add flattening datatype protocol for noncontig send. Different
         * threshold may be required to tradeoff the flattening overhead.*/
    }
  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPCI_TRY_LMT_ISEND);
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

    bool done = false;
    mpi_errno = MPIDI_IPCI_try_lmt_isend(buf, count, datatype, rank, tag, comm,
                                         context_offset, addr, request, &done);
    MPIR_ERR_CHECK(mpi_errno);

    if (!done) {
        mpi_errno = MPIDI_POSIX_mpi_isend(buf, count, datatype, rank, tag, comm,
                                          context_offset, addr, request);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_MPI_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* IPC_SEND_H_INCLUDED */
