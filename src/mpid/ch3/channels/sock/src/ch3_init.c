/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/* Define the ABI version of this channel.  Change this if the channel
   interface (not just the implementation of that interface) changes */
char MPIDI_CH3_ABIVersion[] = "1.1";

/*
 *  MPIDI_CH3_Init  - makes socket specific initializations.  Most of this 
 *                    functionality is in the MPIDI_CH3U_Init_sock upcall 
 *                    because the same tasks need to be done for the ssh 
 *                    (sock + shm) channel.
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t * pg_p, int pg_rank )
{
    int mpi_errno = MPI_SUCCESS;
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

    /* initialize aspects specific to sockets  */
    mpi_errno = MPIDI_CH3U_Init_sock(has_parent, pg_p, pg_rank,
				     &bc_val, &val_max_remaining);

    /* Set the connection information in our process group 
       (publish the business card ) */
    MPIDI_PG_SetConnInfo( pg_rank, (const char *)publish_bc_orig );

    /* Free the business card now that it is published
     (note that publish_bc_orig is the head of bc_val ) */
    MPIDI_CH3I_BCFree( publish_bc_orig );

    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

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
   MPI Port functions */
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns *portFns ATTRIBUTE((unused)) ) 
{
    MPIU_UNREFERENCED_ARG(portFns);
    return 0;
}

/* This function simply tells the CH3 device to use the defaults for the 
   MPI-2 RMA functions */
int MPIDI_CH3_RMAFnsInit( MPIDI_RMAFns *a ATTRIBUTE((unused)) ) 
{
    MPIU_UNREFERENCED_ARG(a);
    return 0;
}

/* Perform the channel-specific vc initialization */
int MPIDI_CH3_VC_Init( MPIDI_VC_t *vc ) {
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    vcch->sendq_head         = NULL;
    vcch->sendq_tail         = NULL;
    vcch->state              = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    MPIDI_VC_InitSock( vc );
    MPIU_DBG_MSG_P(CH3_CONNECT,TYPICAL,"vc=%p: Setting state (ch) to VC_STATE_UNCONNECTED (Initialization)", vc );
    return 0;
}

const char * MPIDI_CH3_VC_GetStateString( struct MPIDI_VC *vc )
{
#ifdef USE_DBG_LOGGING
    return MPIDI_CH3_VC_SockGetStateString( vc );
#else
    return "unknown";
#endif
}

/* Select the routine that uses sockets to connect two communicators
   using a socket */
int MPIDI_CH3_Connect_to_root(const char * port_name, 
			      MPIDI_VC_t ** new_vc)
{
    return MPIDI_CH3I_Connect_to_root_sock( port_name, new_vc );
}

/* This routine is a hook for initializing information for a process
   group before the MPIDI_CH3_VC_Init routine is called */
int MPIDI_CH3_PG_Init( MPIDI_PG_t *pg ATTRIBUTE((unused)) )
{
    return MPI_SUCCESS;
}

/* This routine is a hook for any operations that need to be performed before
   freeing a process group */
int MPIDI_CH3_PG_Destroy( struct MPIDI_PG *pg ATTRIBUTE((unused)) )
{
    return MPI_SUCCESS;
}

/* This routine is a hook for any operations that need to be performed before
   freeing a virtual connection */
int MPIDI_CH3_VC_Destroy( struct MPIDI_VC *vc ATTRIBUTE((unused)) )
{
    return MPI_SUCCESS;
}

/* A dummy function so that all channels provide the same set of functions, 
   enabling dll channels */
int MPIDI_CH3_InitCompleted( void )
{
    return MPI_SUCCESS;
}
