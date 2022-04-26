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
    MPIR_Errflag_t errflag = MPIR_PT2PT_ATTR_GET_ERRFLAG(attr);
    bool syncflag = MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr);

    int vni_src, vni_dst;
    vni_src = 0;
    vni_dst = 0;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni_src).lock);
    mpi_errno = MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                 vni_src, vni_dst, request, syncflag, errflag);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni_src).lock);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    return MPIDIG_mpi_cancel_send(sreq);
}

#endif /* NETMOD_AM_FALLBACK_SEND_H_INCLUDED */
