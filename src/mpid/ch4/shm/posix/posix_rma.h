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
#ifndef POSIX_RMA_H_INCLUDED
#define POSIX_RMA_H_INCLUDED

#include "posix_impl.h"

static inline int MPIDI_POSIX_mpi_put(const void *origin_addr,
                                      int origin_count,
                                      MPI_Datatype origin_datatype,
                                      int target_rank,
                                      MPI_Aint target_disp,
                                      int target_count, MPI_Datatype target_datatype,
                                      MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_PUT);

    mpi_errno = MPIDI_NM_mpi_put(origin_addr, origin_count, origin_datatype, target_rank,
                                 target_disp, target_count, target_datatype, win, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_PUT);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_get(void *origin_addr,
                                      int origin_count,
                                      MPI_Datatype origin_datatype,
                                      int target_rank,
                                      MPI_Aint target_disp,
                                      int target_count, MPI_Datatype target_datatype,
                                      MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_GET);

    mpi_errno = MPIDI_NM_mpi_get(origin_addr, origin_count, origin_datatype, target_rank,
                                 target_disp, target_count, target_datatype, win, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_GET);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_rput(const void *origin_addr,
                                       int origin_count,
                                       MPI_Datatype origin_datatype,
                                       int target_rank,
                                       MPI_Aint target_disp,
                                       int target_count,
                                       MPI_Datatype target_datatype,
                                       MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_RPUT);

    mpi_errno = MPIDI_NM_mpi_rput(origin_addr, origin_count, origin_datatype, target_rank,
                                  target_disp, target_count, target_datatype, win, NULL, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_RPUT);

    return mpi_errno;
}


static inline int MPIDI_POSIX_mpi_compare_and_swap(const void *origin_addr,
                                                   const void *compare_addr,
                                                   void *result_addr,
                                                   MPI_Datatype datatype,
                                                   int target_rank, MPI_Aint target_disp,
                                                   MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMPARE_AND_SWAP);

    mpi_errno = MPIDI_NM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr, datatype,
                                              target_rank, target_disp, win, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMPARE_AND_SWAP);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_raccumulate(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count,
                                              MPI_Datatype target_datatype,
                                              MPI_Op op, MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_RACCUMULATE);

    mpi_errno = MPIDI_NM_mpi_raccumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                         target_disp, target_count, target_datatype, op, win, NULL,
                                         request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_RACCUMULATE);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_rget_accumulate(const void *origin_addr,
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
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_RGET_ACCUMULATE);

    mpi_errno = MPIDI_NM_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                             result_addr, result_count, result_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win, NULL, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_RGET_ACCUMULATE);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_fetch_and_op(const void *origin_addr,
                                               void *result_addr,
                                               MPI_Datatype datatype,
                                               int target_rank,
                                               MPI_Aint target_disp, MPI_Op op, MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_FETCH_AND_OP);

    mpi_errno = MPIDI_NM_mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank,
                                          target_disp, op, win, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_FETCH_AND_OP);

    return mpi_errno;
}


static inline int MPIDI_POSIX_mpi_rget(void *origin_addr,
                                       int origin_count,
                                       MPI_Datatype origin_datatype,
                                       int target_rank,
                                       MPI_Aint target_disp,
                                       int target_count,
                                       MPI_Datatype target_datatype,
                                       MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_RGET);

    mpi_errno = MPIDI_NM_mpi_rget(origin_addr, origin_count, origin_datatype, target_rank,
                                  target_disp, target_count, target_datatype, win, NULL, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_RGET);

    return mpi_errno;
}


static inline int MPIDI_POSIX_mpi_get_accumulate(const void *origin_addr,
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
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_GET_ACCUMULATE);

    mpi_errno = MPIDI_NM_mpi_get_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                            result_count, result_datatype, target_rank, target_disp,
                                            target_count, target_datatype, op, win, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_GET_ACCUMULATE);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_accumulate(const void *origin_addr,
                                             int origin_count,
                                             MPI_Datatype origin_datatype,
                                             int target_rank,
                                             MPI_Aint target_disp,
                                             int target_count,
                                             MPI_Datatype target_datatype, MPI_Op op,
                                             MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ACCUMULATE);

    mpi_errno = MPIDI_NM_mpi_accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                        target_disp, target_count, target_datatype, op, win, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ACCUMULATE);

    return mpi_errno;
}

#endif /* POSIX_RMA_H_INCLUDED */
