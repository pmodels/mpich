/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef SHM_P2P_H_INCLUDED
#define SHM_P2P_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_send(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_SEND);

    ret =
        MPIDI_POSIX_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_SEND);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ssend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_SSEND);

    ret =
        MPIDI_POSIX_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_SSEND);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_send_init(const void *buf, int count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_SEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_SEND_INIT);

    ret = MPIDI_POSIX_mpi_send_init(buf, count, datatype, rank, tag, comm, context_offset, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_SEND_INIT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ssend_init(const void *buf, int count,
                                                      MPI_Datatype datatype, int rank, int tag,
                                                      MPIR_Comm * comm, int context_offset,
                                                      MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_SSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_SSEND_INIT);

    ret = MPIDI_POSIX_mpi_ssend_init(buf, count, datatype, rank, tag, comm,
                                     context_offset, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_SSEND_INIT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rsend_init(const void *buf, int count,
                                                      MPI_Datatype datatype, int rank, int tag,
                                                      MPIR_Comm * comm, int context_offset,
                                                      MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_RSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_RSEND_INIT);

    ret = MPIDI_POSIX_mpi_rsend_init(buf, count, datatype, rank, tag, comm,
                                     context_offset, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_RSEND_INIT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_bsend_init(const void *buf, int count,
                                                      MPI_Datatype datatype, int rank, int tag,
                                                      MPIR_Comm * comm, int context_offset,
                                                      MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_BSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_BSEND_INIT);

    ret = MPIDI_POSIX_mpi_bsend_init(buf, count, datatype, rank, tag, comm,
                                     context_offset, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_BSEND_INIT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_isend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_ISEND);

    ret =
        MPIDI_POSIX_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_ISEND);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_issend(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_ISSEND);

    ret =
        MPIDI_POSIX_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                               request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_ISSEND);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_cancel_send(MPIR_Request * sreq)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_CANCEL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_CANCEL_SEND);

    ret = MPIDI_POSIX_mpi_cancel_send(sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_CANCEL_SEND);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_recv_init(void *buf, int count, MPI_Datatype datatype,
                                                     int rank, int tag, MPIR_Comm * comm,
                                                     int context_offset, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_RECV_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_RECV_INIT);

    ret = MPIDI_POSIX_mpi_recv_init(buf, count, datatype, rank, tag, comm, context_offset, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_RECV_INIT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_recv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset, MPI_Status * status,
                                                MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_RECV);

    ret = MPIDI_POSIX_mpi_recv(buf, count, datatype, rank, tag, comm, context_offset,
                               status, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_RECV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_irecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                 int rank, int tag, MPIR_Comm * comm,
                                                 int context_offset, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_IRECV);

    ret = MPIDI_POSIX_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_IRECV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_imrecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                  MPIR_Request * message, MPIR_Request ** rreqp)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_IMRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_IMRECV);

    ret = MPIDI_POSIX_mpi_imrecv(buf, count, datatype, message, rreqp);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_IMRECV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_cancel_recv(MPIR_Request * rreq)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_CANCEL_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_CANCEL_RECV);

    ret = MPIDI_POSIX_mpi_cancel_recv(rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_CANCEL_RECV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_improbe(int source, int tag, MPIR_Comm * comm,
                                                   int context_offset, int *flag,
                                                   MPIR_Request ** message, MPI_Status * status)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_IMPROBE);

    ret = MPIDI_POSIX_mpi_improbe(source, tag, comm, context_offset, flag, message, status);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_IMPROBE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iprobe(int source, int tag, MPIR_Comm * comm,
                                                  int context_offset, int *flag,
                                                  MPI_Status * status)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_IPROBE);

    ret = MPIDI_POSIX_mpi_iprobe(source, tag, comm, context_offset, flag, status);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_IPROBE);
    return ret;
}

#endif /* SHM_P2P_H_INCLUDED */
