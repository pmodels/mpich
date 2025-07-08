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
    int coll_attr = MPIR_PT2PT_ATTR_GET_ERRFLAG(attr);
    int vci_src, vci_dst;
    /* note: MPIDI_POSIX_SEND_VSIS defined in posix_send.h */
    MPIDI_POSIX_SEND_VSIS(vci_src, vci_dst);

    MPIDI_POSIX_THREAD_CS_ENTER_VCI(vci_src);

    MPIDI_IPCI_ipc_attr_t ipc_attr;
    mpi_errno = MPIDI_IPCI_get_ipc_attr(buf, count, datatype, rank, comm, sizeof(MPIDIG_hdr_t),
                                        &ipc_attr);

    if (ipc_attr.ipc_type != MPIDI_IPCI_TYPE__NONE) {

        mpi_errno = MPIDI_IPCI_send_lmt(buf, count, datatype,
                                        rank, tag, comm, context_offset, addr, &ipc_attr,
                                        vci_src, vci_dst, request, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
        *done = true;
    } else {
        MPI_Aint data_sz;
        MPIDI_Datatype_check_size(datatype, count, data_sz);
        if ((sizeof(MPIDIG_hdr_t) + data_sz) > MPIDI_POSIX_am_eager_limit()) {
            mpi_errno = MPIDI_IPCI_send_rndv(buf, count, datatype, rank, tag, comm, context_offset,
                                             addr, vci_src, vci_dst, request, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
            *done = true;
        }
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
