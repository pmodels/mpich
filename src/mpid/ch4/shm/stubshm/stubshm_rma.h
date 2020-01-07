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



    MPIR_Assert(0);


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



    MPIR_Assert(0);


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



    MPIR_Assert(0);


    return MPI_SUCCESS;
}


static inline int MPIDI_STUBSHM_mpi_compare_and_swap(const void *origin_addr,
                                                     const void *compare_addr,
                                                     void *result_addr,
                                                     MPI_Datatype datatype,
                                                     int target_rank, MPI_Aint target_disp,
                                                     MPIR_Win * win)
{



    MPIR_Assert(0);


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



    MPIR_Assert(0);


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



    MPIR_Assert(0);


    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_fetch_and_op(const void *origin_addr,
                                                 void *result_addr,
                                                 MPI_Datatype datatype,
                                                 int target_rank,
                                                 MPI_Aint target_disp, MPI_Op op, MPIR_Win * win)
{



    MPIR_Assert(0);


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



    MPIR_Assert(0);


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



    MPIR_Assert(0);


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



    MPIR_Assert(0);


    return MPI_SUCCESS;
}

#endif /* STUBSHM_RMA_H_INCLUDED */
