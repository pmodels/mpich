/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
#undef FUNCNAME
#define FUNCNAME MPID_Comm_disconnect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Comm_disconnect(MPID_Comm *comm_ptr)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_DISCONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_DISCONNECT);

    /* Before releasing the communicator, we need to ensure that all VCs are
       in a stable state.  In particular, if a VC is still in the process of
       connecting, complete the connection before tearing it down */
    /* FIXME: How can we get to a state where we are still connecting a VC but
       the MPIR_Comm_release will find that the ref count decrements to zero 
       (it may be that some operation fails to increase/decrease the reference 
       count.  A patch could be to increment the reference count while 
       connecting, then decrement it.  But the increment in the reference 
       count should come 
       from the step that caused the connection steps to be initiated.  
       Possibility: if the send queue is not empty, the ref count should
       be higher.  */
    /* FIXME: This doesn't work yet */
    /*
    mpi_errno = MPIDI_CH3U_Comm_FinishPending( comm_ptr );
    */

    /* it's more than a comm_release, but ok for now */
    /* FIXME: Describe what more might be required */
    /* MPIU_PG_Printall( stdout ); */
    mpi_errno = MPIR_Comm_release(comm_ptr,1);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    /* If any of the VCs were released by this Comm_release, wait
     for those close operations to complete */
    mpi_errno = MPIDI_CH3U_VC_WaitForClose();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    /* MPIU_PG_Printall( stdout ); */


fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_DISCONNECT);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
