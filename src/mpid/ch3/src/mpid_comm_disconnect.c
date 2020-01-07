/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Comm_disconnect - Disconnect a communicator 

   Arguments:
.  comm_ptr - communicator

   Notes:

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Comm_disconnect(MPIR_Comm *comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;




    /* Check to make sure the communicator hasn't already been revoked */
    if (comm_ptr->revoked) {
        MPIR_ERR_SETANDJUMP(mpi_errno,MPIX_ERR_REVOKED,"**revoked");
    }

    /* it's more than a comm_release, but ok for now */
    /* FIXME: Describe what more might be required */
    /* MPIU_PG_Printall( stdout ); */
    comm_ptr->dev.is_disconnected = 1;
    mpi_errno = MPIR_Comm_release(comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);
    /* If any of the VCs were released by this Comm_release, wait
     for those close operations to complete */
    mpi_errno = MPIDI_CH3U_VC_WaitForClose();
    MPIR_ERR_CHECK(mpi_errno);
    /* MPIU_PG_Printall( stdout ); */


fn_exit:

    return mpi_errno;
fn_fail:
    goto fn_exit;
}
