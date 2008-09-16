/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*S
 * MPIDI_VCRT - virtual connection reference table
 *
 * handle - this element is not used, but exists so that we may use the 
 * MPIU_Object routines for reference counting
 *
 * ref_count - number of references to this table
 *
 * vcr_table - array of virtual connection references
 S*/
typedef struct MPIDI_VCRT
{
    int handle;
    volatile int ref_count;
    int size;
    MPIDI_VC_t * vcr_table[1];
}
MPIDI_VCRT_t;

/* What is the arrangement of VCRT and VCR and VC? 
   
   Each VC (the virtual connection itself) is refered to by a reference 
   (pointer) or VCR.  
   Each communicator has a VCRT, which is nothing more than a 
   structure containing a count (size) and an array of pointers to 
   virtual connections (as an abstraction, this could be a sparse
   array, allowing a more scalable representation on massively 
   parallel systems).

 */

static int MPIDI_CH3U_VC_FinishPending( MPIDI_VCRT_t *vcrt );

/*@
  MPID_VCRT_Create - Create a table of VC references

  Notes:
  This routine only provides space for the VC references.  Those should
  be added by assigning to elements of the vc array within the 
  'MPID_VCRT' object.
  @*/
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{
    MPIDI_VCRT_t * vcrt;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_CREATE);
    
    vcrt = MPIU_Malloc(sizeof(MPIDI_VCRT_t) + (size - 1) * sizeof(MPIDI_VC_t *));
    if (vcrt != NULL)
    {
	MPIU_Object_set_ref(vcrt, 1);
	vcrt->size = size;
	*vcrt_ptr = vcrt;
    }
    else
    {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**nomem");
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_CREATE);
    return mpi_errno;
}

/*@
  MPID_VCRT_Add_ref - Add a reference to a VC reference table

  Notes:
  This is called when a communicator duplicates its group of processes.
  It is used in 'commutil.c' and in routines to create communicators from
  dynamic process operations.  It does not change the state of any of the
  virtural connections (VCs).
  @*/
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Add_ref
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_ADD_REF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_ADD_REF);
    MPIU_Object_add_ref(vcrt);
    MPIU_DBG_MSG_FMT(REFCOUNT,TYPICAL,(MPIU_DBG_FDEST,
         "Incr VCRT %p ref count to %d",vcrt,vcrt->ref_count));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_ADD_REF);
    return MPI_SUCCESS;
}

/* FIXME: What should this do?  See proc group and vc discussion */

/*@
  MPID_VCRT_Release - Release a reference to a VC reference table

  Notes:
  
  @*/
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Release
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Release(MPID_VCRT vcrt, int isDisconnect )
{
    int in_use;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_RELEASE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_RELEASE);

    MPIU_Object_release_ref(vcrt, &in_use);
    MPIU_DBG_MSG_FMT(REFCOUNT,TYPICAL,(MPIU_DBG_FDEST,
         "Decr VCRT %p ref count to %d",vcrt,vcrt->ref_count));
    
    /* If this VC reference table is no longer in use, we can
       decrement the reference count of each of the VCs.  If the
       count on the VCs goes to zero, then we can decrement the
       ref count on the process group and so on. 
    */
    if (!in_use) {
	int i, inuse;

	/* FIXME: Need a better way to define how vc's are closed that 
	 takes into account pending operations on vcs, including 
	 close events received from other processes. */
	/* mpi_errno = MPIDI_CH3U_VC_FinishPending( vcrt ); */
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

	for (i = 0; i < vcrt->size; i++)
	{
	    MPIDI_VC_t * const vc = vcrt->vcr_table[i];
	    
	    MPIDI_VC_release_ref(vc, &in_use);

            /* Dynamic connections start with a refcount of 2 instead of 1.
             * That way we can distinguish between an MPI_Free and an
             * MPI_Comm_disconnect. */
	    if (isDisconnect && vc->ref_count == 1) {
		MPIDI_VC_release_ref(vc, &in_use);
	    }

	    if (!in_use)
	    {
		/* If the VC is myself then skip the close message */
		if (vc->pg == MPIDI_Process.my_pg && 
		    vc->pg_rank == MPIDI_Process.my_pg_rank)
		{
                    MPIDI_PG_release_ref(vc->pg, &inuse);
                    if (inuse == 0)
                    {
                        MPIDI_PG_Destroy(vc->pg);
                    }
		    continue;
		}
		
		/* FIXME: the correct test is ACTIVE or REMOTE_CLOSE */
		/*if (vc->state != MPIDI_VC_STATE_INACTIVE) { */
		if (vc->state == MPIDI_VC_STATE_ACTIVE ||
		    vc->state == MPIDI_VC_STATE_REMOTE_CLOSE)
		{
		    MPIDI_CH3U_VC_SendClose( vc, i );
		}
		else
		{
                    MPIDI_PG_release_ref(vc->pg, &inuse);
                    if (inuse == 0)
                    {
                        MPIDI_PG_Destroy(vc->pg);
                    }

		    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
                            "vc=%p: not sending a close to %d, vc in state %s",
			     vc, i, MPIDI_VC_GetStateString(vc->state)));
		}

                /* NOTE: we used to * MPIU_CALL(MPIDI_CH3,VC_Destroy(&(pg->vct[i])))
                   here but that is incorrect.  According to the standard, it's
                   entirely possible (likely even) that this VC might still be
                   connected.  VCs are now destroyed when the PG that "owns"
                   them is destroyed (see MPIDI_PG_Destroy). [goodell@ 2008-06-13] */
	    }
	}

	MPIU_Free(vcrt);
    }

 fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_RELEASE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/*@
  MPID_VCRT_Get_ptr - Return a pointer to the array of VCs for this 
  reference table

  Notes:
  This routine is always used with MPID_VCRT_Create and should be 
  combined with it.

  @*/
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Get_ptr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_GET_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_GET_PTR);
    *vc_pptr = vcrt->vcr_table;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_GET_PTR);
    return MPI_SUCCESS;
}

/*@
  MPID_VCR_Dup - Duplicate a virtual connection reference 

  Notes:
  If the VC is being used for the first time in a VC reference
  table, the reference count is set to two, not one, in order to
  distinquish between freeing a communicator with 'MPI_Comm_free' and
  'MPI_Comm_disconnect', and the reference count on the process group
  is incremented (to indicate that the process group is in use).
  While this has no effect on the process group of 'MPI_COMM_WORLD',
  it is important for process groups accessed through 'MPI_Comm_spawn'
  or 'MPI_Comm_connect/MPI_Comm_accept'.
  
  @*/
#undef FUNCNAME
#define FUNCNAME MPID_VCR_Dup
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCR_Dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_DUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_DUP);

    /* We are allowed to create a vc that belongs to no process group 
     as part of the initial connect/accept action, so in that case,
     ignore the pg ref count update */
    if (orig_vcr->ref_count == 0 && orig_vcr->pg) {
	MPIDI_VC_add_ref( orig_vcr );
	MPIDI_VC_add_ref( orig_vcr );
	MPIDI_PG_add_ref( orig_vcr->pg );
    }
    else {
	MPIDI_VC_add_ref(orig_vcr);
    }
    MPIU_DBG_MSG_FMT(REFCOUNT,TYPICAL,(MPIU_DBG_FDEST,\
         "Incr VCR %p ref count to %d",orig_vcr,orig_vcr->ref_count));
    *new_vcr = orig_vcr;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_DUP);
    return MPI_SUCCESS;
}

/*@
  MPID_VCR_Get_lpid - Get the local process ID for a given VC reference
  @*/
#undef FUNCNAME
#define FUNCNAME MPID_VCR_Get_lpid
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCR_Get_lpid(MPID_VCR vcr, int * lpid_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_GET_LPID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_GET_LPID);
    *lpid_ptr = vcr->lpid;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_GET_LPID);
    return MPI_SUCCESS;
}

/* 
 * The following routines convert to/from the global pids, which are 
 * represented as pairs of ints (process group id, rank in that process group)
 */

/* FIXME: These routines belong in a different place */
#undef FUNCNAME
#define FUNCNAME MPID_GPID_GetAllInComm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_GPID_GetAllInComm( MPID_Comm *comm_ptr, int local_size, 
			    int local_gpids[], int *singlePG )
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int *gpid = local_gpids;
    int lastPGID = -1, pgid;
    MPID_VCR vc;
    MPIDI_STATE_DECL(MPID_STATE_MPID_GPID_GETALLINCOMM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_GPID_GETALLINCOMM);

    MPIU_Assert(comm_ptr->local_size == local_size);
    
    *singlePG = 1;
    for (i=0; i<comm_ptr->local_size; i++) {
	vc = comm_ptr->vcr[i];

	/* Get the process group id as an int */
	MPIDI_PG_IdToNum( vc->pg, &pgid );

	*gpid++ = pgid;
	if (lastPGID != pgid) { 
	    if (lastPGID != -1)
		*singlePG = 0;
	    lastPGID = pgid;
	}
	*gpid++ = vc->pg_rank;

        MPIU_DBG_MSG_FMT(COMM,VERBOSE, (MPIU_DBG_FDEST,
                         "pgid=%d vc->pg_rank=%d",
                         pgid, vc->pg_rank));
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_GPID_GETALLINCOMM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_GPID_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_GPID_Get( MPID_Comm *comm_ptr, int rank, int gpid[] )
{
    int      pgid;
    MPID_VCR vc;
    
    vc = comm_ptr->vcr[rank];

    /* Get the process group id as an int */
    MPIDI_PG_IdToNum( vc->pg, &pgid );
    
    gpid[0] = pgid;
    gpid[1] = vc->pg_rank;
    
    return 0;
}

/* 
 * The following is a very simple code for looping through 
 * the GPIDs.  Note that this code requires that all processes
 * have information on the process groups.
 */
#undef FUNCNAME
#define FUNCNAME MPID_GPID_ToLpidArray
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_GPID_ToLpidArray( int size, int gpid[], int lpid[] )
{
    int i, mpi_errno = MPI_SUCCESS;
    int pgid;
    MPIDI_PG_t *pg = 0;
    MPIDI_PG_iterator iter;

    for (i=0; i<size; i++) {
        MPIDI_PG_Get_iterator(&iter);
	do {
	    MPIDI_PG_Get_next( &iter, &pg );
	    if (!pg) {
		/* Internal error.  This gpid is unknown on this process */
		printf("No matching pg foung for id = %d\n", pgid );
		lpid[i] = -1;
		MPIU_ERR_SET2(mpi_errno,MPI_ERR_INTERN, "**unknowngpid",
			      "**unknowngpid %d %d", gpid[0], gpid[1] );
		return mpi_errno;
	    }
	    MPIDI_PG_IdToNum( pg, &pgid );

	    if (pgid == gpid[0]) {
		/* found the process group.  gpid[1] is the rank in 
		   this process group */
		/* Sanity check on size */
		if (pg->size > gpid[1]) {
		    lpid[i] = pg->vct[gpid[1]].lpid;
		}
		else {
		    lpid[i] = -1;
		    MPIU_ERR_SET2(mpi_errno,MPI_ERR_INTERN, "**unknowngpid",
				  "**unknowngpid %d %d", gpid[0], gpid[1] );
		    return mpi_errno;
		}
		/* printf( "lpid[%d] = %d for gpid = (%d)%d\n", i, lpid[i], 
		   gpid[0], gpid[1] ); */
		break;
	    }
	} while (1);
	gpid += 2;
    }

    return mpi_errno;
}

/*@
  MPID_VCR_CommFromLpids - Create a new communicator from a given set
  of lpids.  

  Notes:
  This is used to create a communicator that is not a subset of some
  existing communicator, for example, in a 'MPI_Comm_spawn' or 
  'MPI_Comm_connect/MPI_Comm_accept'.
 @*/
#undef FUNCNAME
#define FUNCNAME MPID_VCR_CommFromLpids
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCR_CommFromLpids( MPID_Comm *newcomm_ptr, 
			    int size, const int lpids[] )
{
    MPID_Comm *commworld_ptr;
    int i;
    MPIDI_PG_iterator iter;

    commworld_ptr = MPIR_Process.comm_world;
    /* Setup the communicator's vc table: remote group */
    MPID_VCRT_Create( size, &newcomm_ptr->vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
    for (i=0; i<size; i++) {
	MPIDI_VC_t *vc = 0;

	/* For rank i in the new communicator, find the corresponding
	   virtual connection.  For lpids less than the size of comm_world,
	   we can just take the corresponding entry from comm_world.
	   Otherwise, we need to search through the process groups.
	*/
	/* printf( "[%d] Remote rank %d has lpid %d\n", 
	   MPIR_Process.comm_world->rank, i, lpids[i] ); */
	if (lpids[i] < commworld_ptr->remote_size) {
	    vc = commworld_ptr->vcr[lpids[i]];
	}
	else {
	    /* We must find the corresponding vcr for a given lpid */	
	    /* For now, this means iterating through the process groups */
	    MPIDI_PG_t *pg = 0;
	    int j;

	    MPIDI_PG_Get_iterator(&iter);
	    /* Skip comm_world */
	    MPIDI_PG_Get_next( &iter, &pg );
	    do {
		MPIDI_PG_Get_next( &iter, &pg );
		if (!pg) {
		    return MPIR_Err_create_code( MPI_SUCCESS, 
				     MPIR_ERR_RECOVERABLE,
				     "MPID_VCR_CommFromLpids", __LINE__,
				     MPI_ERR_INTERN, "**intern", 0 );
		}
		/* FIXME: a quick check on the min/max values of the lpid
		   for this process group could help speed this search */
		for (j=0; j<pg->size; j++) {
		    /*printf( "Checking lpid %d against %d in pg %s\n",
			    lpids[i], pg->vct[j].lpid, (char *)pg->id );
			    fflush(stdout); */
		    if (pg->vct[j].lpid == lpids[i]) {
			vc = &pg->vct[j];
			/*printf( "found vc %x for lpid = %d in another pg\n", 
			  (int)vc, lpids[i] );*/
			break;
		    }
		}
	    } while (!vc);
	}

	/* printf( "about to dup vc %x for lpid = %d in another pg\n", 
	   (int)vc, lpids[i] ); */
	/* Note that his will increment the ref count for the associate
	   PG if necessary.  */
	MPID_VCR_Dup( vc, &newcomm_ptr->vcr[i] );
    }
    return 0;
}

/* The following is a temporary hook to ensure that all processes in 
   a communicator have a set of process groups.
 
   All arguments are input (all processes in comm must have gpids)

   First: all processes check to see if they have information on all
   of the process groups mentioned by id in the array of gpids.

   The local result is LANDed with Allreduce.
   If any process is missing process group information, then the
   root process broadcasts the process group information as a string; 
   each process then uses this information to update to local process group
   information (in the KVS cache that contains information about 
   contacting any process in the process groups).
*/
#undef FUNCNAME
#define FUNCNAME MPID_PG_ForwardPGInfo
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_PG_ForwardPGInfo( MPID_Comm *peer_ptr, MPID_Comm *comm_ptr, 
			   int nPGids, const int gpids[], 
			   int root )
{
    int i, allfound = 1, pgid, pgidWorld;
    MPIDI_PG_t *pg = 0;
    MPIDI_PG_iterator iter;

    /* Get the pgid for CommWorld (always attached to the first process 
       group) */
    MPIDI_PG_Get_iterator(&iter);
    MPIDI_PG_Get_next( &iter, &pg );
    MPIDI_PG_IdToNum( pg, &pgidWorld );
    
    /* Extract the unique process groups */
    for (i=0; i<nPGids && allfound; i++) {
	if (gpids[0] != pgidWorld) {
	    /* Add this gpid to the list of values to check */
	    /* FIXME: For testing, we just test in place */
            MPIDI_PG_Get_iterator(&iter);
	    do {
                MPIDI_PG_Get_next( &iter, &pg );
		if (!pg) {
		    /* We don't know this pgid */
		    allfound = 0;
		    break;
		}
		MPIDI_PG_IdToNum( pg, &pgid );
	    } while (pgid != gpids[0]);
	}
	gpids += 2;
    }

    /* See if everyone is happy */
    NMPI_Allreduce( MPI_IN_PLACE, &allfound, 1, MPI_INT, MPI_LAND, 
		    comm_ptr->handle );

    if (allfound) return MPI_SUCCESS;

    /* FIXME: We need a cleaner way to handle this case than using an ifdef.
       We could have an empty version of MPID_PG_BCast in ch3u_port.c, but
       that's a rather crude way of addressing this problem.  Better is to
       make the handling of local and remote PIDS for the dynamic process
       case part of the dynamic process "module"; devices that don't support
       dynamic processes (and hence have only COMM_WORLD) could optimize for 
       that case */
#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
    /* We need to share the process groups.  We use routines
       from ch3u_port.c */
    MPID_PG_BCast( peer_ptr, comm_ptr, root );
#endif
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_VC_FinishPending
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3U_VC_FinishPending( MPIDI_VCRT_t *vcrt )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t **vc;
    int i, size, nPending;
    MPID_Progress_state progress_state; 
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_VC_FINISHPENDING);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_VC_FINISHPENDING);

    do {
	/* Compute the number of pending ops.
	   A virtual connection has pending operations if the state
	   is not INACTIVE or if the sendq is not null */
	nPending = 0;
	vc       = vcrt->vcr_table;
	size     = vcrt->size;
	/* printf( "Size = %d\n", size ); fflush(stdout); */
	for (i=0; i<size; i++) {
	    if (vc[i]->state != MPIDI_VC_STATE_INACTIVE) {
		/* FIXME: Printf for debugging */
		printf ("state for vc[%d] is %d\n",
			i, vc[i]->state ); fflush(stdout);
		nPending++;
	    }
#if 0
	    /* FIXME: We shouldn't have any references to the channel-specific
	       fields in this part of the code.  This case should actually
	       not be needed; if there is a pending send element, the
	       top-level state should not be inactive */
	    if (vc[i]->ch.sendq_head) {
		/* FIXME: Printf for debugging */
		printf( "Nonempty sendQ for vc[%d]\n", i ); fflush(stdout);
		nPending++;
	    }
#endif
	}
	if (nPending > 0) {
	    printf( "Panic! %d pending operations!\n", nPending );
	    fflush(stdout);
	    MPIU_Assert( nPending == 0 );
	}
	else {
	    break;
	}

	MPID_Progress_start(&progress_state);
	MPIU_DBG_MSG_D(CH3_DISCONNECT,VERBOSE,
		       "Waiting for %d close operations",
		       nPending);
	mpi_errno = MPID_Progress_wait(&progress_state);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS) {
	    MPID_Progress_end(&progress_state);
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				"**ch3|close_progress");
	}
	/* --END ERROR HANDLING-- */
	MPID_Progress_end(&progress_state);
    } while(nPending > 0);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_VC_FINISHPENDING);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/*
 * MPIDI_CH3U_Comm_FinishPending - Complete any pending operations on the 
 * communicator.  
 *
 * Notes: 
 * This should be used before freeing or disconnecting a communicator.
 *
 * For better scalability, we might want to form a list of VC's with 
 * pending operations.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Comm_FinishPending
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Comm_FinishPending( MPID_Comm *comm_ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_COMM_FINISHPENDING);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_COMM_FINISHPENDING);

    mpi_errno = MPIDI_CH3U_VC_FinishPending( comm_ptr->vcrt );
    if (!mpi_errno && comm_ptr->local_vcrt) {
	mpi_errno = MPIDI_CH3U_VC_FinishPending( comm_ptr->local_vcrt );
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_COMM_FINISHPENDING);
    return mpi_errno;
}

/* ----------------------------------------------------------------------- */
/* Routines to initialize a VC */

/*
 * The lpid counter counts new processes that this process knows about.
 */
static int lpid_counter = 0;

/* Fully initialize a VC.  This invokes the channel-specific 
   VC initialization routine MPIDI_CH3_VC_Init . */
int MPIDI_VC_Init( MPIDI_VC_t *vc, MPIDI_PG_t *pg, int rank )
{
    vc->state = MPIDI_VC_STATE_INACTIVE;
    MPIU_Object_set_ref(vc, 0);
    vc->handle  = MPID_VCONN;
    vc->pg      = pg;
    vc->pg_rank = rank;
    vc->lpid    = lpid_counter++;
    MPIDI_VC_Init_seqnum_send(vc);
    MPIDI_VC_Init_seqnum_recv(vc);
    vc->rndvSend_fn           = MPIDI_CH3_RndvSend;
    vc->rndvRecv_fn           = MPIDI_CH3_RecvRndv;
    vc->eager_max_msg_sz      = MPIDI_CH3_EAGER_MAX_MSG_SIZE;
    vc->sendNoncontig_fn      = MPIDI_CH3_SendNoncontig_iov;
    MPIU_CALL(MPIDI_CH3,VC_Init( vc ));
    MPIU_DBG_PrintVCState(vc);

    return MPI_SUCCESS;
}
