/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */


#include "mpiimpl.h"
#include "tsp_stubtran.h"

int MPII_Stubtran_init(void)
{
    return MPI_SUCCESS;
}

int MPII_Stubtran_finalize(void)
{
    return MPI_SUCCESS;
}

int MPII_Stubtran_comm_init(MPIR_Comm * comm)
{
    return MPI_SUCCESS;
}

int MPII_Stubtran_comm_cleanup(MPIR_Comm * comm)
{
    return MPI_SUCCESS;
}
