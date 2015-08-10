/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* This is the group-collective implementation of the barrier operation.  It
   currently is built on group allreduce.

   This is an intracommunicator barrier only!
*/

int MPIR_Barrier_group( MPID_Comm *comm_ptr, MPID_Group *group_ptr, int tag, MPIR_Errflag_t *errflag )
{
    int src = 0;
    int dst, mpi_errno = MPI_SUCCESS;

    /* Trivial barriers return immediately */
    if (comm_ptr->local_size == 1) goto fn_exit;

    mpi_errno = MPIR_Allreduce_group(&src, &dst, 1, MPI_INT, MPI_BAND, 
                                      comm_ptr, group_ptr, tag, errflag);

    if (mpi_errno != MPI_SUCCESS || *errflag != MPIR_ERR_NONE)
        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

