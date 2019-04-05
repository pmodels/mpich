/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
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
