/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*D
  
MPI_DUP_FN - A function to simple-mindedly copy attributes  

D*/
int MPIR_Dup_fn ( MPI_Comm comm ATTRIBUTE((unused)), 
		  int keyval ATTRIBUTE((unused)), 
		  void *extra_state ATTRIBUTE((unused)), void *attr_in, 
		  void *attr_out, int *flag )
{
    /* No error checking at present */

    MPIU_UNREFERENCED_ARG(comm);
    MPIU_UNREFERENCED_ARG(keyval);
    MPIU_UNREFERENCED_ARG(extra_state);

    /* Set attr_out, the flag and return success */
    (*(void **)attr_out) = attr_in;
    (*flag) = 1;
    return (MPI_SUCCESS);
}
