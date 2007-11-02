/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidi_ch3_impl.h"

/* These routines are invoked by the ch3 connect and accept code, 
   with the MPIDI_CH3_HAS_CONN_ACCEPT_HOOK ifdef */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Complete_unidirectional_connection
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Complete_unidirectional_connection( MPIDI_VC_t *connect_vc )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *accept_vc = NULL;
    MPID_Progress_state progress_state;
    int port_name_tag;
    char connector_port[MPI_MAX_PORT_NAME];
    int num_written = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMPLETE_UNIDIRECTIONAL_CONNECTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMPLETE_UNIDIRECTIONAL_CONNECTION);

    /* If the VC creates non-duplex connections then the acceptor will
     * need to connect back to form the other half of the connection.
     * This code accepts the return connection. 
     */

    /* Open a port on this side and send it to the acceptor so it can connect back */
    mpi_errno = MPID_Open_port(NULL, connector_port);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
    mpi_errno = MPIDI_CH3I_SHM_write(connect_vc, connector_port, MPI_MAX_PORT_NAME, &num_written);
    if (mpi_errno != MPI_SUCCESS || num_written != MPI_MAX_PORT_NAME) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* extract the tag from the port_name */
    mpi_errno = MPIDI_GetTagFromPort( connector_port, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* dequeue the accept queue to see if a connection with the
       root on the connect side has been formed in the progress
       engine (the connection is returned in the form of a vc). If
       not, poke the progress engine. */

    MPID_Progress_start(&progress_state);
    for(;;)
    {
	MPIDI_CH3I_Acceptq_dequeue(&accept_vc, port_name_tag);
	if (accept_vc != NULL)
	{
	    break;
	}

	mpi_errno = MPID_Progress_wait(&progress_state);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno)
	{
	    MPID_Progress_end(&progress_state);
	    MPIU_ERR_POP(mpi_errno);
	}
	/* --END ERROR HANDLING-- */
    }
    MPID_Progress_end(&progress_state);

    /* merge the newly accepted VC with the connect VC */
    connect_vc->ch.shm_read_queue_info = accept_vc->ch.shm_read_queue_info;
    connect_vc->ch.read_shmq = accept_vc->ch.read_shmq;
    connect_vc->ch.shm_reading_pkt = TRUE;

    /* remove the accept_vc from the reading list and add the connect_vc */
    MPIDI_CH3I_SHM_Remove_vc_references(accept_vc);
    MPIDI_CH3I_SHM_Add_to_reader_list(connect_vc);
    MPIU_Free(accept_vc);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMPLETE_UNIDIRECTIONAL_CONNECTION);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* This routine is the "other side" of 
   MPIDI_CH3_Complete_unidirectional_connection */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Complete_unidirectional_connection2
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Complete_unidirectional_connection2( MPIDI_VC_t *new_vc )
{
    int mpi_errno = MPI_SUCCESS;
    char connector_port[MPI_MAX_PORT_NAME];
    int num_read = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMPLETE_UNIDIRECTIONAL_CONNECTION2);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMPLETE_UNIDIRECTIONAL_CONNECTION2);

    num_read = 0;
    while (num_read == 0)
    {
	mpi_errno = MPIDI_CH3I_SHM_read(new_vc, connector_port, MPI_MAX_PORT_NAME, &num_read);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    /* --BEGIN ERROR HANDLING-- */
    if (num_read != MPI_MAX_PORT_NAME)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", num_read);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    mpi_errno = MPIDI_CH3_Connect_to_root(connector_port, &new_vc);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* restore this vc to the readers list */
    MPIDI_CH3I_SHM_Add_to_reader_list(new_vc);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMPLETE_UNIDIRECTIONAL_CONNECTION2);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* This routine is used to free any channel-specific resources that were
   allocated when building a connection */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Cleanup_after_connection
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Cleanup_after_connection( MPIDI_VC_t *new_vc )
{
    MPIDI_VC_t *iter, *trailer;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_CLEANUP_AFTER_CONNECTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_CLEANUP_AFTER_CONNECTION);

    /* remove the new_vc from the reading list */
    iter = trailer = MPIDI_CH3I_Process.shm_reading_list;
    while (iter != NULL) {
	if (iter == new_vc) {
	    /* First free the resources associated with this VC */
	    MPIDI_CH3I_SHM_Release_mem(&new_vc->ch.shm_read_queue_info);
	    
	    if (trailer != iter) {
		/* remove the new_vc from the list */
		trailer->ch.shm_next_reader = iter->ch.shm_next_reader;
	    }
	    else {
		/* remove the new_vc from the head of the list */
		MPIDI_CH3I_Process.shm_reading_list = MPIDI_CH3I_Process.shm_reading_list->ch.shm_next_reader;
	    }
	}
	if (trailer != iter)
	    trailer = trailer->ch.shm_next_reader;
	iter = iter->ch.shm_next_reader;
    }
    /* remove the new_vc from the writing list */
    iter = trailer = MPIDI_CH3I_Process.shm_writing_list;
    while (iter != NULL) {
	if (iter == new_vc) {
	    /* First free the resources associated with this VC */
	    MPIDI_CH3I_SHM_Release_mem(&new_vc->ch.shm_write_queue_info);
	    
	    if (trailer != iter) {
		/* remove the new_vc from the list */
		trailer->ch.shm_next_writer = iter->ch.shm_next_writer;
	    }
	    else {
		/* remove the new_vc from the head of the list */
		MPIDI_CH3I_Process.shm_writing_list = MPIDI_CH3I_Process.shm_writing_list->ch.shm_next_writer;
	    }
	}
	if (trailer != iter)
	    trailer = trailer->ch.shm_next_writer;
	iter = iter->ch.shm_next_writer;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_CLEANUP_AFTER_CONNECTION);
    return mpi_errno;
}
