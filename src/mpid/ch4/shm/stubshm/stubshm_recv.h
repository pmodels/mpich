/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef STUBSHM_RECV_H_INCLUDED
#define STUBSHM_RECV_H_INCLUDED

#include "stubshm_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_recv(void *buf,
                                                    MPI_Aint count,
                                                    MPI_Datatype datatype,
                                                    int rank,
                                                    int tag,
                                                    MPIR_Comm * comm,
                                                    int context_offset, MPI_Status * status,
                                                    MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_RECV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_RECV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_imrecv(void *buf,
                                                      MPI_Aint count,
                                                      MPI_Datatype datatype,
                                                      MPIR_Request * message, MPIR_Request ** rreqp)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IMRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IMRECV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IMRECV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_irecv(void *buf,
                                                     MPI_Aint count,
                                                     MPI_Datatype datatype,
                                                     int rank,
                                                     int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IRECV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IRECV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_cancel_recv(MPIR_Request * rreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_CANCEL_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_CANCEL_RECV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_CANCEL_RECV);
    return MPI_SUCCESS;
}

#endif /* STUBSHM_RECV_H_INCLUDED */
