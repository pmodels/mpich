/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ch4r_recvq.h"

#undef FUNCNAME
#define FUNCNAME MPIDIG_recvq_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDIG_recvq_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_PVAR_LEVEL_REGISTER_STATIC(RECVQ, MPI_UNSIGNED, posted_recvq_length, 0,      /* init value */
                                      MPI_T_VERBOSITY_USER_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "CH4",      /* category name */
                                      "length of the posted message receive queue");

    MPIR_T_PVAR_LEVEL_REGISTER_STATIC(RECVQ, MPI_UNSIGNED, unexpected_recvq_length, 0,  /* init value */
                                      MPI_T_VERBOSITY_USER_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "CH4",      /* category name */
                                      "length of the unexpected message receive queue");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(RECVQ, MPI_UNSIGNED_LONG_LONG, posted_recvq_match_attempts, MPI_T_VERBOSITY_USER_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "CH4",        /* category name */
                                        "number of search passes on the posted message receive queue");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(RECVQ,
                                        MPI_UNSIGNED_LONG_LONG,
                                        unexpected_recvq_match_attempts,
                                        MPI_T_VERBOSITY_USER_DETAIL,
                                        MPI_T_BIND_NO_OBJECT,
                                        (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS),
                                        "CH4",
                                        "number of search passes on the unexpected message receive queue");

    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RECVQ, MPI_DOUBLE, time_failed_matching_postedq, MPI_T_VERBOSITY_USER_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "CH4",     /* category name */
                                      "total time spent on unsuccessful search passes on the posted receives queue");

    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RECVQ, MPI_DOUBLE, time_matching_unexpectedq, MPI_T_VERBOSITY_USER_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "CH4",        /* category name */
                                      "total time spent on search passes on the unexpected receive queue");

    return mpi_errno;
}
