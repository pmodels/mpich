/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "nd_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_poll(int in_blocking_poll)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_POLL);

    /* MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_POLL); */

    mpi_errno = MPID_Nem_nd_sm_poll(in_blocking_poll);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_POLL); */
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}
