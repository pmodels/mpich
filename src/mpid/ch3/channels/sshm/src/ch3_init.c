/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

MPIDI_CH3I_Process_t MPIDI_CH3I_Process = {NULL};

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t * pg_p, int pg_rank )
{
    int mpi_errno;
    char *publish_bc_orig = NULL;
    char *bc_val = NULL;
    int val_max_remaining;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CH3_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CH3_INIT);

    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Initialize the business card */
    mpi_errno = MPIDI_CH3I_BCInit( &bc_val, &val_max_remaining );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    publish_bc_orig = bc_val;

    mpi_errno = MPIDI_CH3U_Init_sshm(has_parent, pg_p, pg_rank,
				     &bc_val, &val_max_remaining);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);


    /* Free the business card now that it is published
     (note that publish_bc_orig is the head of bc_val ) */
    MPIDI_CH3I_BCFree( publish_bc_orig );

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CH3_INIT);
    return mpi_errno;
 fn_fail:
    if (publish_bc_orig != NULL) {
        MPIU_Free(publish_bc_orig);
    }           
    goto fn_exit;
}

/* This function simply tells the CH3 device to use the defaults for the 
   MPI Port functions.  This should be ok here, since we want to 
   use the socket routines to perform all connect/accept actions
   after MPID_Init returns (see the shm_unlink discussion) */
int MPIDI_CH3_PortFnsInit(MPIDI_PortFns *portFns) 
{
#ifdef USE_PERSISTENT_SHARED_MEMORY
    MPIU_UNREFERENCED_ARG(portFns);
    return MPI_SUCCESS;
#else
    portFns->OpenPort    = 0;
    portFns->ClosePort   = 0;
    portFns->CommAccept  = 0;
    portFns->CommConnect = 0;
    return MPI_SUCCESS;
#endif
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Get_business_card(char *value, int length)
{
    return MPIDI_CH3U_Get_business_card_sshm(&value, &length);
}

/* Perform the channel-specific vc initialization */
int MPIDI_CH3_VC_Init( MPIDI_VC_t *vc )
{
    vc->ch.sendq_head         = NULL;
    vc->ch.sendq_tail         = NULL;
    vc->ch.recv_active        = NULL;
    vc->ch.send_active        = NULL;
    vc->ch.req                = NULL;
    vc->ch.read_shmq          = NULL;
    vc->ch.write_shmq         = NULL;
    vc->ch.shm                = NULL;
    vc->ch.shm_state          = 0;
    vc->ch.state              = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    vc->ch.shm_read_connected = 0;
    vc->ch.shm_reading_pkt    = 0;
    return 0;
}

/* Select the routine that uses shared memory to connect two communicators
   using a socket */
int MPIDI_CH3_Connect_to_root(const char * port_name, 
			      MPIDI_VC_t ** new_vc)
{
    return MPIDI_CH3I_Connect_to_root_sshm( port_name, new_vc );
}

/* This routine is a hook for initializing information for a process
   group before the MPIDI_CH3_VC_Init routine is called */
int MPIDI_CH3_PG_Init( MPIDI_PG_t *pg )
{
    /* FIXME: This should call a routine from the ch3/util/shm directory
       to initialize the use of shared memory for processes WITHIN this 
       process group */
    return MPI_SUCCESS;
}

int MPIDI_CH3_RMAFnsInit( MPIDI_RMAFns *RMAFns ) 
{ 
    RMAFns->Win_create = MPIDI_CH3_Win_create;
    RMAFns->Win_free = MPIDI_CH3_Win_free;
    RMAFns->Put = MPIDI_CH3_Put;
    RMAFns->Get = MPIDI_CH3_Get;
    RMAFns->Accumulate = MPIDI_CH3_Accumulate;
    RMAFns->Win_fence = MPIDI_CH3_Win_fence;
    RMAFns->Win_post = MPIDI_CH3_Win_post;
    RMAFns->Win_start = MPIDI_CH3_Win_start;
    RMAFns->Win_complete = MPIDI_CH3_Win_complete;
    RMAFns->Win_wait = MPIDI_CH3_Win_wait;
    RMAFns->Win_lock = MPIDI_CH3_Win_lock;
    RMAFns->Win_unlock = MPIDI_CH3_Win_unlock;
    RMAFns->Alloc_mem = MPIDI_CH3_Alloc_mem;
    RMAFns->Free_mem = MPIDI_CH3_Free_mem;

    return MPI_SUCCESS;
}

int MPIDI_CH3_VC_Init( struct MPIDI_VC *vc_ptr )
{
    vc_ptr->ch.sendq_head = NULL;
    vc_ptr->ch.sendq_tail = NULL;
    vc_ptr->ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    vc_ptr->ch.recv_active = NULL;
    vc_ptr->ch.send_active = NULL;
    vc_ptr->ch.req = NULL;
    vc_ptr->ch.read_shmq = NULL;
    vc_ptr->ch.write_shmq = NULL;
    vc_ptr->ch.shm = NULL;
    vc_ptr->ch.shm_state = 0;
    vc_ptr->ch.shm_next_reader = NULL;
    vc_ptr->ch.shm_next_writer = NULL;
    vc_ptr->ch.shm_read_connected = 0;
    
    return MPI_SUCCESS;
}
