/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_AM_H_INCLUDED
#define SHM_AM_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"
#include "../ipc/src/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_send_hdr(int rank, MPIR_Comm * comm,
                                                   int handler_id,
                                                   const void *am_hdr, MPI_Aint am_hdr_sz,
                                                   int src_vci, int dst_vci)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_am_send_hdr(rank, comm, handler_id, am_hdr, am_hdr_sz, src_vci, dst_vci);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_isend(int rank, MPIR_Comm * comm, int handler_id,
                                                const void *am_hdr, MPI_Aint am_hdr_sz,
                                                const void *data, MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int src_vci, int dst_vci, MPIR_Request * sreq)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_am_isend(rank, comm, handler_id, am_hdr,
                               am_hdr_sz, data, count, datatype, src_vci, dst_vci, sreq);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_send_hdr_reply(MPIR_Comm * comm,
                                                         int src_rank, int handler_id,
                                                         const void *am_hdr, MPI_Aint am_hdr_sz,
                                                         int src_vci, int dst_vci)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_am_send_hdr_reply(comm, src_rank, handler_id, am_hdr, am_hdr_sz,
                                        src_vci, dst_vci);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_isend_reply(MPIR_Comm * comm,
                                                      int src_rank, int handler_id,
                                                      const void *am_hdr, MPI_Aint am_hdr_sz,
                                                      const void *data, MPI_Aint count,
                                                      MPI_Datatype datatype,
                                                      int src_vci, int dst_vci, MPIR_Request * sreq)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_am_isend_reply(comm, src_rank, handler_id, am_hdr, am_hdr_sz,
                                     data, count, datatype, src_vci, dst_vci, sreq);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_SHM_am_hdr_max_sz(void)
{
    return MPIDI_POSIX_am_hdr_max_sz();
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_SHM_am_eager_limit(void)
{
    return MPIDI_POSIX_am_eager_limit();
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_SHM_am_eager_buf_limit(void)
{
    return MPIDI_POSIX_am_eager_buf_limit();
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHM_am_request_init(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    MPIDI_SHM_REQUEST(req, status) = 0;
    MPIDI_POSIX_am_request_init(req);

    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHM_am_request_finalize(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    MPIDI_POSIX_am_request_finalize(req);

    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_SHM_am_check_eager(MPI_Aint am_hdr_sz, MPI_Aint data_sz,
                                                       const void *data, MPI_Aint count,
                                                       MPI_Datatype datatype, MPIR_Request * sreq)
{
    /* TODO: add checking for IPC transmission */
    return (am_hdr_sz + data_sz) <= MPIDI_POSIX_am_eager_limit();
}

MPL_STATIC_INLINE_PREFIX MPIDIG_recv_data_copy_cb MPIDI_SHM_am_get_data_copy_cb(uint32_t attr)
{
    return NULL;
}

#endif /* SHM_AM_H_INCLUDED */
