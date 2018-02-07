/* -*- Mode: C ; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_COLL_IMPL_H_INCLUDED
#define OFI_COLL_IMPL_H_INCLUDED

#include "ofi_coll_params.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_Barrier__recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline
    int MPIDI_OFI_Barrier__recursive_doubling(MPIR_Comm * comm_ptr,
                                              MPIR_Errflag_t * errflag,
                                              MPIDI_OFI_coll_algo_container_t *
                                              params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Barrier_intra_recursive_doubling(comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_Bcast__binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline
    int MPIDI_OFI_Bcast__binomial(void *buffer,
                                  int count,
                                  MPI_Datatype datatype,
                                  int root,
                                  MPIR_Comm * comm_ptr,
                                  MPIR_Errflag_t * errflag,
                                  MPIDI_OFI_coll_algo_container_t *
                                  params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_Bcast__scatter_recursive_doubling_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline
    int MPIDI_OFI_Bcast__scatter_recursive_doubling_allgather(void *buffer,
                                                              int count,
                                                              MPI_Datatype datatype,
                                                              int root,
                                                              MPIR_Comm * comm_ptr,
                                                              MPIR_Errflag_t * errflag,
                                                              MPIDI_OFI_coll_algo_container_t *
                                                              params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_Bcast__scatter_ring_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline
    int MPIDI_OFI_Bcast__scatter_ring_allgather(void *buffer,
                                                int count,
                                                MPI_Datatype datatype,
                                                int root,
                                                MPIR_Comm * comm_ptr,
                                                MPIR_Errflag_t * errflag,
                                                MPIDI_OFI_coll_algo_container_t *
                                                params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_allreduce__recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline
    int MPIDI_OFI_allreduce__recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                MPIDI_OFI_coll_algo_container_t *
                                                params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_allreduce__reduce_scatter_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline
    int MPIDI_OFI_allreduce__reduce_scatter_allgather(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm_ptr,
                                                      MPIR_Errflag_t * errflag,
                                                      MPIDI_OFI_coll_algo_container_t *
                                                      params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Allreduce_intra_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op,
                                                      comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_reduce__reduce_scatter_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline
    int MPIDI_OFI_reduce__reduce_scatter_gather(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op, int root,
                                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                MPIDI_OFI_coll_algo_container_t *
                                                params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Reduce_intra_reduce_scatter_gather(sendbuf, recvbuf, count, datatype, op, root,
                                                comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_reduce__binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline
    int MPIDI_OFI_reduce__binomial(const void *sendbuf, void *recvbuf, int count,
                                   MPI_Datatype datatype, MPI_Op op, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                   MPIDI_OFI_coll_algo_container_t *
                                   params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Reduce_intra_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    return mpi_errno;
}

#endif /* OFI_COLL_IMPL_H_INCLUDED */
