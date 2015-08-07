/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"


#undef FUNCNAME
#define FUNCNAME MPIR_NO_OP
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_NO_OP( void *invec, void *inoutvec, int *Len, MPI_Datatype *type )
{
    return;
}


#undef FUNCNAME
#define FUNCNAME MPIR_NO_OP_check_dtype
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_NO_OP_check_dtype( MPI_Datatype type )
{
    return MPI_SUCCESS;
}

