/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
MPIDI_CH3I_Process_t MPIDI_CH3I_Process = {NULL};

/* Define the ABI version of this channel.  Change this if the channel
   interface (not just the implementation of that interface) changes */
char MPIDI_CH3_ABIVersion[] = "1.1";

/* FIXME: These VC debug print routines belong in mpid_vc.c, with 
   channel-specific code in an appropriate file.  Much of that 
   channel-specific code might belong in ch3/util/sock or ch3/util/shm */
#ifdef USE_MPIU_DBG_PRINT_VC

/* VC state printing debugging functions */

static const char * VCState2(MPIDI_VC_State_t state)
{
    static char unknown_state[32];
    switch (state)
    {
    case MPIDI_VC_STATE_INACTIVE:
	return "INACTIVE";
    case MPIDI_VC_STATE_ACTIVE:
	return "ACTIVE";
    case MPIDI_VC_STATE_LOCAL_CLOSE:
	return "LOCAL_CLOSE";
    case MPIDI_VC_STATE_REMOTE_CLOSE:
	return "REMOTE_CLOSE";
    case MPIDI_VC_STATE_CLOSE_ACKED:
	return "CLOSE_ACKED";
    }
    MPIU_Snprintf(unknown_state, 32, "STATE_%d", state);
    return unknown_state;
}

static const char * VCState(MPIDI_VC_t *vc)
{
    return VCState2(vc->state);
}

void MPIU_DBG_PrintVC(MPIDI_VC_t *vc)
{
    printf("vc.state : %s\n", VCState(vc));
    printf("vc.pg_rank : %d\n", vc->pg_rank);
    printf("vc.ref_count: %d\n", vc->ref_count);
    printf("vc.lpid : %d\n", vc->lpid);
    if (vc->pg == NULL)
    {
	printf("vc.pg : NULL\n");
    }
    else
    {
	printf("vc.pg->id : %s\n", vc->pg->id);
	printf("vc.pg->size : %d\n", vc->pg->size);
	printf("vc.pg->ref_count : %d\n", vc->pg->ref_count);
    }
    fflush(stdout);
}

void MPIU_DBG_PrintVCState2(MPIDI_VC_t *vc, MPIDI_VC_State_t new_state)
{
    printf("[%s%d]vc%d.state = %s->%s (%s)\n", MPIU_DBG_parent_str, MPIR_Process.comm_world->rank, vc->pg_rank, VCState(vc), VCState2(new_state), (vc->pg && vc->pg_rank >= 0) ? vc->pg->id : "?");
    fflush(stdout);
}

void MPIU_DBG_PrintVCState(MPIDI_VC_t *vc)
{
    printf("[%s%d]vc%d.state = %s (%s)\n", MPIU_DBG_parent_str, MPIR_Process.comm_world->rank, vc->pg_rank, VCState(vc), (vc->pg && vc->pg_rank >= 0) ? vc->pg->id : "?");
    fflush(stdout);
}

#endif /* USE_MPIU_DBG_PRINT_VC */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t *pg_p, int pg_rank )
{
    int mpi_errno = MPI_SUCCESS;
    int pg_size;
    int p;
    char *publish_bc_orig = NULL;
    char *bc_val = NULL;
    int val_max_remaining;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CH3_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CH3_INIT);

    if (sizeof(MPIDI_CH3I_VC) > 4*MPIDI_CH3_VC_SIZE) {
	printf( "Panic! storage too small (need %d)!\n",
		(int) sizeof(MPIDI_CH3I_VC) );fflush(stdout);
    }
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Initialize the business card */
    mpi_errno = MPIDI_CH3I_BCInit( &bc_val, &val_max_remaining );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    publish_bc_orig = bc_val;

    /* initialize aspects specific to sockets.  do NOT publish business 
       card yet  */
    mpi_errno = MPIDI_CH3U_Init_sock(has_parent, pg_p, pg_rank,
				     &bc_val, &val_max_remaining);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* initialize aspects specific to sshm.  now publish business card   */
    mpi_errno = MPIDI_CH3U_Init_sshm(has_parent, pg_p, pg_rank,
				     &bc_val, &val_max_remaining);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Set the connection information in our process group 
       (publish the business card ) */
    MPIDI_PG_SetConnInfo( pg_rank, publish_bc_orig );

    /* Free the business card now that it is published
     (note that publish_bc_orig is the head of bc_val ) */
    MPIDI_CH3I_BCFree( publish_bc_orig );

    mpi_errno = PMI_Get_size(&pg_size);
    if (mpi_errno != 0) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
			     "**pmi_get_size", "**pmi_get_size %d", mpi_errno);
    }

    /* Allocate and initialize the VC table associated with this process
       group (and thus COMM_WORLD) */
    /* FIXME: This doesn't allocate and only inits one field.  Is this
       now part of the channel-specific hook for channel-specific VC info? */
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)pg_p->vct[p].channel_private;
	vcch->bShm = FALSE;
    }

fn_exit:

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CH3_INIT);
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

/* This function simply tells the CH3 device to use the defaults for the 
   MPI Port functions.  This should be ok here, since we want to 
   use the socket routines to perform all connect/accept actions
   after MPID_Init returns (see the shm_unlink discussion) */
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns *a ) 
{ 
    return 0;
}
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Get_business_card(int myRank, char *value, int length)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_GET_BUSINESS_CARD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_GET_BUSINESS_CARD);

    mpi_errno = MPIDI_CH3U_Get_business_card_sock(myRank, &value, &length);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**buscard");
    }

    mpi_errno = MPIDI_CH3U_Get_business_card_sshm(&value, &length);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**buscard");
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_GET_BUSINESS_CARD);
    return mpi_errno;
}

/* This function simply tells the CH3 device to use the defaults for the 
   MPI-2 RMA functions */
int MPIDI_CH3_RMAFnsInit( MPIDI_RMAFns *a ) 
{ 
    return 0;
}

/* Perform the channel-specific vc initialization */
int MPIDI_CH3_VC_Init( MPIDI_VC_t *vc ) {
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    vcch->sendq_head         = NULL;
    vcch->sendq_tail         = NULL;
    vcch->state              = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    MPIDI_VC_InitSock( vc );
    MPIDI_VC_InitShm( vc );
    /* This variable is used when sock and sshm are combined */
    vcch->bShm               = FALSE;
    return 0;
}

/* Hook routine for any channel-specific actions when a vc is freed. */
int MPIDI_CH3_VC_Destroy( struct MPIDI_VC *vc )
{
    return MPI_SUCCESS;
}

const char * MPIDI_CH3_VC_GetStateString( struct MPIDI_VC *vc )
{
#ifdef USE_DBG_LOGGING
    return MPIDI_CH3_VC_SshmGetStateString( vc );
    /* If the states could be different, we'd need to figure out which
       routine to call */
    /*    return MPIDI_CH3_VC_SockGetStateString( vc );*/
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
int MPIDI_CH3_PG_Init( MPIDI_PG_t *pg )
{
    /* FIXME: This should call a routine from the ch3/util/shm directory
       to initialize the use of shared memory for processes WITHIN this 
       process group */
    return MPI_SUCCESS;
}

/* This routine is a hook for any operations that need to be performed before
   freeing a process group */
int MPIDI_CH3_PG_Destroy( struct MPIDI_PG *pg )
{
    return MPI_SUCCESS;
}

/* A dummy function so that all channels provide the same set of functions, 
   enabling dll channels */
int MPIDI_CH3_InitCompleted( void )
{
    return MPI_SUCCESS;
}
