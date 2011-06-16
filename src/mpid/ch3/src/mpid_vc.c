/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
    MPIU_OBJECT_HEADER; /* adds handle and ref_count fields */
    int contains_failed_vc;
    int last_check_for_failed_vc;
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
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_CREATE);

    MPIU_CHKPMEM_MALLOC(vcrt, MPIDI_VCRT_t *, sizeof(MPIDI_VCRT_t) + (size - 1) * sizeof(MPIDI_VC_t *),	mpi_errno, "**nomem");
    vcrt->handle = HANDLE_SET_KIND(0, HANDLE_KIND_INVALID);
    MPIU_Object_set_ref(vcrt, 1);
    vcrt->size = size;
    *vcrt_ptr = vcrt;
    vcrt->contains_failed_vc = FALSE;
    vcrt->last_check_for_failed_vc = 0;

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_CREATE);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
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
    MPIU_DBG_MSG_FMT(REFCOUNT,TYPICAL,(MPIU_DBG_FDEST, "Incr VCRT %p ref count",vcrt));
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
    MPIU_DBG_MSG_FMT(REFCOUNT,TYPICAL,(MPIU_DBG_FDEST, "Decr VCRT %p ref count",vcrt));
    
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
  MPID_VCRT_Contains_failed_vc - returns TRUE iff a VC in this VCRT is in MORUBIND state
  @*/
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Contains_failed_vc
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_VCRT_Contains_failed_vc(MPID_VCRT vcrt)
{
    if (vcrt->contains_failed_vc) {
        /* We have already determined that this VCRT has a dead VC */
        return TRUE;
    } else if (vcrt->last_check_for_failed_vc < MPIDI_Failed_vc_count) {
        /* A VC has failed since the last time we checked for dead VCs
           in this VCRT */
        int i;
        for (i = 0; i < vcrt->size; ++i) {
            if (vcrt->vcr_table[i]->state == MPIDI_VC_STATE_MORIBUND) {
                vcrt->contains_failed_vc = TRUE;
                return TRUE;
            }
        }
        vcrt->last_check_for_failed_vc = MPIDI_Failed_vc_count;
    }
    return FALSE;
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
    int mpi_errno = MPI_SUCCESS;
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
                MPIU_ERR_CHKINTERNAL(!pg, mpi_errno, "no pg");
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_PG_ForwardPGInfo( MPID_Comm *peer_ptr, MPID_Comm *comm_ptr, 
			   int nPGids, const int gpids[], 
			   int root )
{
    int mpi_errno = MPI_SUCCESS;
    int i, allfound = 1, pgid, pgidWorld;
    MPIDI_PG_t *pg = 0;
    MPIDI_PG_iterator iter;
    int errflag = FALSE;
    
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
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    
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
    vc->eager_max_msg_sz = MPIDI_CH3_EAGER_MAX_MSG_SIZE;
    vc->sendNoncontig_fn = MPIDI_CH3_SendNoncontig_iov;
#ifdef ENABLE_COMM_OVERRIDES
    vc->comm_ops         = NULL;
#endif
    /* FIXME: We need a better abstraction for initializing the thread state 
       for an object */
#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT
    MPID_Thread_mutex_create(&vc->pobj_mutex,NULL);
#endif /* MPIU_THREAD_GRANULARITY */
    MPIU_CALL(MPIDI_CH3,VC_Init( vc ));
    MPIU_DBG_PrintVCState(vc);

    return MPI_SUCCESS;
}

/* ----------------------------------------------------------------------- */
/* Routines to vend topology information. */

static MPID_Node_id_t g_num_nodes = 0;
char MPIU_hostname[MAX_HOSTNAME_LEN] = "_UNKNOWN_"; /* '_' is an illegal char for a hostname so */
                                                    /* this will never match */

#undef FUNCNAME
#define FUNCNAME MPID_Get_node_id
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Get_node_id(MPID_Comm *comm, int rank, MPID_Node_id_t *id_p)
{
    *id_p = comm->vcr[rank]->node_id;
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_Get_max_node_id
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* Providing a comm argument permits optimization, but this function is always
   allowed to return the max for the universe. */
int MPID_Get_max_node_id(MPID_Comm *comm, MPID_Node_id_t *max_id_p)
{
    /* easiest way to implement this is to track it at PG create/destroy time */
    *max_id_p = g_num_nodes - 1;
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
    MPIU_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock_gethost", "**sock_gethost %s %d", MPIU_Strerror(errno), errno);
    MPIU_hostname[MAX_HOSTNAME_LEN-1] = '\0';

    /* Allocate space for pmi key */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);

    MPIU_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

    mpi_errno = MPIDI_PG_GetConnKVSname(&kvs_name);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Put my hostname id */
    if (pg->size > 1)
    {
        memset(key, 0, key_max_sz);
        MPIU_Snprintf(key, key_max_sz, "hostname[%d]", our_pg_rank);

        pmi_errno = PMI_KVS_Put(kvs_name, key, MPIU_hostname);
        MPIU_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);

        pmi_errno = PMI_KVS_Commit(kvs_name);
        MPIU_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);

        pmi_errno = PMI_Barrier();
        MPIU_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
    }
    
fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
#endif


#define parse_error() MPIU_ERR_INTERNALANDJUMP(mpi_errno, "parse error")
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
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
        (*map)[i].start_id = strtol(c, &c, 0);
        skip_space(c);

        expect_and_skip_c(c, ',');
        skip_space(c);

        if (!isdigit(*c))
            parse_error();
        (*map)[i].count = strtol(c, &c, 0);
        skip_space(c);

        expect_and_skip_c(c, ',');
        skip_space(c);

        if (!isdigit(*c))
            parse_error();
        (*map)[i].size = strtol(c, &c, 0);

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
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int populate_ids_from_mapping(char *mapping, int *num_nodes, MPIDI_PG_t *pg, int *did_map)
{
    int mpi_errno = MPI_SUCCESS;
    /* PMI_process_mapping is available */
    mapping_type_t mt = UNKNOWN_MAPPING;
    map_block_t *mb = NULL;
    int nblocks = 0;
    int rank;
    int block, block_node, node_proc;

    *did_map = 1; /* reset upon failure */

    mpi_errno = parse_mapping(mapping, &mt, &mb, &nblocks);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (NULL_MAPPING == mt) goto fn_fail;
    MPIU_ERR_CHKINTERNAL(mt != VECTOR_MAPPING, mpi_errno, "unsupported mapping type");

    rank = 0;
    /* for a representation like (block,N,(1,1)) this while loop causes us to
     * re-use that sole map block over and over until we have assigned node
     * ids to every process */
    while (rank < pg->size) {
        for (block = 0; block < nblocks; ++block) {
            int node_id = mb[block].start_id;
            for (block_node = 0; block_node < mb[block].count; ++block_node) {
                if (node_id > *num_nodes)
                    *num_nodes = node_id;

                for (node_proc = 0; node_proc < mb[block].size; ++node_proc) {
                    pg->vct[rank].node_id = node_id;
                    ++rank;
                    if (rank == pg->size)
                        goto map_done;
                }
                ++node_id;
            }
        }
    }

map_done:
    ++(*num_nodes); /* add one to get the num instead of the max */
fn_exit:
    MPIU_Free(mb);
    return mpi_errno;
fn_fail:
    *did_map = 0;
    goto fn_exit;
}

/* Fills in the node_id info from PMI info.  Adapted from MPIU_Get_local_procs.
   This function is collective over the entire PG because PMI_Barrier is called.

   our_pg_rank should be set to -1 if this is not the current process' PG.  This
   is currently not supported due to PMI limitations.

   Fallback Algorithm:

   Each process kvs_puts its hostname and stores the total number of
   processes (g_num_global).  Each process determines the number of nodes
   (g_num_nodes) and assigns a node id to each process (g_node_ids[]):

     For each hostname the process seaches the list of unique nodes
     names (node_names[]) for a match.  If a match is found, the node id
     is recorded for that matching process.  Otherwise, the hostname is
     added to the list of node names.
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_Populate_vc_node_ids
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
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
    int pmi_version = MPIU_DEFAULT_PMI_VERSION, pmi_subversion = MPIU_DEFAULT_PMI_SUBVERSION;
    MPIU_CHKLMEM_DECL(4);

    /* See if the user wants to override our default values */
    MPL_env2int("PMI_VERSION", &pmi_version);
    MPL_env2int("PMI_SUBVERSION", &pmi_subversion);

    if (pg->size == 1) {
        pg->vct[0].node_id = g_num_nodes++;
        goto fn_exit;
    }

    /* Used for debugging only.  This disables communication over shared memory */
#ifdef ENABLED_NO_LOCAL
    no_local = 1;
#else
    no_local = MPIR_PARAM_NOLOCAL;
#endif

    /* Used for debugging on a single machine: Odd procs on a node are
       seen as local to each other, and even procs on a node are seen
       as local to each other. */
#ifdef ENABLED_ODD_EVEN_CLIQUES
    odd_even_cliques = 1;
#else
    odd_even_cliques = MPIR_PARAM_ODD_EVEN_CLIQUES;
#endif

    if (no_local) {
        /* just assign 0 to n-1 as node ids and bail */
        for (i = 0; i < pg->size; ++i) {
            pg->vct[i].node_id = g_num_nodes++;
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
        int num_nodes = 0;

        mpi_errno = PMI2_Info_GetJobAttr("PMI_process_mapping", process_mapping, sizeof(process_mapping), &found);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKINTERNAL(!found, mpi_errno, "PMI_process_mapping attribute not found");
        /* this code currently assumes pg is comm_world */
        mpi_errno = populate_ids_from_mapping(process_mapping, &num_nodes, pg, &did_map);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKINTERNAL(!did_map, mpi_errno, "unable to populate node ids from PMI_process_mapping");
        g_num_nodes = num_nodes;
    }
#else /* USE_PMI2_API */
    if (our_pg_rank == -1) {
        /* FIXME this routine can't handle the dynamic process case at this
           time.  This will require more support from the process manager. */
        MPIU_Assert(0);
    }

    /* Allocate space for pmi key and value */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    MPIU_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    MPIU_CHKLMEM_MALLOC(value, char *, val_max_sz, mpi_errno, "value");

    mpi_errno = MPIDI_PG_GetConnKVSname(&kvs_name);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* See if process manager supports PMI_process_mapping keyval */

    /* FIXME 'PMI_process_mapping' only applies for the original PG (MPI_COMM_WORLD) */
    if (pmi_version == 1 && pmi_subversion == 1) {
        pmi_errno = PMI_KVS_Get(kvs_name, "PMI_process_mapping", value, val_max_sz);
        if (pmi_errno == 0) {
            int did_map = 0;
            int num_nodes = 0;
            /* this code currently assumes pg is comm_world */
            mpi_errno = populate_ids_from_mapping(value, &num_nodes, pg, &did_map);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            g_num_nodes = num_nodes;
            if (did_map) {
                goto fn_exit;
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
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

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

    g_num_nodes = 0; /* defensive */

    for (i = 0; i < pg->size; ++i)
    {
        MPIU_Assert(g_num_nodes < pg->size);
        if (i == our_pg_rank)
        {
            /* This is us, no need to perform a get */
            MPIU_Snprintf(node_names[g_num_nodes], key_max_sz, "%s", MPIU_hostname);
        }
        else
        {
            memset(key, 0, key_max_sz);
            MPIU_Snprintf(key, key_max_sz, "hostname[%d]", i);

            pmi_errno = PMI_KVS_Get(kvs_name, key, node_names[g_num_nodes], key_max_sz);
            MPIU_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
        }

        /* Find the node_id for this process, or create a new one */
        /* FIXME:need a better algorithm -- this one does O(N^2) strncmp()s! */
        /* The right fix is to get all this information from the process
           manager, rather than bother with this hostname hack at all. */
        for (j = 0; j < g_num_nodes; ++j)
            if (!strncmp(node_names[j], node_names[g_num_nodes], key_max_sz))
                break;
        if (j == g_num_nodes)
            ++g_num_nodes;
        else
            node_names[g_num_nodes][0] = '\0';
        pg->vct[i].node_id = j;
    }

    if (odd_even_cliques)
    {
        /* Create new processes for all odd numbered processes. This
           may leave nodes ids with no processes assigned to them, but
           I think this is OK */
        for (i = 0; i < pg->size; ++i)
            if (i & 0x1)
                pg->vct[i].node_id += g_num_nodes;
        g_num_nodes *= 2;
    }
#endif

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

