/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif
#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif
#include <ctype.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH3_NOLOCAL
      category    : CH3
      alt-env     : MPIR_CVAR_CH3_NO_LOCAL
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, force all processes to operate as though all processes
        are located on another node.  For example, this disables shared
        memory communication hierarchical collectives.

    - name        : MPIR_CVAR_CH3_ODD_EVEN_CLIQUES
      category    : CH3
      alt-env     : MPIR_CVAR_CH3_EVEN_ODD_CLIQUES
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, odd procs on a node are seen as local to each other, and even
        procs on a node are seen as local to each other.  Used for debugging on
        a single machine.

    - name        : MPIR_CVAR_CH3_EAGER_MAX_MSG_SIZE
      category    : CH3
      type        : int
      default     : 131072
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This cvar controls the message size at which CH3 switches
        from eager to rendezvous mode.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* What is the arrangement of VCRT and VCR and VC? 
   
   Each VC (the virtual connection itself) is refered to by a reference 
   (pointer) or VCR.  
   Each communicator has a VCRT, which is nothing more than a 
   structure containing a count (size) and an array of pointers to 
   virtual connections (as an abstraction, this could be a sparse
   array, allowing a more scalable representation on massively 
   parallel systems).

 */

/*@
  MPIDI_VCRT_Create - Create a table of VC references

  Notes:
  This routine only provides space for the VC references.  Those should
  be added by assigning to elements of the vc array within the 
  'MPIDI_VCRT' object.
  @*/
#undef FUNCNAME
#define FUNCNAME MPIDI_VCRT_Create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_VCRT_Create(int size, struct MPIDI_VCRT **vcrt_ptr)
{
    MPIDI_VCRT_t * vcrt;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_VCRT_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_VCRT_CREATE);

    MPIU_CHKPMEM_MALLOC(vcrt, MPIDI_VCRT_t *, sizeof(MPIDI_VCRT_t) + (size - 1) * sizeof(MPIDI_VC_t *),	mpi_errno, "**nomem");
    vcrt->handle = HANDLE_SET_KIND(0, HANDLE_KIND_INVALID);
    MPIU_Object_set_ref(vcrt, 1);
    vcrt->size = size;
    *vcrt_ptr = vcrt;

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_VCRT_CREATE);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/*@
  MPIDI_VCRT_Add_ref - Add a reference to a VC reference table

  Notes:
  This is called when a communicator duplicates its group of processes.
  It is used in 'commutil.c' and in routines to create communicators from
  dynamic process operations.  It does not change the state of any of the
  virtural connections (VCs).
  @*/
#undef FUNCNAME
#define FUNCNAME MPIDI_VCRT_Add_ref
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_VCRT_Add_ref(struct MPIDI_VCRT *vcrt)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_VCRT_ADD_REF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_VCRT_ADD_REF);
    MPIU_Object_add_ref(vcrt);
    MPIU_DBG_MSG_FMT(REFCOUNT,TYPICAL,(MPIU_DBG_FDEST, "Incr VCRT %p ref count",vcrt));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_VCRT_ADD_REF);
    return MPI_SUCCESS;
}

/* FIXME: What should this do?  See proc group and vc discussion */

/*@
  MPIDI_VCRT_Release - Release a reference to a VC reference table

  Notes:
  
  @*/
#undef FUNCNAME
#define FUNCNAME MPIDI_VCRT_Release
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_VCRT_Release(struct MPIDI_VCRT *vcrt, int isDisconnect )
{
    int in_use;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_VCRT_RELEASE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_VCRT_RELEASE);

    MPIU_Object_release_ref(vcrt, &in_use);
    MPIU_DBG_MSG_FMT(REFCOUNT,TYPICAL,(MPIU_DBG_FDEST, "Decr VCRT %p ref count",vcrt));
    
    /* If this VC reference table is no longer in use, we can
       decrement the reference count of each of the VCs.  If the
       count on the VCs goes to zero, then we can decrement the
       ref count on the process group and so on. 
    */
    if (!in_use) {
	int i, inuse;

	for (i = 0; i < vcrt->size; i++)
	{
	    MPIDI_VC_t * const vc = vcrt->vcr_table[i];
	    
	    MPIDI_VC_release_ref(vc, &in_use);

            /* Dynamic connections start with a refcount of 2 instead of 1.
             * That way we can distinguish between an MPI_Free and an
             * MPI_Comm_disconnect. */
            /* XXX DJG FIXME-MT should we be checking this? */
            /* probably not, need to do something like the following instead: */
#if 0
            if (isDisconnect) {
                MPIU_Assert(in_use);
                /* FIXME this is still bogus, the VCRT may contain a mix of
                 * dynamic and non-dynamic VCs, so the ref_count isn't
                 * guaranteed to have started at 2.  The best thing to do might
                 * be to avoid overloading the reference counting this way and
                 * use a separate check for dynamic VCs (another flag? compare
                 * PGs?) */
                MPIU_Object_release_ref(vc, &in_use);
            }
#endif
	    if (isDisconnect && MPIU_Object_get_ref(vc) == 1) {
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

                /* NOTE: we used to * MPIDI_CH3_VC_Destroy(&(pg->vct[i])))
                   here but that is incorrect.  According to the standard, it's
                   entirely possible (likely even) that this VC might still be
                   connected.  VCs are now destroyed when the PG that "owns"
                   them is destroyed (see MPIDI_PG_Destroy). [goodell@ 2008-06-13] */
	    }
	}

	MPIU_Free(vcrt);
    }

 fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_VCRT_RELEASE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/*@
  MPIDI_VCR_Dup - Duplicate a virtual connection reference

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
#define FUNCNAME MPIDI_VCR_Dup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_VCR_Dup(MPIDI_VCR orig_vcr, MPIDI_VCR * new_vcr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_DUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_DUP);

    /* We are allowed to create a vc that belongs to no process group 
     as part of the initial connect/accept action, so in that case,
     ignore the pg ref count update */
    /* XXX DJG FIXME-MT should we be checking this? */
    /* we probably need a test-and-incr operation or equivalent to avoid races */
    if (MPIU_Object_get_ref(orig_vcr) == 0 && orig_vcr->pg) {
	MPIDI_VC_add_ref( orig_vcr );
	MPIDI_VC_add_ref( orig_vcr );
	MPIDI_PG_add_ref( orig_vcr->pg );
    }
    else {
	MPIDI_VC_add_ref(orig_vcr);
    }
    MPIU_DBG_MSG_FMT(REFCOUNT,TYPICAL,(MPIU_DBG_FDEST,"Incr VCR %p ref count",orig_vcr));
    *new_vcr = orig_vcr;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_DUP);
    return MPI_SUCCESS;
}

/*@
  MPID_Comm_get_lpid - Get the local process ID for a given VC reference
  @*/
#undef FUNCNAME
#define FUNCNAME MPID_Comm_get_lpid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Comm_get_lpid(MPID_Comm *comm_ptr, int idx, int * lpid_ptr, MPIU_BOOL is_remote)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_GET_LPID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_GET_LPID);

    if (comm_ptr->comm_kind == MPID_INTRACOMM)
        *lpid_ptr = comm_ptr->dev.vcrt->vcr_table[idx]->lpid;
    else if (is_remote)
        *lpid_ptr = comm_ptr->dev.vcrt->vcr_table[idx]->lpid;
    else
        *lpid_ptr = comm_ptr->dev.local_vcrt->vcr_table[idx]->lpid;

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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_GPID_GetAllInComm( MPID_Comm *comm_ptr, int local_size, 
			    MPID_Gpid local_gpids[], int *singlePG )
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int *gpid = (int*)&local_gpids[0];
    int lastPGID = -1, pgid;
    MPIDI_VCR vc;
    MPIDI_STATE_DECL(MPID_STATE_MPID_GPID_GETALLINCOMM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_GPID_GETALLINCOMM);

    MPIU_Assert(comm_ptr->local_size == local_size);
    
    *singlePG = 1;
    for (i=0; i<comm_ptr->local_size; i++) {
	vc = comm_ptr->dev.vcrt->vcr_table[i];

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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_GPID_Get( MPID_Comm *comm_ptr, int rank, MPID_Gpid *in_gpid )
{
    int      pgid;
    MPIDI_VCR vc;
    int*     gpid = (int*)in_gpid;
    vc = comm_ptr->dev.vcrt->vcr_table[rank];

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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_GPID_ToLpidArray( int size, MPID_Gpid in_gpid[], int lpid[] )
{
    int i, mpi_errno = MPI_SUCCESS;
    int pgid;
    MPIDI_PG_t *pg = 0;
    MPIDI_PG_iterator iter;
    int *gpid = (int*)&in_gpid[0];

    for (i=0; i<size; i++) {
        MPIDI_PG_Get_iterator(&iter);
	do {
	    MPIDI_PG_Get_next( &iter, &pg );
	    if (!pg) {
		/* --BEGIN ERROR HANDLING-- */
		/* Internal error.  This gpid is unknown on this process */
		/* A printf is NEVER valid in code that might be executed
		   by the user, even in an error case (use 
		   MPL_internal_error_printf if you need to print
		   an error message and its not appropriate to use the
		   regular error code system */
		/* printf("No matching pg foung for id = %d\n", pgid ); */
		lpid[i] = -1;
		MPIR_ERR_SET2(mpi_errno,MPI_ERR_INTERN, "**unknowngpid",
			      "**unknowngpid %d %d", gpid[0], gpid[1] );
		return mpi_errno;
		/* --END ERROR HANDLING-- */
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
		    /* --BEGIN ERROR HANDLING-- */
		    lpid[i] = -1;
		    MPIR_ERR_SET2(mpi_errno,MPI_ERR_INTERN, "**unknowngpid",
				  "**unknowngpid %d %d", gpid[0], gpid[1] );
		    return mpi_errno;
		    /* --END ERROR HANDLING-- */
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
  MPID_Create_intercomm_from_lpids - Create a new communicator from a given set
  of lpids.  

  Notes:
  This is used to create a communicator that is not a subset of some
  existing communicator, for example, in a 'MPI_Comm_spawn' or 
  'MPI_Comm_connect/MPI_Comm_accept'.  Thus, it is only used for intercommunicators.
 @*/
#undef FUNCNAME
#define FUNCNAME MPID_Create_intercomm_from_lpids
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Create_intercomm_from_lpids( MPID_Comm *newcomm_ptr,
			    int size, const int lpids[] )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *commworld_ptr;
    int i;
    MPIDI_PG_iterator iter;

    commworld_ptr = MPIR_Process.comm_world;
    /* Setup the communicator's vc table: remote group */
    MPIDI_VCRT_Create( size, &newcomm_ptr->dev.vcrt );
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
	    vc = commworld_ptr->dev.vcrt->vcr_table[lpids[i]];
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
                MPIR_ERR_CHKINTERNAL(!pg, mpi_errno, "no pg");
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
	MPIDI_VCR_Dup( vc, &newcomm_ptr->dev.vcrt->vcr_table[i] );
    }
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_PG_ForwardPGInfo( MPID_Comm *peer_ptr, MPID_Comm *comm_ptr, 
			   int nPGids, const MPID_Gpid in_gpids[],
			   int root )
{
    int mpi_errno = MPI_SUCCESS;
    int i, allfound = 1, pgid, pgidWorld;
    MPIDI_PG_t *pg = 0;
    MPIDI_PG_iterator iter;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    
    const int *gpids = (const int*)&in_gpids[0];

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
    mpi_errno = MPIR_Allreduce_impl( MPI_IN_PLACE, &allfound, 1, MPI_INT, MPI_LAND, comm_ptr, &errflag );
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    
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
 fn_exit:
    return MPI_SUCCESS;
 fn_fail:
    goto fn_exit;
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
    vc->handle  = HANDLE_SET_MPI_KIND(0, MPID_VCONN);
    MPIU_Object_set_ref(vc, 0);
    vc->pg      = pg;
    vc->pg_rank = rank;
    vc->lpid    = lpid_counter++;
    vc->node_id = -1;
    MPIDI_VC_Init_seqnum_send(vc);
    MPIDI_VC_Init_seqnum_recv(vc);
    vc->rndvSend_fn      = MPIDI_CH3_RndvSend;
    vc->rndvRecv_fn      = MPIDI_CH3_RecvRndv;
    vc->ready_eager_max_msg_sz = -1; /* no limit */;
    vc->eager_max_msg_sz = MPIR_CVAR_CH3_EAGER_MAX_MSG_SIZE;

    vc->sendNoncontig_fn = MPIDI_CH3_SendNoncontig_iov;
#ifdef ENABLE_COMM_OVERRIDES
    vc->comm_ops         = NULL;
#endif
    /* FIXME: We need a better abstraction for initializing the thread state 
       for an object */
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT
    {
        int err;
        MPID_Thread_mutex_create(&vc->pobj_mutex,&err);
        MPIU_Assert(err == 0);
    }
#endif /* MPICH_THREAD_GRANULARITY */
    MPIDI_CH3_VC_Init(vc);
    MPIDI_DBG_PrintVCState(vc);

    return MPI_SUCCESS;
}

/* ----------------------------------------------------------------------- */
/* Routines to vend topology information. */

static MPID_Node_id_t g_max_node_id = -1;
char MPIU_hostname[MAX_HOSTNAME_LEN] = "_UNKNOWN_"; /* '_' is an illegal char for a hostname so */
                                                    /* this will never match */

#undef FUNCNAME
#define FUNCNAME MPID_Get_node_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Get_node_id(MPID_Comm *comm, int rank, MPID_Node_id_t *id_p)
{
    *id_p = comm->dev.vcrt->vcr_table[rank]->node_id;
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_Get_max_node_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* Providing a comm argument permits optimization, but this function is always
   allowed to return the max for the universe. */
int MPID_Get_max_node_id(MPID_Comm *comm, MPID_Node_id_t *max_id_p)
{
    /* easiest way to implement this is to track it at PG create/destroy time */
    *max_id_p = g_max_node_id;
    MPIU_Assert(*max_id_p >= 0);
    return MPI_SUCCESS;
}

#if !defined(USE_PMI2_API)
/* this function is not used in pmi2 */
static int publish_node_id(MPIDI_PG_t *pg, int our_pg_rank)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int ret;
    char *key;
    int key_max_sz;
    char *kvs_name;
    MPIU_CHKLMEM_DECL(1);

    /* set MPIU_hostname */
    ret = gethostname(MPIU_hostname, MAX_HOSTNAME_LEN);
    MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock_gethost", "**sock_gethost %s %d", MPIU_Strerror(errno), errno);
    MPIU_hostname[MAX_HOSTNAME_LEN-1] = '\0';

    /* Allocate space for pmi key */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);

    MPIU_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

    mpi_errno = MPIDI_PG_GetConnKVSname(&kvs_name);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* Put my hostname id */
    if (pg->size > 1)
    {
        memset(key, 0, key_max_sz);
        MPL_snprintf(key, key_max_sz, "hostname[%d]", our_pg_rank);

        pmi_errno = PMI_KVS_Put(kvs_name, key, MPIU_hostname);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);

        pmi_errno = PMI_KVS_Commit(kvs_name);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);

        pmi_errno = PMI_Barrier();
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
    }
    
fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
#endif


#define parse_error() MPIR_ERR_INTERNALANDJUMP(mpi_errno, "parse error")
/* advance _c until we find a non whitespace character */
#define skip_space(_c) while (isspace(*(_c))) ++(_c)
/* return true iff _c points to a character valid as an indentifier, i.e., [-_a-zA-Z0-9] */
#define isident(_c) (isalnum(_c) || (_c) == '-' || (_c) == '_')

/* give an error iff *_c != _e */
#define expect_c(_c, _e) do { if (*(_c) != _e) parse_error(); } while (0)
#define expect_and_skip_c(_c, _e) do { expect_c(_c, _e); ++c; } while (0)
/* give an error iff the first |_m| characters of the string _s are equal to _e */
#define expect_s(_s, _e) (strncmp(_s, _e, strlen(_e)) == 0 && !isident((_s)[strlen(_e)]))

typedef enum {
    UNKNOWN_MAPPING = -1,
    NULL_MAPPING = 0,
    VECTOR_MAPPING
} mapping_type_t;

#define VECTOR "vector"

typedef struct map_block
{
    int start_id;
    int count;
    int size;
} map_block_t;

#undef FUNCNAME
#define FUNCNAME parse_mapping
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int parse_mapping(char *map_str, mapping_type_t *type, map_block_t **map, int *nblocks)
{
    int mpi_errno = MPI_SUCCESS;
    char *c = map_str, *d;
    int num_blocks = 0;
    int i;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_PARSE_MAPPING);

    MPIDI_FUNC_ENTER(MPID_STATE_PARSE_MAPPING);

    /* parse string of the form:
       '(' <format> ',' '(' <num> ',' <num> ',' <num> ')' {',' '(' <num> ',' <num> ',' <num> ')'} ')'

       the values of each 3-tuple have the following meaning (X,Y,Z):
         X - node id start value
         Y - number of nodes with size Z
         Z - number of processes assigned to each node
     */
    MPIU_DBG_MSG_S(CH3_OTHER,VERBOSE,"parsing mapping string '%s'", map_str);

    if (!strlen(map_str)) {
        /* An empty-string indicates an inability to determine or express the
         * process layout on the part of the process manager.  Consider this a
         * non-fatal error case. */
        *type = NULL_MAPPING;
        *map = NULL;
        *nblocks = 0;
        goto fn_exit;
    }

    skip_space(c);
    expect_and_skip_c(c, '(');
    skip_space(c);

    d = c;
    if (expect_s(d, VECTOR))
        *type = VECTOR_MAPPING;
    else
        parse_error();
    c += strlen(VECTOR);
    skip_space(c);

    /* first count the number of block descriptors */
    d = c;
    while (*d) {
        if (*d == '(')
            ++num_blocks;
        ++d;
    }

    MPIU_CHKPMEM_MALLOC(*map, map_block_t *, sizeof(map_block_t) * num_blocks, mpi_errno, "map");

    /* parse block descriptors */
    for (i = 0; i < num_blocks; ++i) {
        expect_and_skip_c(c, ',');
        skip_space(c);

        expect_and_skip_c(c, '(');
        skip_space(c);

        if (!isdigit(*c))
            parse_error();
        (*map)[i].start_id = (int)strtol(c, &c, 0);
        skip_space(c);

        expect_and_skip_c(c, ',');
        skip_space(c);

        if (!isdigit(*c))
            parse_error();
        (*map)[i].count = (int)strtol(c, &c, 0);
        skip_space(c);

        expect_and_skip_c(c, ',');
        skip_space(c);

        if (!isdigit(*c))
            parse_error();
        (*map)[i].size = (int)strtol(c, &c, 0);

        expect_and_skip_c(c, ')');
        skip_space(c);
    }

    expect_and_skip_c(c, ')');

    *nblocks = num_blocks;
    MPIU_CHKPMEM_COMMIT();
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_PARSE_MAPPING);
    return mpi_errno;
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#if 0
static void t(const char *s, int nprocs)
{
    int ret;
    map_block_t *mb;
    int nblocks=0;
    int i;
    mapping_type_t mt = UNKNOWN_MAPPING;
    int rank;
    int block, block_node, node_proc;

    ret = parse_mapping(strdup(s), &mt, &mb, &nblocks);
    printf("str=\"%s\" type=%d ret=%d\n", s, mt, ret);
    if (ret) return;
    for (i = 0; i < nblocks; ++i)
        printf("    %d: start=%d size=%d count=%d\n", i, mb[i].start_id, mb[i].size, mb[i].count);
    printf("\n");


    rank = 0;
    while (rank < nprocs) {
        int node_id;
        for (block = 0; block < nblocks; ++block) {
            node_id = mb[block].start_id;
            for (block_node = 0; block_node < mb[block].count; ++block_node) {
                for (node_proc = 0; node_proc < mb[block].size; ++node_proc) {
                    printf("    %d  %d\n", rank, node_id);
                    ++rank;
                    if (rank == nprocs)
                        goto done;
                }
                ++node_id;
            }
        }
    }
done:
    return;

}


 void test_parse_mapping(void)
{
    t("(vector, (0,1,1))", 5);
    t("(vector, (0,1,1), (1,5,3), (6,2, 5))", 100);
    t("(vector, (1,1,1), (0,2,2))", 5);
    
    t("(vector, (1,1,1), (0,2,2),)", 5);
    t("XXX, (1,1))", 1);
    t("vector, (1,1))", 1);
    t("(vector, (1.11, 2,2))", 1);
    t("", 1);

}


#endif

#undef FUNCNAME
#define FUNCNAME populate_ids_from_mapping
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int populate_ids_from_mapping(char *mapping, MPID_Node_id_t *max_node_id, MPIDI_PG_t *pg, int *did_map)
{
    int mpi_errno = MPI_SUCCESS;
    /* PMI_process_mapping is available */
    mapping_type_t mt = UNKNOWN_MAPPING;
    map_block_t *mb = NULL;
    int nblocks = 0;
    int rank;
    int block, block_node, node_proc;
    int *tmp_rank_list, i;
    int found_wrap;

    *did_map = 1; /* reset upon failure */

    mpi_errno = parse_mapping(mapping, &mt, &mb, &nblocks);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    if (NULL_MAPPING == mt) goto fn_fail;
    MPIR_ERR_CHKINTERNAL(mt != VECTOR_MAPPING, mpi_errno, "unsupported mapping type");

    /* allocate nodes to ranks */
    found_wrap = 0;
    for (rank = 0;;) {
        /* FIXME: The patch is hacky because it assumes that seeing a
         * start node ID of 0 means a wrap around.  This is not
         * necessarily true.  A user-defined node list can, in theory,
         * use the node ID 0 without actually creating a wrap around.
         * The reason this patch still works in this case is because
         * Hydra creates a new node list starting from node ID 0 for
         * user-specified nodes during MPI_Comm_spawn{_multiple}.  If
         * a different process manager searches for allocated nodes in
         * the user-specified list, this patch will break. */

        /* If we found that the blocks wrap around, repeat loops
         * should only start at node id 0 */
        for (block = 0; found_wrap && mb[block].start_id; block++);

        for (; block < nblocks; block++) {
            if (mb[block].start_id == 0)
                found_wrap = 1;
            for (block_node = 0; block_node < mb[block].count; block_node++) {
                for (node_proc = 0; node_proc < mb[block].size; node_proc++) {
                    pg->vct[rank].node_id = mb[block].start_id + block_node;
                    if (++rank == pg->size)
                        goto break_out;
                }
            }
        }
    }

 break_out:
    /* identify maximum node id */
    *max_node_id = -1;
    for (i = 0; i < pg->size; i++)
        if (pg->vct[i].node_id + 1 > *max_node_id)
            *max_node_id = pg->vct[i].node_id;

fn_exit:
    MPIU_Free(mb);
    return mpi_errno;
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    *did_map = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* Fills in the node_id info from PMI info.  Adapted from MPIU_Get_local_procs.
   This function is collective over the entire PG because PMI_Barrier is called.

   our_pg_rank should be set to -1 if this is not the current process' PG.  This
   is currently not supported due to PMI limitations.

   Fallback Algorithm:

   Each process kvs_puts its hostname and stores the total number of
   processes (g_num_global).  Each process determines maximum node id
   (g_max_node_id) and assigns a node id to each process (g_node_ids[]):

     For each hostname the process seaches the list of unique nodes
     names (node_names[]) for a match.  If a match is found, the node id
     is recorded for that matching process.  Otherwise, the hostname is
     added to the list of node names.
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_Populate_vc_node_ids
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_Populate_vc_node_ids(MPIDI_PG_t *pg, int our_pg_rank)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int i, j;
    char *key;
    char *value;
    int key_max_sz;
    int val_max_sz;
    char *kvs_name;
    char **node_names;
    char *node_name_buf;
    int no_local = 0;
    int odd_even_cliques = 0;
    int pmi_version = MPIDI_CH3I_DEFAULT_PMI_VERSION;
    int pmi_subversion = MPIDI_CH3I_DEFAULT_PMI_SUBVERSION;
    MPIU_CHKLMEM_DECL(4);

    /* See if the user wants to override our default values */
    MPL_env2int("PMI_VERSION", &pmi_version);
    MPL_env2int("PMI_SUBVERSION", &pmi_subversion);

    if (pg->size == 1) {
        pg->vct[0].node_id = ++g_max_node_id;
        goto fn_exit;
    }

    /* Used for debugging only.  This disables communication over shared memory */
#ifdef ENABLED_NO_LOCAL
    no_local = 1;
#else
    no_local = MPIR_CVAR_CH3_NOLOCAL;
#endif

    /* Used for debugging on a single machine: Odd procs on a node are
       seen as local to each other, and even procs on a node are seen
       as local to each other. */
#ifdef ENABLED_ODD_EVEN_CLIQUES
    odd_even_cliques = 1;
#else
    odd_even_cliques = MPIR_CVAR_CH3_ODD_EVEN_CLIQUES;
#endif

    if (no_local) {
        /* just assign 0 to n-1 as node ids and bail */
        for (i = 0; i < pg->size; ++i) {
            pg->vct[i].node_id = ++g_max_node_id;
        }
        goto fn_exit;
    }

#ifdef USE_PMI2_API
    {
        char process_mapping[PMI2_MAX_VALLEN];
        int outlen;
        int found = FALSE;
        int i;
        map_block_t *mb;
        int nblocks;
        int rank;
        int block, block_node, node_proc;
        int did_map = 0;

        mpi_errno = PMI2_Info_GetJobAttr("PMI_process_mapping", process_mapping, sizeof(process_mapping), &found);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKINTERNAL(!found, mpi_errno, "PMI_process_mapping attribute not found");
        /* this code currently assumes pg is comm_world */
        mpi_errno = populate_ids_from_mapping(process_mapping, &g_max_node_id, pg, &did_map);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKINTERNAL(!did_map, mpi_errno, "unable to populate node ids from PMI_process_mapping");
    }
#else /* USE_PMI2_API */
    if (our_pg_rank == -1) {
        /* FIXME this routine can't handle the dynamic process case at this
           time.  This will require more support from the process manager. */
        MPIU_Assert(0);
    }

    /* Allocate space for pmi key and value */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    MPIU_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    MPIU_CHKLMEM_MALLOC(value, char *, val_max_sz, mpi_errno, "value");

    mpi_errno = MPIDI_PG_GetConnKVSname(&kvs_name);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* See if process manager supports PMI_process_mapping keyval */

    if (pmi_version == 1 && pmi_subversion == 1) {
        pmi_errno = PMI_KVS_Get(kvs_name, "PMI_process_mapping", value, val_max_sz);
        if (pmi_errno == 0) {
            int did_map = 0;
            /* this code currently assumes pg is comm_world */
            mpi_errno = populate_ids_from_mapping(value, &g_max_node_id, pg, &did_map);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            if (did_map) {
                goto odd_even_cliques;
            }
            else {
                MPIU_DBG_MSG_S(CH3_OTHER,TERSE,"did_map==0, unable to populate node ids from mapping=%s",value);
            }
            /* else fall through to O(N^2) PMI_KVS_Gets version */
        }
        else {
            MPIU_DBG_MSG(CH3_OTHER,TERSE,"unable to obtain the 'PMI_process_mapping' PMI key");
        }
    }

    mpi_errno = publish_node_id(pg, our_pg_rank);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* Allocate temporary structures.  These would need to be persistent if
       we somehow were able to support dynamic processes via this method. */
    MPIU_CHKLMEM_MALLOC(node_names, char **, pg->size * sizeof(char*), mpi_errno, "node_names");
    MPIU_CHKLMEM_MALLOC(node_name_buf, char *, pg->size * key_max_sz * sizeof(char), mpi_errno, "node_name_buf");

    /* Gather hostnames */
    for (i = 0; i < pg->size; ++i)
    {
        node_names[i] = &node_name_buf[i * key_max_sz];
        node_names[i][0] = '\0';
    }

    g_max_node_id = 0; /* defensive */

    for (i = 0; i < pg->size; ++i)
    {
        MPIU_Assert(g_max_node_id < pg->size);
        if (i == our_pg_rank)
        {
            /* This is us, no need to perform a get */
            MPL_snprintf(node_names[g_max_node_id], key_max_sz, "%s", MPIU_hostname);
        }
        else
        {
            memset(key, 0, key_max_sz);
            MPL_snprintf(key, key_max_sz, "hostname[%d]", i);

            pmi_errno = PMI_KVS_Get(kvs_name, key, node_names[g_max_node_id], key_max_sz);
            MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
        }

        /* Find the node_id for this process, or create a new one */
        /* FIXME:need a better algorithm -- this one does O(N^2) strncmp()s! */
        /* The right fix is to get all this information from the process
           manager, rather than bother with this hostname hack at all. */
        for (j = 0; j < g_max_node_id + 1; ++j)
            if (!strncmp(node_names[j], node_names[g_max_node_id], key_max_sz))
                break;
        if (j == g_max_node_id + 1)
            ++g_max_node_id;
        else
            node_names[g_max_node_id][0] = '\0';
        pg->vct[i].node_id = j;
    }

odd_even_cliques:
    if (odd_even_cliques)
    {
        /* Create new processes for all odd numbered processes. This
           may leave nodes ids with no processes assigned to them, but
           I think this is OK */
        for (i = 0; i < pg->size; ++i)
            if (i & 0x1)
                pg->vct[i].node_id += g_max_node_id + 1;
        g_max_node_id = (g_max_node_id + 1) * 2;
    }
#endif

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

