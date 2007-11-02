/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidi_ch3_impl.h"

/* These routines are invoked by the ch3 connect and accept code, 
   with the MPIDI_CH3_HAS_CONN_ACCEPT_HOOK ifdef */

#ifdef MPIDI_CH3_HAS_CONN_ACCEPT_HOOK

    /* alternatively, ch3:sctp can not use the CONN_ACCEPT_HOOK and could put these into acceptq_dequeue
     *  using some more desriptive hook describing that the progress engine isn't build for select(), and
     *  uses dynamic_tmp_ vars instead...  this is what MPIDI_CH3_CHANNEL_AVOIDS_SELECT does.
     */


/* called by MPIDI_Create_inter_root_communicator_connect */
int MPIDI_CH3_Complete_unidirectional_connection( MPIDI_VC_t *connect_vc )
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

/* called by MPIDI_Create_inter_root_communicator_accept, after dequeued from acceptQ */
int MPIDI_CH3_Complete_unidirectional_connection2( MPIDI_VC_t *new_vc )
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_CH3I_dynamic_tmp_vc = new_vc;
    MPIDI_CH3I_dynamic_tmp_fd = new_vc->ch.fd;
    
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Cleanup_after_connection
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Cleanup_after_connection( MPIDI_VC_t *new_vc )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_CLEANUP_AFTER_CONNECTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_CLEANUP_AFTER_CONNECTION);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_CLEANUP_AFTER_CONNECTION);
    return mpi_errno;
}

#endif
