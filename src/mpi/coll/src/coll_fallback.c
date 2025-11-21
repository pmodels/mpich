/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "coll_impl.h"

int MPIR_Barrier_fallback(MPIR_Comm * comm_ptr, int coll_attr)
{
    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    return MPIR_Barrier_intra_dissemination(comm_ptr, coll_attr);
}

int MPIR_Allgather_fallback(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                            void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                            MPIR_Comm * comm_ptr, int coll_attr)
{
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        return MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm_ptr, coll_attr);
    } else {
        return MPIR_Allgather_inter_local_gather_remote_bcast(sendbuf, sendcount, sendtype,
                                                              recvbuf, recvcount, recvtype,
                                                              comm_ptr, coll_attr);
    }
}

int MPIR_Allgatherv_fallback(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                             MPI_Datatype recvtype, MPIR_Comm * comm_ptr, int coll_attr)
{
    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    return MPIR_Allgatherv_intra_brucks(sendbuf, sendcount, sendtype,
                                        recvbuf, recvcounts, displs, recvtype, comm_ptr, coll_attr);
}

int MPIR_Allreduce_fallback(const void *sendbuf, void *recvbuf,
                            MPI_Aint count, MPI_Datatype datatype,
                            MPI_Op op, MPIR_Comm * comm_ptr, int coll_attr)
{
    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    return MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                   comm_ptr, coll_attr);
}

int MPIR_Bcast_fallback(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                        MPIR_Comm * comm_ptr, int coll_attr)
{
    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    return MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, coll_attr);
}

int MPIR_Gather_fallback(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                         void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                         MPIR_Comm * comm_ptr, int coll_attr)
{
    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    return MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype,
                                      recvbuf, recvcount, recvtype, root, comm_ptr, coll_attr);
}

int MPIR_Reduce_scatter_block_fallback(const void *sendbuf, void *recvbuf, MPI_Aint recvcount,
                                       MPI_Datatype datatype, MPI_Op op,
                                       MPIR_Comm * comm_ptr, int coll_attr)
{
    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);
    MPIR_Assert(MPIR_Op_is_commutative(op));
    return MPIR_Reduce_scatter_block_intra_recursive_halving(sendbuf, recvbuf, recvcount, datatype,
                                                             op, comm_ptr, coll_attr);
}
