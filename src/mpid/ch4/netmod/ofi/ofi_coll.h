/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_COLL_H_INCLUDED
#define OFI_COLL_H_INCLUDED

#include "ofi_impl.h"
#include "ofi_coll_impl.h"

#define MPIDI_OFI_MPI_OP_TO_COLLOP(NS0,NS1,NS2,NS3)                     \
    static inline MPIDI_OFI_COLL_ ##NS0## _ ##NS2## _op_t *             \
    MPIDI_OFI_MPI_OP_TO_COLLOP_ ##NS0## _ ##NS2## _fn (MPI_Op op) {     \
        MPIR_Op       *op_ptr;                                          \
        if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {               \
            unsigned idx = ((op)&0xf)-1;                                \
            extern     MPIDI_OFI_COLL_ ##NS0## _ ##NS2## _op_t          \
                MPIDI_OFI_COLL_ ##NS0## _ ##NS2## _op_table[];          \
            return &MPIDI_OFI_COLL_ ##NS0## _ ##NS2## _op_table[idx];   \
        }                                                               \
        else {                                                          \
            MPIR_Op_get_ptr(op, op_ptr);                                \
            return &MPIDI_OFI_OP(op_ptr).op_ ##NS1## _ ##NS3;           \
        }                                                               \
    }
MPIDI_OFI_MPI_OP_TO_COLLOP(MPICH,mpich,KNOMIAL,knomial);
MPIDI_OFI_MPI_OP_TO_COLLOP(MPICH,mpich,KARY,kary);
MPIDI_OFI_MPI_OP_TO_COLLOP(MPICH,mpich,DISSEM,dissem);
MPIDI_OFI_MPI_OP_TO_COLLOP(MPICH,mpich,RECEXCH,recexch);
MPIDI_OFI_MPI_OP_TO_COLLOP(TRIGGERED,triggered,KNOMIAL,knomial);
MPIDI_OFI_MPI_OP_TO_COLLOP(TRIGGERED,triggered,KARY,kary);
MPIDI_OFI_MPI_OP_TO_COLLOP(TRIGGERED,triggered,DISSEM,dissem);
MPIDI_OFI_MPI_OP_TO_COLLOP(TRIGGERED,triggered,RECEXCH,recexch);
MPIDI_OFI_MPI_OP_TO_COLLOP(STUB,stub,KNOMIAL,knomial);
MPIDI_OFI_MPI_OP_TO_COLLOP(STUB,stub,KARY,kary);
MPIDI_OFI_MPI_OP_TO_COLLOP(STUB,stub,DISSEM,dissem);
MPIDI_OFI_MPI_OP_TO_COLLOP(STUB,stub,RECEXCH,recexch);
MPIDI_OFI_MPI_OP_TO_COLLOP(STUB,stub,STUB,stub);
MPIDI_OFI_MPI_OP_TO_COLLOP(MPICH,mpich,STUB,stub);
MPIDI_OFI_MPI_OP_TO_COLLOP(SHM,shm,GR,gr);

static inline int MPIDI_OFI_cycle_algorithm(MPIR_Comm *comm_ptr, int pick[], int num) {
    if(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        return 0;

#if 0
    int idx;
    MPIDI_OFI_COMM(comm_ptr).issued_collectives++;
    idx = MPIDI_OFI_COMM(comm_ptr).issued_collectives%num;
    return pick[idx];
#else
    return 0;
#endif
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_barrier(MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno, coll_error;
    int valid_colls[] = {1, 2};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm_ptr,valid_colls,2);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_BARRIER);
    *errflag = MPI_SUCCESS;

    if(use_coll == 0)
        mpi_errno = MPIR_Barrier(comm_ptr, errflag);
    else if(use_coll == 1)
        mpi_errno = MPIDI_OFI_COLL_MPICH_KNOMIAL_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_mpich_knomial,
                        &coll_error,
                        2);
    else if(use_coll == 2)
        mpi_errno = MPIDI_OFI_COLL_MPICH_KARY_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_mpich_kary,
                        &coll_error,
                        2);
    else if(use_coll == 3)
        mpi_errno = MPIDI_OFI_COLL_MPICH_DISSEM_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_mpich_dissem,
                        &coll_error);
    else if(use_coll == 4)
        mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KNOMIAL_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_triggered_knomial,
                        &coll_error,
                        2);
    else if(use_coll == 5)
        mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KARY_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_triggered_kary,
                        &coll_error,
                        2);
    else if(use_coll == 6)
        mpi_errno = MPIDI_OFI_COLL_TRIGGERED_DISSEM_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_triggered_dissem,
                        &coll_error);
    else {
        MPIR_Assert(0);
        mpi_errno = MPIDI_OFI_COLL_STUB_KNOMIAL_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_stub_knomial,
                        &coll_error,
                        2);
        mpi_errno = MPIDI_OFI_COLL_STUB_KARY_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_stub_kary,
                        &coll_error,
                        2);
        mpi_errno = MPIDI_OFI_COLL_STUB_DISSEM_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_stub_dissem,
                        &coll_error);
        mpi_errno = MPIDI_OFI_COLL_STUB_STUB_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_stub_stub,
                        &coll_error);
        mpi_errno = MPIDI_OFI_COLL_MPICH_STUB_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_mpich_stub,
                        &coll_error);
        mpi_errno = MPIDI_OFI_COLL_SHM_GR_barrier(
                        &MPIDI_OFI_COMM(comm_ptr).comm_shm_gr,
                        &coll_error);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_BARRIER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                     int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno, coll_error;
    int valid_colls[] = {1, 2};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm_ptr,valid_colls,2);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_BCAST);
    *errflag = MPI_SUCCESS;

    if(use_coll == 0)
        mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm_ptr, errflag);
    else {
        MPIR_Datatype *dt_ptr;
        MPID_Datatype_get_ptr(datatype,dt_ptr);

        if(use_coll == 1)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KNOMIAL_bcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_knomial,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_knomial,
                            &coll_error,
                            2);
        else if(use_coll == 2)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KARY_bcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_kary,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_kary,
                            &coll_error,
                            2);
        else if(use_coll == 3)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KNOMIAL_bcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_knomial,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_knomial,
                            &coll_error,
                            2);
        else if(use_coll == 4)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KARY_bcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_kary,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_kary,
                            &coll_error,
                            2);
        else {
            MPIR_Assert(0);
            mpi_errno = MPIDI_OFI_COLL_STUB_KNOMIAL_bcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_knomial,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_knomial,
                            &coll_error,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_KARY_bcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_kary,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_kary,
                            &coll_error,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_STUB_bcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_stub,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_stub,
                            &coll_error);
            mpi_errno = MPIDI_OFI_COLL_MPICH_STUB_bcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_stub,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_stub,
                            &coll_error);
            mpi_errno = MPIDI_OFI_COLL_SHM_GR_bcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_shm_gr,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_shm_gr,
                            &coll_error);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_BCAST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                                         MPIR_Errflag_t *errflag)
{
    int mpi_errno, coll_error;
    int valid_colls[] = {0,2,4};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm_ptr,valid_colls,1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);
    *errflag = MPI_SUCCESS;

    if(use_coll == 0)
        mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    else {
        MPIR_Datatype *dt_ptr;
        MPID_Datatype_get_ptr(datatype,dt_ptr);

        if(use_coll == 1)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KNOMIAL_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_KNOMIAL_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_knomial,
                            &coll_error,
                            2);
        else if(use_coll == 2)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KARY_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_KARY_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_kary,
                            &coll_error,
                            2);
        else if(use_coll == 3)/**NOTE: Dissemination based allreduce is implemented only for commutative operations
                              It returns -1 otherwise and would case mpich tests to fail**/
            mpi_errno = MPIDI_OFI_COLL_MPICH_DISSEM_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_dissem,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_DISSEM_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_dissem,
                            &coll_error);
        else if(use_coll == 4)
            mpi_errno = MPIDI_OFI_COLL_MPICH_RECEXCH_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_recexch,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_RECEXCH_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_recexch,
                            &coll_error);
        else if(use_coll == 5)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KNOMIAL_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_TRIGGERED_KNOMIAL_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_knomial,
                            &coll_error,
                            2);
        else if(use_coll == 6)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KARY_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_TRIGGERED_KARY_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_kary,
                            &coll_error,
                            2);
        else if(use_coll == 7)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_DISSEM_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_dissem,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_TRIGGERED_DISSEM_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_dissem,
                            &coll_error);
        else {
            MPIR_Assert(0);
            mpi_errno = MPIDI_OFI_COLL_STUB_KNOMIAL_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_KNOMIAL_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_knomial,
                            &coll_error,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_KARY_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_KARY_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_kary,
                            &coll_error,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_DISSEM_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_dissem,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_DISSEM_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_dissem,
                            &coll_error);
            mpi_errno = MPIDI_OFI_COLL_STUB_STUB_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_stub,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_STUB_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_stub,
                            &coll_error);
            mpi_errno = MPIDI_OFI_COLL_MPICH_STUB_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_stub,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_STUB_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_stub,
                            &coll_error);
            mpi_errno = MPIDI_OFI_COLL_SHM_GR_allreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_shm_gr,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_SHM_GR_fn(op),
                            &MPIDI_OFI_COMM(comm_ptr).comm_shm_gr,
                            &coll_error);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno, coll_error;
    int valid_colls[]={0,1,2,3};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm_ptr,valid_colls,1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);

    *errflag = MPI_SUCCESS;

    if(use_coll==0)
        mpi_errno = MPIR_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                   recvtype, comm_ptr, errflag);
    else {
        MPIR_Datatype *st_ptr, *rt_ptr;
        MPID_Datatype_get_ptr(sendtype, st_ptr);
        MPID_Datatype_get_ptr(recvtype, rt_ptr);

        if(use_coll==1)
            mpi_errno =
                MPIDI_OFI_COLL_MPICH_KARY_allgather(
                    sendbuf, sendcount,
                    &MPIDI_OFI_DT(st_ptr).dt_mpich_kary,
                    recvbuf, recvcount,&MPIDI_OFI_DT(rt_ptr).dt_mpich_kary,
                    &MPIDI_OFI_COMM(comm_ptr).comm_mpich_kary, &coll_error, 2);
        else
            MPIR_Assert(0);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts, const int *displs,
                                          MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                                          MPIR_Errflag_t *errflag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);

    mpi_errno = MPIR_Allgatherv(sendbuf, sendcount, sendtype,
                                recvbuf, recvcounts, displs, recvtype, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GATHER);

    mpi_errno = MPIR_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_gatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, const int *recvcounts, const int *displs,
                                       MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr,
                                       MPIR_Errflag_t *errflag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GATHERV);

    mpi_errno = MPIR_Gatherv(sendbuf, sendcount, sendtype,
                             recvbuf, recvcounts, displs, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCATTER);

    mpi_errno = MPIR_Scatter(sendbuf, sendcount, sendtype,
                             recvbuf, recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCATTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_scatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                        const int *displs, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCATTERV);

    mpi_errno = MPIR_Scatterv(sendbuf, sendcounts, displs,
                              sendtype, recvbuf, recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCATTERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno, coll_error;
    int valid_colls[] = {0, 1, 2, 3};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm_ptr,valid_colls,1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);
    *errflag = MPI_SUCCESS;

    if(use_coll == 0)
        mpi_errno = MPIR_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, \
                                  recvtype, comm_ptr, errflag);
    else {
        MPIR_Datatype *send_dt_ptr, *recv_dt_ptr;
        MPID_Datatype_get_ptr(sendtype,send_dt_ptr);
        MPID_Datatype_get_ptr(recvtype,recv_dt_ptr);

        mpi_errno = MPIDI_OFI_COLL_MPICH_DISSEM_alltoall(sendbuf, sendcount, &MPIDI_OFI_DT(send_dt_ptr).dt_mpich_dissem,\
                                                         recvbuf, recvcount, &MPIDI_OFI_DT(recv_dt_ptr).dt_mpich_dissem,\
                                                         &MPIDI_OFI_COMM(comm_ptr).comm_mpich_dissem, &coll_error);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                         const int *sdispls, MPI_Datatype sendtype,
                                         void *recvbuf, const int *recvcounts,
                                         const int *rdispls, MPI_Datatype recvtype,
                                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno, coll_error;
    int valid_colls[] = {0,1,2,3};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm_ptr,valid_colls,1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);

    *errflag = MPI_SUCCESS;

    if(use_coll==0)
        mpi_errno = MPIR_Alltoallv(sendbuf, sendcounts, sdispls,
                                   sendtype, recvbuf, recvcounts,
                                   rdispls, recvtype, comm_ptr, errflag);
    else {
        MPIR_Datatype *st_ptr, *rt_ptr;
        MPID_Datatype_get_ptr(sendtype, st_ptr);
        MPID_Datatype_get_ptr(recvtype, rt_ptr);

        if(use_coll==1)
            mpi_errno =
                MPIDI_OFI_COLL_MPICH_KARY_alltoallv(
                    sendbuf, sendcounts, sdispls,
                    &MPIDI_OFI_DT(st_ptr).dt_mpich_kary,
                    recvbuf, recvcounts, rdispls,
                    &MPIDI_OFI_DT(rt_ptr).dt_mpich_kary,
                    &MPIDI_OFI_COMM(comm_ptr).comm_mpich_kary,
                    &coll_error);
        else
            MPIR_Assert(0);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_alltoallw(const void *sendbuf, const int sendcounts[],
                                         const int sdispls[], const MPI_Datatype sendtypes[],
                                         void *recvbuf, const int recvcounts[],
                                         const int rdispls[], const MPI_Datatype recvtypes[],
                                         MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);

    mpi_errno = MPIR_Alltoallw(sendbuf, sendcounts, sdispls,
                               sendtypes, recvbuf, recvcounts,
                               rdispls, recvtypes, comm_ptr, errflag);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_reduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, int root,
                                      MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno, coll_error;
    int valid_colls[] = {1, 2};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm_ptr,valid_colls,2);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE);

    *errflag = MPI_SUCCESS;

    if(use_coll == 0)
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype,
                                op, root, comm_ptr, errflag);
    else {
        MPIR_Datatype *dt_ptr;
        MPID_Datatype_get_ptr(datatype,dt_ptr);

        if(use_coll == 1)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KNOMIAL_reduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_KNOMIAL_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_knomial,
                            &coll_error,
                            2);
        else if(use_coll == 2)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KARY_reduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_KARY_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_kary,
                            &coll_error,
                            2);
        else if(use_coll == 3)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KNOMIAL_reduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_TRIGGERED_KNOMIAL_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_knomial,
                            &coll_error,
                            2);
        else if(use_coll == 4)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KARY_reduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_TRIGGERED_KARY_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_kary,
                            &coll_error,
                            2);
        else {
            MPIR_Assert(0);
            mpi_errno = MPIDI_OFI_COLL_STUB_KNOMIAL_reduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_KNOMIAL_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_knomial,
                            &coll_error,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_KARY_reduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_KARY_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_kary,
                            &coll_error,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_STUB_reduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_stub,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_STUB_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_stub,
                            &coll_error);
            mpi_errno = MPIDI_OFI_COLL_MPICH_STUB_reduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_stub,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_STUB_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_stub,
                            &coll_error);
            mpi_errno = MPIDI_OFI_COLL_SHM_GR_reduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_shm_gr,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_SHM_GR_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_shm_gr,
                            &coll_error);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_reduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                              const int recvcounts[], MPI_Datatype datatype,
                                              MPI_Op op, MPIR_Comm *comm_ptr,
                                              MPIR_Errflag_t *errflag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);

    mpi_errno = MPIR_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_reduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                    int recvcount, MPI_Datatype datatype,
                                                    MPI_Op op, MPIR_Comm *comm_ptr,
                                                    MPIR_Errflag_t *errflag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);

    mpi_errno = MPIR_Reduce_scatter_block(sendbuf, recvbuf, recvcount,
                                          datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_scan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                                    MPIR_Errflag_t *errflag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCAN);

    mpi_errno = MPIR_Scan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCAN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_exscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                                      MPIR_Errflag_t *errflag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_EXSCAN);

    mpi_errno = MPIR_Exscan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_EXSCAN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_neighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm *comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHER);

    mpi_errno =
        MPIR_Neighbor_allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                     comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_neighbor_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int recvcounts[], const int displs[],
                                                   MPI_Datatype recvtype, MPIR_Comm *comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHERV);

    mpi_errno = MPIR_Neighbor_allgatherv_impl(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcounts, displs, recvtype, comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_neighbor_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm *comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALL);

    mpi_errno = MPIR_Neighbor_alltoall_impl(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_neighbor_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_neighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                  const int sdispls[], MPI_Datatype sendtype,
                                                  void *recvbuf, const int recvcounts[],
                                                  const int rdispls[], MPI_Datatype recvtype,
                                                  MPIR_Comm *comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLV);

    mpi_errno = MPIR_Neighbor_alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype,
                                             recvbuf, recvcounts, rdispls, recvtype, comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_neighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_neighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                  const MPI_Aint sdispls[],
                                                  const MPI_Datatype sendtypes[], void *recvbuf,
                                                  const int recvcounts[], const MPI_Aint rdispls[],
                                                  const MPI_Datatype recvtypes[],
                                                  MPIR_Comm *comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLW);

    mpi_errno = MPIR_Neighbor_alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                             recvbuf, recvcounts, rdispls, recvtypes, comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER);

    mpi_errno = MPIR_Ineighbor_allgather_impl(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int recvcounts[], const int displs[],
                                                    MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                                                    MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV);

    mpi_errno = MPIR_Ineighbor_allgatherv_impl(sendbuf, sendcount, sendtype,
                                               recvbuf, recvcounts, displs, recvtype,
                                               comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL);

    mpi_errno = MPIR_Ineighbor_alltoall_impl(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                   const int sdispls[], MPI_Datatype sendtype,
                                                   void *recvbuf, const int recvcounts[],
                                                   const int rdispls[], MPI_Datatype recvtype,
                                                   MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV);

    mpi_errno = MPIR_Ineighbor_alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype,
                                              recvbuf, recvcounts, rdispls, recvtype,
                                              comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                   const MPI_Aint sdispls[],
                                                   const MPI_Datatype sendtypes[], void *recvbuf,
                                                   const int recvcounts[], const MPI_Aint rdispls[],
                                                   const MPI_Datatype recvtypes[],
                                                   MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);

    mpi_errno = MPIR_Ineighbor_alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                              recvbuf, recvcounts, rdispls, recvtypes,
                                              comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ibarrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ibarrier(MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    int valid_colls[] = {0};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm_ptr,valid_colls,1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IBARRIER);

    if(use_coll == 0)
        mpi_errno = MPIR_Ibarrier_impl(comm_ptr, req);
    else {
        MPIR_Request *mpid_req;
        MPIDI_OFI_REQUEST_CREATE(mpid_req,MPIR_REQUEST_KIND__COLL);
        *req = mpid_req->handle;

        if(use_coll == 1)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KNOMIAL_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_knomial,
                            2);
        else if(use_coll == 2)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KARY_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_kary,
                            2);
        else if(use_coll == 3)
            mpi_errno = MPIDI_OFI_COLL_MPICH_DISSEM_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_dissem,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_dissem);
        else if(use_coll == 4)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KNOMIAL_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.triggered_knomial,
                            2);
        else if(use_coll == 5)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KARY_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.triggered_kary,
                            2);
        else if(use_coll == 6)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_DISSEM_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_dissem,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.triggered_dissem);
        else {
            MPIR_Assert(0);
            mpi_errno = MPIDI_OFI_COLL_STUB_KNOMIAL_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_knomial,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_KARY_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_kary,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_DISSEM_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_dissem,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_dissem);
            mpi_errno = MPIDI_OFI_COLL_STUB_STUB_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_stub,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_stub);
            mpi_errno = MPIDI_OFI_COLL_MPICH_STUB_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_stub,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_stub);
            mpi_errno = MPIDI_OFI_COLL_SHM_GR_ibarrier(
                            &MPIDI_OFI_COMM(comm_ptr).comm_shm_gr,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.shm_gr);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IBARRIER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ibcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                      int root, MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    int valid_colls[] = {1, 2};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm_ptr,valid_colls,2);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IBCAST);

    if(use_coll == 0)
        mpi_errno = MPIR_Ibcast_impl(buffer, count, datatype, root, comm_ptr, req);
    else {
        MPIR_Datatype *dt_ptr;
        MPIR_Request *mpid_req;
        MPIDI_OFI_REQUEST_CREATE(mpid_req,MPIR_REQUEST_KIND__COLL);
        *req = mpid_req->handle;
        MPID_Datatype_get_ptr(datatype,dt_ptr);

        if(use_coll == 1)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KNOMIAL_ibcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_knomial,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_knomial,
                            2);
        else if(use_coll == 2)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KARY_ibcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_kary,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_kary,
                            2);
        else if(use_coll == 3)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KNOMIAL_ibcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_knomial,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.triggered_knomial,
                            2);
        else if(use_coll == 4)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KARY_ibcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_kary,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.triggered_kary,
                            2);
        else {
            MPIR_Assert(0);
            mpi_errno = MPIDI_OFI_COLL_STUB_KNOMIAL_ibcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_knomial,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_knomial,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_KARY_ibcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_kary,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_kary,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_STUB_ibcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_stub,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_stub,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_stub);
            mpi_errno = MPIDI_OFI_COLL_MPICH_STUB_ibcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_stub,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_stub,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_stub);
            mpi_errno = MPIDI_OFI_COLL_SHM_GR_ibcast(
                            buffer,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_shm_gr,
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_shm_gr,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.shm_gr);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IBCAST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iallgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLGATHER);

    mpi_errno = MPIR_Iallgather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                     recvcount, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iallgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                                           MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV);

    mpi_errno = MPIR_Iallgatherv_impl(sendbuf, sendcount, sendtype,
                                      recvbuf, recvcounts, displs, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iallreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm,
                                          MPI_Request *request)
{
    int mpi_errno;
    int valid_colls[] = {1, 2, 4};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm,valid_colls,3);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE);

    if(use_coll == 0)
        mpi_errno = MPIR_Iallreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, request);
    else {
        MPIR_Datatype *dt_ptr;
        MPIR_Request *mpid_req;
        MPIDI_OFI_REQUEST_CREATE(mpid_req,MPIR_REQUEST_KIND__COLL);
        *request = mpid_req->handle;
        MPID_Datatype_get_ptr(datatype,dt_ptr);

        if(use_coll == 1)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KNOMIAL_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_KNOMIAL_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_mpich_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_knomial,
                            2);
        else if(use_coll == 2)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KARY_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_KARY_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_mpich_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_kary,
                            2);
        else if(use_coll == 3)
            mpi_errno = MPIDI_OFI_COLL_MPICH_DISSEM_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_dissem,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_DISSEM_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_mpich_dissem,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_dissem);
        else if(use_coll == 4)
            mpi_errno = MPIDI_OFI_COLL_MPICH_RECEXCH_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_recexch,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_RECEXCH_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_mpich_recexch,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_recexch);
        else if(use_coll == 5)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KNOMIAL_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_TRIGGERED_KNOMIAL_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_triggered_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.triggered_knomial,
                            2);
        else if(use_coll == 6)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KARY_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_TRIGGERED_KARY_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_triggered_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.triggered_kary,
                            2);
        else if(use_coll == 7)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_DISSEM_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_dissem,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_TRIGGERED_DISSEM_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_triggered_dissem,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.triggered_dissem);
        else {
            MPIR_Assert(0);
            mpi_errno = MPIDI_OFI_COLL_STUB_KNOMIAL_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_KNOMIAL_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_stub_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_knomial,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_KARY_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_KARY_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_stub_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_kary,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_DISSEM_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_dissem,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_DISSEM_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_stub_dissem,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_dissem);
            mpi_errno = MPIDI_OFI_COLL_STUB_STUB_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_stub,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_STUB_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_stub_stub,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_stub);
            mpi_errno = MPIDI_OFI_COLL_MPICH_STUB_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_stub,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_STUB_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_mpich_stub,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_stub);
            mpi_errno = MPIDI_OFI_COLL_SHM_GR_iallreduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_shm_gr,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_SHM_GR_fn(op),
                            &MPIDI_OFI_COMM(comm).comm_shm_gr,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.shm_gr);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ialltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno, coll_error;
    int valid_colls[] = {1, 2, 3};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm_ptr,valid_colls,1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALL);

    if(use_coll == 0)
        mpi_errno = MPIR_Ialltoall_impl(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcount, recvtype, comm_ptr, req);
    else {
        MPIR_Datatype *send_dt_ptr, *recv_dt_ptr;
        MPIR_Request  *mpid_req;
        MPID_Datatype_get_ptr(sendtype,send_dt_ptr);
        MPID_Datatype_get_ptr(recvtype,recv_dt_ptr);
        MPIDI_OFI_REQUEST_CREATE(mpid_req,MPIR_REQUEST_KIND__COLL);
        *req = mpid_req->handle;
        mpi_errno =
            MPIDI_OFI_COLL_MPICH_DISSEM_ialltoall(
                sendbuf, sendcount,
                &MPIDI_OFI_DT(send_dt_ptr).dt_mpich_dissem,
                recvbuf, recvcount,
                &MPIDI_OFI_DT(recv_dt_ptr).dt_mpich_dissem,
                &MPIDI_OFI_COMM(comm_ptr).comm_mpich_dissem,
                &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_dissem);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ialltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                          const int *sdispls, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts,
                                          const int *rdispls, MPI_Datatype recvtype,
                                          MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV);

    mpi_errno = MPIR_Ialltoallv_impl(sendbuf, sendcounts, sdispls,
                                     sendtype, recvbuf, recvcounts,
                                     rdispls, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ialltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                          const int *sdispls, const MPI_Datatype sendtypes[],
                                          void *recvbuf, const int *recvcounts,
                                          const int *rdispls, const MPI_Datatype recvtypes[],
                                          MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);

    mpi_errno = MPIR_Ialltoallw_impl(sendbuf, sendcounts, sdispls,
                                     sendtypes, recvbuf, recvcounts,
                                     rdispls, recvtypes, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iexscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                                       MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);

    mpi_errno = MPIR_Iexscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_igather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       int root, MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IGATHER);

    mpi_errno = MPIR_Igather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, root, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_igatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int *recvcounts, const int *displs,
                                        MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr,
                                        MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IGATHERV);

    mpi_errno = MPIR_Igatherv_impl(sendbuf, sendcount, sendtype,
                                   recvbuf, recvcounts, displs, recvtype, root, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                     int recvcount, MPI_Datatype datatype,
                                                     MPI_Op op, MPIR_Comm *comm_ptr,
                                                     MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);

    mpi_errno = MPIR_Ireduce_scatter_block_impl(sendbuf, recvbuf, recvcount,
                                                datatype, op, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                               const int recvcounts[], MPI_Datatype datatype,
                                               MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);

    mpi_errno = MPIR_Ireduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op,
                                          comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, int root,
                                       MPIR_Comm *comm_ptr, MPI_Request *req)
{
    int mpi_errno;
    int valid_colls[] = {1, 2};
    int use_coll = MPIDI_OFI_cycle_algorithm(comm_ptr,valid_colls,2);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE);

    if(use_coll == 0)
        mpi_errno = MPIR_Ireduce_impl(sendbuf, recvbuf, count, datatype,
                                      op, root, comm_ptr, req);
    else {
        MPIR_Datatype *dt_ptr;
        MPIR_Request *mpid_req;
        MPIDI_OFI_REQUEST_CREATE(mpid_req,MPIR_REQUEST_KIND__COLL);
        *req = mpid_req->handle;
        MPID_Datatype_get_ptr(datatype,dt_ptr);

        if(use_coll == 1)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KNOMIAL_ireduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_KNOMIAL_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_knomial,
                            2);
        else if(use_coll == 2)
            mpi_errno = MPIDI_OFI_COLL_MPICH_KARY_ireduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_KARY_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_kary,
                            2);
        else if(use_coll == 3)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KNOMIAL_ireduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_TRIGGERED_KNOMIAL_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.triggered_knomial,
                            2);
        else if(use_coll == 4)
            mpi_errno = MPIDI_OFI_COLL_TRIGGERED_KARY_ireduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_triggered_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_TRIGGERED_KARY_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_triggered_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.triggered_kary,
                            2);
        else {
            MPIR_Assert(0);
            mpi_errno = MPIDI_OFI_COLL_STUB_KNOMIAL_ireduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_knomial,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_KNOMIAL_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_knomial,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_knomial,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_KARY_ireduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_kary,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_KARY_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_kary,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_kary,
                            2);
            mpi_errno = MPIDI_OFI_COLL_STUB_STUB_ireduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_stub_stub,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_STUB_STUB_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_stub_stub,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.stub_stub);
            mpi_errno = MPIDI_OFI_COLL_MPICH_STUB_ireduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_mpich_stub,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_MPICH_STUB_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_mpich_stub,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.mpich_stub);
            mpi_errno = MPIDI_OFI_COLL_SHM_GR_ireduce(
                            sendbuf,recvbuf,count,
                            &MPIDI_OFI_DT(dt_ptr).dt_shm_gr,
                            MPIDI_OFI_MPI_OP_TO_COLLOP_SHM_GR_fn(op),
                            root,
                            &MPIDI_OFI_COMM(comm_ptr).comm_shm_gr,
                            &MPIDI_OFI_REQUEST(mpid_req,util).collreq.shm_gr);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                                     MPI_Request *req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCAN);

    mpi_errno = MPIR_Iscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCAN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iscatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iscatter(const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        int recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm *comm, MPI_Request *request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCATTER);

    mpi_errno = MPIR_Iscatter_impl(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcount, recvtype, root, comm, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCATTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iscatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                         const int *displs, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount,
                                         MPI_Datatype recvtype, int root,
                                         MPIR_Comm *comm, MPI_Request *request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCATTERV);

    mpi_errno = MPIR_Iscatterv_impl(sendbuf, sendcounts, displs, sendtype,
                                    recvbuf, recvcount, recvtype, root, comm, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCATTERV);
    return mpi_errno;
}

#endif /* OFI_COLL_H_INCLUDED */
