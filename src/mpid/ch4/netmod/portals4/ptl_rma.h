/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef PTL_RMA_H_INCLUDED
#define PTL_RMA_H_INCLUDED

#include "ptl_impl.h"

static inline int MPIDI_NM_mpi_put(const void *origin_addr,
                                   int origin_count,
                                   MPI_Datatype origin_datatype,
                                   int target_rank,
                                   MPI_Aint target_disp,
                                   int target_count, MPI_Datatype target_datatype,
                                   MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4U_mpi_put(origin_addr, origin_count, origin_datatype,
                              target_rank, target_disp, target_count, target_datatype, win);
}

static inline int MPIDI_NM_mpi_get(void *origin_addr,
                                   int origin_count,
                                   MPI_Datatype origin_datatype,
                                   int target_rank,
                                   MPI_Aint target_disp,
                                   int target_count, MPI_Datatype target_datatype,
                                   MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4U_mpi_get(origin_addr, origin_count, origin_datatype,
                              target_rank, target_disp, target_count, target_datatype, win);
}

static inline int MPIDI_NM_mpi_rput(const void *origin_addr,
                                    int origin_count,
                                    MPI_Datatype origin_datatype,
                                    int target_rank,
                                    MPI_Aint target_disp,
                                    int target_count,
                                    MPI_Datatype target_datatype,
                                    MPIR_Win * win,
                                    MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_rput(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count, target_datatype, win,
                               request);
}


static inline int MPIDI_NM_mpi_compare_and_swap(const void *origin_addr,
                                                const void *compare_addr,
                                                void *result_addr,
                                                MPI_Datatype datatype,
                                                int target_rank, MPI_Aint target_disp,
                                                MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4U_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                           datatype, target_rank, target_disp, win);
}

static inline int MPIDI_NM_mpi_raccumulate(const void *origin_addr,
                                           int origin_count,
                                           MPI_Datatype origin_datatype,
                                           int target_rank,
                                           MPI_Aint target_disp,
                                           int target_count,
                                           MPI_Datatype target_datatype,
                                           MPI_Op op, MPIR_Win * win,
                                           MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, op, win, request);
}

static inline int MPIDI_NM_mpi_rget_accumulate(const void *origin_addr,
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
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                          result_addr, result_count, result_datatype,
                                          target_rank, target_disp, target_count,
                                          target_datatype, op, win, request);
}

static inline int MPIDI_NM_mpi_fetch_and_op(const void *origin_addr,
                                            void *result_addr,
                                            MPI_Datatype datatype,
                                            int target_rank,
                                            MPI_Aint target_disp, MPI_Op op,
                                            MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4U_mpi_fetch_and_op(origin_addr, result_addr, datatype,
                                       target_rank, target_disp, op, win);
}


static inline int MPIDI_NM_mpi_rget(void *origin_addr,
                                    int origin_count,
                                    MPI_Datatype origin_datatype,
                                    int target_rank,
                                    MPI_Aint target_disp,
                                    int target_count,
                                    MPI_Datatype target_datatype,
                                    MPIR_Win * win,
                                    MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_rget(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count, target_datatype, win,
                               request);
}


static inline int MPIDI_NM_mpi_get_accumulate(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              void *result_addr,
                                              int result_count,
                                              MPI_Datatype result_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count,
                                              MPI_Datatype target_datatype, MPI_Op op,
                                              MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4U_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                         result_addr, result_count, result_datatype,
                                         target_rank, target_disp, target_count,
                                         target_datatype, op, win);
}

static inline int MPIDI_NM_mpi_accumulate(const void *origin_addr,
                                          int origin_count,
                                          MPI_Datatype origin_datatype,
                                          int target_rank,
                                          MPI_Aint target_disp,
                                          int target_count,
                                          MPI_Datatype target_datatype, MPI_Op op,
                                          MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDI_CH4U_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, op,
                                     win);
}

#endif /* PTL_RMA_H_INCLUDED */
