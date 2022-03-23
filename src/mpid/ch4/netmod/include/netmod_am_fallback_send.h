/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef NETMOD_AM_FALLBACK_SEND_H_INCLUDED
#define NETMOD_AM_FALLBACK_SEND_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int attr,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    mpi_errno = MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, 0, 0,
                                 request);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_issend(const void *buf,
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype,
                                                 int rank,
                                                 int tag,
                                                 MPIR_Comm * comm, int attr,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    mpi_errno = MPIDIG_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, addr, 0, 0,
                                  request);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_isend_coll(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int attr,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    mpi_errno = MPIDIG_isend_coll(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  0, 0, request, errflag);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    return MPIDIG_mpi_cancel_send(sreq);
}

#endif /* NETMOD_AM_FALLBACK_SEND_H_INCLUDED */
