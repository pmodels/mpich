/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "iallreduce_tsp_recexch_reduce_scatter_recexch_allgatherv_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Iallreduce_intra_gentran_recexch_reduce_scatter_recexch_allgatherv(const void *sendbuf,
                                                                            void *recvbuf,
                                                                            int count,
                                                                            MPI_Datatype datatype,
                                                                            MPI_Op op,
                                                                            MPIR_Comm * comm,
                                                                            MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPII_Gentran_Iallreduce_intra_recexch_reduce_scatter_recexch_allgatherv(sendbuf, recvbuf,
                                                                                count, datatype, op,
                                                                                comm, req,
                                                                                MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL);

    return mpi_errno;
}
