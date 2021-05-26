/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


void MPIR_NO_OP(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    return;
}


int MPIR_NO_OP_check_dtype(MPI_Datatype type)
{
    return MPI_SUCCESS;
}
