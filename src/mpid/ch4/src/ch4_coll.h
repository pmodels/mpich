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
#ifndef CH4_COLL_H_INCLUDED
#define CH4_COLL_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"

#ifdef MPIDI_BUILD_CH4_COLL
#include "../generic/coll/include/ch4_coll_pre.h"
#include "../generic/coll/include/ch4_coll_post.h"
#endif

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_USE_BCAST
      category    : COLLECTIVE
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Controls bcast algorithm:
        0 - MPIR_bcast
        1 - KNOMIAL_Bcast
        2 - KARY_Bcast
    - name        : MPIR_CVAR_USE_REDUCE
      category    : COLLECTIVE
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Controls reduce algorithm:
        0 - MPIR_reduce
        1 - KNOMIAL_reduce
        2 - KARY_reduce
    - name        : MPIR_CVAR_USE_ALLREDUCE
      category    : COLLECTIVE
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Controls allreduce algorithm:
        0 - MPIR_allreduce
        1 - KNOMIAL_allreduce
        2 - KARY_allreduce
        3 - RECEXCH_allreduce
    - name        : MPIR_CVAR_USE_BARRIER
      category    : COLLECTIVE
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Controls barrier algorithm:
        0 - MPIR_barrier
        1 - DISSEM_barrier
    - name        : MPIR_CVAR_USE_IBARRIER
      category    : COLLECTIVE
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Controls ibarrier algorithm:
        0 - MPIR_ibarrier
        1 - DISSEM_ibarrier
    - name        : MPIR_CVAR_USE_ALLTOALL
      category    : COLLECTIVE
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Controls alltoall algorithm:
        0 - MPIR_alltoall
        1 - DISSEM_alltoall
    - name        : MPIR_CVAR_USE_IALLTOALL
      category    : COLLECTIVE
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Controls ialltoall algorithm:
        0 - MPIR_ialltoall
        1 - DISSEM_ialltoall




=== END_MPI_T_CVAR_INFO_BLOCK ===
*/
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_cycle_algorithm(MPIR_Comm *comm_ptr, int pick[], int num) {
    if(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        return 0;
    int idx;
    MPIDI_COLL_COMM(comm_ptr)->issued_collectives++;
    idx = MPIDI_COLL_COMM(comm_ptr)->issued_collectives%num;
    return pick[idx];
}

MPL_STATIC_INLINE_PREFIX int MPID_Barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BARRIER);

#ifdef MPIDI_BUILD_CH4_COLL
    int valid_coll[] = {0,1,2,3};
    int use_coll = (MPIR_CVAR_USE_BARRIER < 0)?
                        MPIDI_CH4_cycle_algorithm(comm, valid_coll, 1) :
                        MPIR_CVAR_USE_BARRIER;
    switch(use_coll) {
    case 0:
#endif
        ret = MPIDI_NM_mpi_barrier(comm, errflag);
#ifdef MPIDI_BUILD_CH4_COLL
        break;
    case 1:
        ret = MPIDI_COLL_MPICH_DISSEM_barrier( &(MPIDI_COLL_COMM(comm)->mpich_dissem),
                        errflag);
        break;
    case 2:
        ret = MPIDI_COLL_MPICH_KARY_barrier(&(MPIDI_COLL_COMM(comm)->mpich_kary), errflag, 2);
        break;
    case 3:
        ret = MPIDI_COLL_MPICH_KNOMIAL_barrier(&(MPIDI_COLL_COMM(comm)->mpich_knomial), errflag, 2);
        break;
    }
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_BARRIER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Bcast(void *buffer, int count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BCAST);

#ifdef MPIDI_BUILD_CH4_COLL
    int valid_coll[] = {1,2};
    int use_coll = (MPIR_CVAR_USE_BCAST < 0) ? MPIDI_CH4_cycle_algorithm(comm, valid_coll, 2) : MPIR_CVAR_USE_BCAST;

    MPIR_Datatype *dt_ptr;
    MPID_Datatype_get_ptr(datatype, dt_ptr);

    switch(use_coll) {
    case 0:
#endif
        ret = MPIDI_NM_mpi_bcast(buffer, count, datatype, root, comm, errflag);

#ifdef MPIDI_BUILD_CH4_COLL
        break;
    case 1:
        ret = MPIDI_COLL_MPICH_KNOMIAL_bcast(buffer, count,
                &(MPIDI_COLL_DT(dt_ptr)->mpich_knomial),
                root, &(MPIDI_COLL_COMM(comm)->mpich_knomial), errflag, 2);
        break;
    case 2:
        ret = MPIDI_COLL_MPICH_KARY_bcast(buffer, count,
                &(MPIDI_COLL_DT(dt_ptr)->mpich_kary),
                root, &(MPIDI_COLL_COMM(comm)->mpich_kary), errflag, 2);
        break;
    }
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_BCAST);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allreduce(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLREDUCE);

#ifdef MPIDI_BUILD_CH4_COLL
    MPIR_Datatype *dt_ptr;
    MPID_Datatype_get_ptr(datatype, dt_ptr);
    MPIR_Op *op_p;
    MPIDI_COLL_Op_get_ptr(op, &op_p);

    int valid_coll[] = {1,2,3,4};
    int use_coll = (MPIR_CVAR_USE_ALLREDUCE < 0) ? MPIDI_CH4_cycle_algorithm(comm, valid_coll, 3) : MPIR_CVAR_USE_ALLREDUCE;

    switch (use_coll){
    case 0:
#endif
        ret = MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);

#ifdef MPIDI_BUILD_CH4_COLL
        break;
    case 1:
        ret = MPIDI_COLL_MPICH_KNOMIAL_allreduce(sendbuf, recvbuf, count,
                                        &(MPIDI_COLL_DT(dt_ptr)->mpich_knomial),
                                        &(MPIDI_COLL_OP(op_p)->mpich_knomial),
                                        &(MPIDI_COLL_COMM(comm)->mpich_knomial), errflag, 2);
        break;
    case 2:
        ret = MPIDI_COLL_MPICH_KARY_allreduce(sendbuf, recvbuf, count,
                                        &(MPIDI_COLL_DT(dt_ptr)->mpich_kary),
                                        &(MPIDI_COLL_OP(op_p)->mpich_kary),
                                        &(MPIDI_COLL_COMM(comm)->mpich_kary), errflag, 2);
        break;
    case 3:
        ret = MPIDI_COLL_MPICH_RECEXCH_allreduce(sendbuf, recvbuf, count,
                                        &(MPIDI_COLL_DT(dt_ptr)->mpich_recexch),
                                        &(MPIDI_COLL_OP(op_p)->mpich_recexch),
                                        &(MPIDI_COLL_COMM(comm)->mpich_recexch), errflag);
        break;
    case 4:
        ret = MPIDI_COLL_MPICH_DISSEM_allreduce(sendbuf, recvbuf, count,
                                        &(MPIDI_COLL_DT(dt_ptr)->mpich_dissem),
                                        &(MPIDI_COLL_OP(op_p)->mpich_dissem),
                                        &(MPIDI_COLL_COMM(comm)->mpich_dissem), errflag);
        break;
    }
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allgather(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLGATHER);

    ret = MPIDI_NM_mpi_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allgatherv(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLGATHERV);

    ret = MPIDI_NM_mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcounts, displs, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatter(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCATTER);

    ret = MPIDI_NM_mpi_scatter(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatterv(const void *sendbuf, const int *sendcounts,
                                            const int *displs, MPI_Datatype sendtype,
                                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                            int root, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCATTERV);

    ret = MPIDI_NM_mpi_scatterv(sendbuf, sendcounts, displs, sendtype,
                                recvbuf, recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCATTERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GATHER);

    ret = MPIDI_NM_mpi_gather(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GATHERV);

    ret = MPIDI_NM_mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf,
                               recvcounts, displs, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoall(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALL);

#ifdef MPIDI_BUILD_CH4_COLL
    MPIR_Datatype *send_dt_ptr;
    MPID_Datatype_get_ptr(sendtype, send_dt_ptr);
    MPIR_Datatype *recv_dt_ptr;
    MPID_Datatype_get_ptr(recvtype, recv_dt_ptr);

    int valid_coll[] = {1};
    int use_coll = (MPIR_CVAR_USE_ALLTOALL < 0) ? MPIDI_CH4_cycle_algorithm(comm, valid_coll, 1) : MPIR_CVAR_USE_ALLTOALL;

    switch (0){
    case 0:
#endif
        ret = MPIDI_NM_mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm, errflag);
#ifdef MPIDI_BUILD_CH4_COLL
        break;
    case 1:
        ret = MPIDI_COLL_MPICH_DISSEM_alltoall(sendbuf, sendcount,
                                        &(MPIDI_COLL_DT(send_dt_ptr)->mpich_dissem),
                                        recvbuf, recvcount,
                                        &(MPIDI_COLL_DT(recv_dt_ptr)->mpich_dissem),
                                        &(MPIDI_COLL_COMM(comm)->mpich_dissem), errflag);
        break;
    }
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallv(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, MPI_Datatype sendtype,
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALLV);

    ret = MPIDI_NM_mpi_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                 recvbuf, recvcounts, rdispls, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallw(const void *sendbuf, const int sendcounts[],
                                             const int sdispls[], const MPI_Datatype sendtypes[],
                                             void *recvbuf, const int recvcounts[],
                                             const int rdispls[], const MPI_Datatype recvtypes[],
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALLW);

    ret = MPIDI_NM_mpi_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                 recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce(const void *sendbuf, void *recvbuf,
                                          int count, MPI_Datatype datatype, MPI_Op op,
                                          int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE);

#ifdef MPIDI_BUILD_CH4_COLL
    MPIR_Datatype *dt_ptr;
    MPID_Datatype_get_ptr(datatype, dt_ptr);
    MPIR_Op *op_p;
    MPIDI_COLL_Op_get_ptr(op, &op_p);

    int valid_coll[] = {1,2};
    int use_coll = (MPIR_CVAR_USE_REDUCE < 0) ? MPIDI_CH4_cycle_algorithm(comm_ptr, valid_coll, 2) : MPIR_CVAR_USE_REDUCE;

    switch(use_coll) {
    case 0:
#endif
        ret = MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

#ifdef MPIDI_BUILD_CH4_COLL
        break;
    case 1:
        ret = MPIDI_COLL_MPICH_KNOMIAL_reduce(sendbuf, recvbuf, count,
                                        &(MPIDI_COLL_DT(dt_ptr)->mpich_knomial),
                                        &(MPIDI_COLL_OP(op_p)->mpich_knomial), root,
                                        &(MPIDI_COLL_COMM(comm_ptr)->mpich_knomial), errflag, 2);
        break;
    case 2:
        ret = MPIDI_COLL_MPICH_KARY_reduce(sendbuf, recvbuf, count,
                                        &(MPIDI_COLL_DT(dt_ptr)->mpich_kary),
                                        &(MPIDI_COLL_OP(op_p)->mpich_kary), root,
                                        &(MPIDI_COLL_COMM(comm_ptr)->mpich_kary), errflag, 2);
        break;
    }
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter(const void *sendbuf, void *recvbuf,
                                                  const int recvcounts[], MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm_ptr,
                                                  MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE_SCATTER);

    ret = MPIDI_NM_mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                      errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                        int recvcount, MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount,
                                            datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scan(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                        MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCAN);

    ret = MPIDI_NM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Exscan(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_EXSCAN);

    ret = MPIDI_NM_mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_EXSCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgather(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_mpi_neighbor_allgather(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const int *recvcounts, const int *displs,
                                                       MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                      const int *sdispls, MPI_Datatype sendtype,
                                                      void *recvbuf, const int *recvcounts,
                                                      const int *rdispls, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                          sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                      const MPI_Aint * sdispls,
                                                      const MPI_Datatype * sendtypes, void *recvbuf,
                                                      const int *recvcounts,
                                                      const MPI_Aint * rdispls,
                                                      const MPI_Datatype * recvtypes,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                          sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoall(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_mpi_neighbor_alltoall(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgather(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       int recvcount, MPI_Datatype recvtype,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_mpi_ineighbor_allgather(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        const int *recvcounts, const int *displs,
                                                        MPI_Datatype recvtype, MPIR_Comm * comm,
                                                        MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                       const int *sdispls, MPI_Datatype sendtype,
                                                       void *recvbuf, const int *recvcounts,
                                                       const int *rdispls, MPI_Datatype recvtype,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                           sendtype, recvbuf, recvcounts, rdispls, recvtype, comm,
                                           req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                       const MPI_Aint * sdispls,
                                                       const MPI_Datatype * sendtypes,
                                                       void *recvbuf, const int *recvcounts,
                                                       const MPI_Aint * rdispls,
                                                       const MPI_Datatype * recvtypes,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                           sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                                           comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibarrier(MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IBARRIER);



    ret = MPIDI_NM_mpi_ibarrier(comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IBARRIER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibcast(void *buffer, int count, MPI_Datatype datatype,
                                          int root, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IBCAST);

    ret = MPIDI_NM_mpi_ibcast(buffer, count, datatype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IBCAST);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgather(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLGATHER);

    ret = MPIDI_NM_mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgatherv(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const int *recvcounts, const int *displs,
                                               MPI_Datatype recvtype, MPIR_Comm * comm,
                                               MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLGATHERV);

    ret = MPIDI_NM_mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                              MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLREDUCE);

    ret = MPIDI_NM_mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoall(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLTOALL);

    ret = MPIDI_NM_mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallv(const void *sendbuf, const int *sendcounts,
                                              const int *sdispls, MPI_Datatype sendtype,
                                              void *recvbuf, const int *recvcounts,
                                              const int *rdispls, MPI_Datatype recvtype,
                                              MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLTOALLV);

    ret = MPIDI_NM_mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                  recvbuf, recvcounts, rdispls, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallw(const void *sendbuf, const int *sendcounts,
                                              const int *sdispls, const MPI_Datatype * sendtypes,
                                              void *recvbuf, const int *recvcounts,
                                              const int *rdispls, const MPI_Datatype * recvtypes,
                                              MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLTOALLW);

    ret = MPIDI_NM_mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                  recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iexscan(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                           MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IEXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IEXSCAN);

    ret = MPIDI_NM_mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IEXSCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igather(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IGATHER);

    ret = MPIDI_NM_mpi_igather(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igatherv(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            const int *recvcounts, const int *displs,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                            MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IGATHERV);

    ret = MPIDI_NM_mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                         int recvcount, MPI_Datatype datatype,
                                                         MPI_Op op, MPIR_Comm * comm,
                                                         MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IREDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IREDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IREDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                   const int *recvcounts, MPI_Datatype datatype,
                                                   MPI_Op op, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IREDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IREDUCE_SCATTER);

    ret = MPIDI_NM_mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IREDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, int root,
                                           MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IREDUCE);

    ret = MPIDI_NM_mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscan(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISCAN);

    ret = MPIDI_NM_mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatter(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                            MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISCATTER);

    ret = MPIDI_NM_mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatterv(const void *sendbuf, const int *sendcounts,
                                             const int *displs, MPI_Datatype sendtype,
                                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                             int root, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISCATTERV);

    ret = MPIDI_NM_mpi_iscatterv(sendbuf, sendcounts, displs, sendtype,
                                 recvbuf, recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISCATTERV);
    return ret;
}

#endif /* CH4_COLL_H_INCLUDED */
