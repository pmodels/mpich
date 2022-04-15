/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_AM_FALLBACK_RECV_H_INCLUDED
#define SHM_AM_FALLBACK_RECV_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_imrecv(void *buf,
                                                  MPI_Aint count, MPI_Datatype datatype,
                                                  MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    int vci = MPIDI_Request_get_vci(message);
#endif
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDIG_mpi_imrecv(buf, count, datatype, message);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_irecv(void *buf,
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype,
                                                 int rank,
                                                 int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    mpi_errno = MPIDIG_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                 0, request, 1, NULL);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_cancel_recv(MPIR_Request * rreq)
{
    return MPIDIG_mpi_cancel_recv(rreq);
}

#endif /* SHM_AM_FALLBACK_RECV_H_INCLUDED */
