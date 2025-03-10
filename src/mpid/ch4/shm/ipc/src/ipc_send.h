/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_SEND_H_INCLUDED
#define IPC_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "mpidimpl.h"
#include "ipc_types.h"
#include "../xpmem/xpmem_post.h"
#include "../gpu/gpu_post.h"
#include "ipc_p2p.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_IPCI_try_lmt_isend(const void *buf, MPI_Aint count,
                                                      MPI_Datatype datatype, int rank, int tag,
                                                      MPIR_Comm * comm, int attr,
                                                      MPIDI_av_entry_t * addr,
                                                      MPIR_Request ** request, bool * done)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int errflag = MPIR_PT2PT_ATTR_GET_ERRFLAG(attr);
    int vci_src, vci_dst;
    /* note: MPIDI_POSIX_SEND_VSIS defined in posix_send.h */
    MPIDI_POSIX_SEND_VSIS(vci_src, vci_dst);

    MPIDI_POSIX_THREAD_CS_ENTER_VCI(vci_src);

    bool do_ipc = false;
    MPIDI_IPCI_ipc_attr_t ipc_attr;
#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    if (!do_ipc) {
        mpi_errno = MPIDI_GPU_get_ipc_attr(buf, count, datatype, rank, comm, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
        if (ipc_attr.ipc_type == MPIDI_IPCI_TYPE__SKIP) {
            /* GPU IPC is not supported but it is still a device memory,
             * we can't do shared memory IPC either, so skip to fn_exit. */
            goto fn_exit;
        }
        if (ipc_attr.ipc_type != MPIDI_IPCI_TYPE__NONE) {
            do_ipc = true;
        }
    }
#endif
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    if (!do_ipc) {
        mpi_errno = MPIDI_XPMEM_get_ipc_attr(buf, count, datatype, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
        do_ipc = (ipc_attr.ipc_type != MPIDI_IPCI_TYPE__NONE);
    }
#endif
#ifdef MPIDI_CH4_SHM_ENABLE_CMA
    if (!do_ipc) {
        mpi_errno = MPIDI_CMA_get_ipc_attr(buf, count, datatype, &ipc_attr);
        MPIR_ERR_CHECK(mpi_errno);
        do_ipc = (ipc_attr.ipc_type != MPIDI_IPCI_TYPE__NONE);
    }
#endif

    if (!do_ipc) {
        goto fn_exit;
    }

    int dt_contig;
    MPIR_Datatype_is_contig(datatype, &dt_contig);
    if (!dt_contig) {
        int flattened_sz;
        void *flattened_dt;
        MPIR_Datatype_get_flattened(datatype, &flattened_dt, &flattened_sz);
        if (sizeof(MPIDI_IPC_rts_t) + flattened_sz > MPIDI_POSIX_am_hdr_max_sz()) {
            do_ipc = false;
        }
    }

    if (do_ipc) {
        mpi_errno = MPIDI_IPCI_send_lmt(buf, count, datatype,
                                        rank, tag, comm, context_offset, addr, ipc_attr,
                                        vci_src, vci_dst, request, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        *done = true;
    }
  fn_exit:
    MPIDI_POSIX_THREAD_CS_EXIT_VCI(vci_src);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_mpi_isend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int attr,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    bool done = false;
    mpi_errno = MPIDI_IPCI_try_lmt_isend(buf, count, datatype, rank, tag, comm,
                                         attr, addr, request, &done);
    MPIR_ERR_CHECK(mpi_errno);

    if (!done) {
        mpi_errno = MPIDI_POSIX_mpi_isend(buf, count, datatype, rank, tag, comm,
                                          attr, addr, request);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* IPC_SEND_H_INCLUDED */
