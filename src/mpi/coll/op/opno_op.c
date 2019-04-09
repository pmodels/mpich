/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"


void MPIR_NO_OP(void *invec, void *inoutvec, int *Len, MPI_Datatype * type)
{
    return;
}


int MPIR_NO_OP_check_dtype(MPI_Datatype type)
{
    return MPI_SUCCESS;
}
