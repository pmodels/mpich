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
#ifndef STUBSHM_RMA_H_INCLUDED
#define STUBSHM_RMA_H_INCLUDED

#include "stubshm_impl.h"

static inline int MPIDI_STUBSHM_mpi_put(const void *origin_addr,
                                        int origin_count,
                                        MPI_Datatype origin_datatype,
                                        int target_rank,
                                        MPI_Aint target_disp,
                                        int target_count, MPI_Datatype target_datatype,
                                        MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_PUT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_PUT);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_get(void *origin_addr,
                                        int origin_count,
                                        MPI_Datatype origin_datatype,
                                        int target_rank,
                                        MPI_Aint target_disp,
                                        int target_count, MPI_Datatype target_datatype,
                                        MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_GET);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_GET);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_rput(const void *origin_addr,
                                         int origin_count,
                                         MPI_Datatype origin_datatype,
                                         int target_rank,
                                         MPI_Aint target_disp,
                                         int target_count,
                                         MPI_Datatype target_datatype,
                                         MPIR_Win * win, MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_RPUT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_RPUT);
    return MPI_SUCCESS;
}


static inline int MPIDI_STUBSHM_mpi_compare_and_swap(const void *origin_addr,
                                                     const void *compare_addr,
                                                     void *result_addr,
                                                     MPI_Datatype datatype,
                                                     int target_rank, MPI_Aint target_disp,
                                                     MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_COMPARE_AND_SWAP);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_COMPARE_AND_SWAP);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_raccumulate(const void *origin_addr,
                                                int origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                int target_count,
                                                MPI_Datatype target_datatype,
                                                MPI_Op op, MPIR_Win * win, MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_RACCUMULATE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_RACCUMULATE);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_rget_accumulate(const void *origin_addr,
                                                    int origin_count,
                                                    MPI_Datatype origin_datatype,
                                                    void *result_addr,
                                                    int result_count,
                                                    MPI_Datatype result_datatype,
                                                    int target_rank,
                                                    MPI_Aint target_disp,
                                                    int target_count,
                                                    MPI_Datatype target_datatype,
                                                    MPI_Op op, MPIR_Win * win,
                                                    MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_RGET_ACCUMULATE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_RGET_ACCUMULATE);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_fetch_and_op(const void *origin_addr,
                                                 void *result_addr,
                                                 MPI_Datatype datatype,
                                                 int target_rank,
                                                 MPI_Aint target_disp, MPI_Op op, MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_FETCH_AND_OP);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_FETCH_AND_OP);
    return MPI_SUCCESS;
}


static inline int MPIDI_STUBSHM_mpi_rget(void *origin_addr,
                                         int origin_count,
                                         MPI_Datatype origin_datatype,
                                         int target_rank,
                                         MPI_Aint target_disp,
                                         int target_count,
                                         MPI_Datatype target_datatype,
                                         MPIR_Win * win, MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_RGET);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_RGET);
    return MPI_SUCCESS;
}


static inline int MPIDI_STUBSHM_mpi_get_accumulate(const void *origin_addr,
                                                   int origin_count,
                                                   MPI_Datatype origin_datatype,
                                                   void *result_addr,
                                                   int result_count,
                                                   MPI_Datatype result_datatype,
                                                   int target_rank,
                                                   MPI_Aint target_disp,
                                                   int target_count,
                                                   MPI_Datatype target_datatype, MPI_Op op,
                                                   MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_GET_ACCUMULATE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_GET_ACCUMULATE);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_accumulate(const void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count,
                                               MPI_Datatype target_datatype, MPI_Op op,
                                               MPIR_Win * win)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ACCUMULATE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ACCUMULATE);
    return MPI_SUCCESS;
}

#endif /* STUBSHM_RMA_H_INCLUDED */
