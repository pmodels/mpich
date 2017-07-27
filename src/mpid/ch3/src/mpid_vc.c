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

#include "build_nodemap.h"
/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
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
    MPIR_CHKPMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_VCRT_CREATE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_VCRT_CREATE);

    MPIR_CHKPMEM_MALLOC(vcrt, MPIDI_VCRT_t *, sizeof(MPIDI_VCRT_t) + (size - 1) * sizeof(MPIDI_VC_t *),	mpi_errno, "**nomem");
    vcrt->handle = HANDLE_SET_KIND(0, HANDLE_KIND_INVALID);
    MPIR_Object_set_ref(vcrt, 1);
    vcrt->size = size;
    *vcrt_ptr = vcrt;

 fn_exit:
    MPIR_CHKPMEM_COMMIT();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_VCRT_CREATE);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_VCRT_ADD_REF);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_VCRT_ADD_REF);
    MPIR_Object_add_ref(vcrt);
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_REFCOUNT,TYPICAL,(MPL_DBG_FDEST, "Incr VCRT %p ref count",vcrt));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_VCRT_ADD_REF);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_VCRT_RELEASE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_VCRT_RELEASE);

    MPIR_Object_release_ref(vcrt, &in_use);
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_REFCOUNT,TYPICAL,(MPL_DBG_FDEST, "Decr VCRT %p ref count",vcrt));
    
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
                MPIR_Assert(in_use);
                /* FIXME this is still bogus, the VCRT may contain a mix of
                 * dynamic and non-dynamic VCs, so the ref_count isn't
                 * guaranteed to have started at 2.  The best thing to do might
                 * be to avoid overloading the reference counting this way and
                 * use a separate check for dynamic VCs (another flag? compare
                 * PGs?) */
                MPIR_Object_release_ref(vc, &in_use);
            }
#endif
	    if (isDisconnect && MPIR_Object_get_ref(vc) == 1) {
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

		    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
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

	MPL_free(vcrt);
    }

 fn_exit:    
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_VCRT_RELEASE);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_VCR_DUP);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_VCR_DUP);

    /* We are allowed to create a vc that belongs to no process group 
     as part of the initial connect/accept action, so in that case,
     ignore the pg ref count update */
    /* XXX DJG FIXME-MT should we be checking this? */
    /* we probably need a test-and-incr operation or equivalent to avoid races */
    if (MPIR_Object_get_ref(orig_vcr) == 0 && orig_vcr->pg) {
	MPIDI_VC_add_ref( orig_vcr );
	MPIDI_VC_add_ref( orig_vcr );
	MPIDI_PG_add_ref( orig_vcr->pg );
    }
    else {
	MPIDI_VC_add_ref(orig_vcr);
    }
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_REFCOUNT,TYPICAL,(MPL_DBG_FDEST,"Incr VCR %p ref count",orig_vcr));
    *new_vcr = orig_vcr;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_VCR_DUP);
    return MPI_SUCCESS;
}

/*@
  MPID_Comm_get_lpid - Get the local process ID for a given VC reference
  @*/
#undef FUNCNAME
#define FUNCNAME MPID_Comm_get_lpid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Comm_get_lpid(MPIR_Comm *comm_ptr, int idx, int * lpid_ptr, MPL_bool is_remote)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_VCR_GET_LPID);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_VCR_GET_LPID);

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
        *lpid_ptr = comm_ptr->dev.vcrt->vcr_table[idx]->lpid;
    else if (is_remote)
        *lpid_ptr = comm_ptr->dev.vcrt->vcr_table[idx]->lpid;
    else
        *lpid_ptr = comm_ptr->dev.local_vcrt->vcr_table[idx]->lpid;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_VCR_GET_LPID);
    return MPI_SUCCESS;
}

/* 
 * The following routines convert to/from the global pids, which are 
 * represented as pairs of ints (process group id, rank in that process group)
 */

/* FIXME: These routines belong in a different place */
#undef FUNCNAME
#define FUNCNAME MPIDI_GPID_GetAllInComm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_GPID_GetAllInComm( MPIR_Comm *comm_ptr, int local_size,
                             MPIDI_Gpid local_gpids[], int *singlePG )
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int *gpid = (int*)&local_gpids[0];
    int lastPGID = -1, pgid;
    MPIDI_VCR vc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GPID_GETALLINCOMM);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GPID_GETALLINCOMM);

    MPIR_Assert(comm_ptr->local_size == local_size);
    
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

        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE, (MPL_DBG_FDEST,
                         "pgid=%d vc->pg_rank=%d",
                         pgid, vc->pg_rank));
    }
    
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GPID_GETALLINCOMM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_GPID_Get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_GPID_Get( MPIR_Comm *comm_ptr, int rank, MPIDI_Gpid *in_gpid )
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
#define FUNCNAME MPIDI_GPID_ToLpidArray
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_GPID_ToLpidArray( int size, MPIDI_Gpid in_gpid[], int lpid[] )
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

#undef FUNCNAME
#define FUNCNAME MPIDI_LPID_GetAllInComm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_LPID_GetAllInComm(MPIR_Comm *comm_ptr, int local_size,
                                          int local_lpids[])
{
    int i;
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert( comm_ptr->local_size == local_size );
    for (i=0; i<comm_ptr->local_size; i++) {
	mpi_errno |= MPID_Comm_get_lpid( comm_ptr, i, &local_lpids[i], FALSE );
    }
    return mpi_errno;
}

#ifdef HAVE_ERROR_CHECKING
#define N_STATIC_LPID32 128
/*@
  MPIDI_CheckDisjointLpids - Exchange address mapping for intercomm creation.
 @*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CheckDisjointLpids
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CheckDisjointLpids(int lpids1[], int n1, int lpids2[], int n2)
{
    int i, mask_size, idx, bit, maxlpid = -1;
    int mpi_errno = MPI_SUCCESS;
    uint32_t lpidmaskPrealloc[N_STATIC_LPID32];
    uint32_t *lpidmask;
    MPIR_CHKLMEM_DECL(1);

    /* Find the max lpid */
    for (i=0; i<n1; i++) {
        if (lpids1[i] > maxlpid) maxlpid = lpids1[i];
    }
    for (i=0; i<n2; i++) {
        if (lpids2[i] > maxlpid) maxlpid = lpids2[i];
    }

    mask_size = (maxlpid / 32) + 1;

    if (mask_size > N_STATIC_LPID32) {
        MPIR_CHKLMEM_MALLOC(lpidmask,uint32_t*,mask_size*sizeof(uint32_t),
                            mpi_errno,"lpidmask");
    }
    else {
        lpidmask = lpidmaskPrealloc;
    }

    /* zero the bitvector array */
    memset(lpidmask, 0x00, mask_size*sizeof(*lpidmask));

    /* Set the bits for the first array */
    for (i=0; i<n1; i++) {
        idx = lpids1[i] / 32;
        bit = lpids1[i] % 32;
        lpidmask[idx] = lpidmask[idx] | (1 << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Look for any duplicates in the second array */
    for (i=0; i<n2; i++) {
        idx = lpids2[i] / 32;
        bit = lpids2[i] % 32;
        if (lpidmask[idx] & (1 << bit)) {
            MPIR_ERR_SET1(mpi_errno,MPI_ERR_COMM,
                          "**dupprocesses", "**dupprocesses %d", lpids2[i] );
            goto fn_fail;
        }
        /* Add a check on duplicates *within* group 2 */
        lpidmask[idx] = lpidmask[idx] | (1 << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Also fall through for normal return */
 fn_fail:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
}
#endif /* HAVE_ERROR_CHECKING */

/*@
  MPID_Intercomm_exchange_map - Exchange address mapping for intercomm creation.
 @*/
#undef FUNCNAME
#define FUNCNAME MPID_Intercomm_exchange_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Intercomm_exchange_map(MPIR_Comm *local_comm_ptr, int local_leader,
                                MPIR_Comm *peer_comm_ptr, int remote_leader,
                                int *remote_size, int **remote_lpids,
                                int *is_low_group)
{
    int mpi_errno = MPI_SUCCESS;
    int singlePG;
    int local_size,*local_lpids=0;
    MPIDI_Gpid *local_gpids=NULL, *remote_gpids=NULL;
    int comm_info[2];
    int cts_tag;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_CHKLMEM_DECL(3);

    cts_tag = 0 | MPIR_Process.tagged_coll_mask;

    if (local_comm_ptr->rank == local_leader) {

        /* First, exchange the group information.  If we were certain
           that the groups were disjoint, we could exchange possible
           context ids at the same time, saving one communication.
           But experience has shown that that is a risky assumption.
        */
        /* Exchange information with my peer.  Use sendrecv */
        local_size = local_comm_ptr->local_size;

        /* printf( "About to sendrecv in intercomm_create\n" );fflush(stdout);*/
        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,"rank %d sendrecv to rank %d", peer_comm_ptr->rank,
                                       remote_leader));
        mpi_errno = MPIC_Sendrecv( &local_size,  1, MPI_INT,
                                      remote_leader, cts_tag,
                                      remote_size, 1, MPI_INT,
                                      remote_leader, cts_tag,
                                      peer_comm_ptr, MPI_STATUS_IGNORE, &errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST, "local size = %d, remote size = %d", local_size,
                                       *remote_size ));
        /* With this information, we can now send and receive the
           global process ids from the peer. */
        MPIR_CHKLMEM_MALLOC(remote_gpids,MPIDI_Gpid*,(*remote_size)*sizeof(MPIDI_Gpid), mpi_errno,"remote_gpids");
        *remote_lpids = (int*) MPL_malloc((*remote_size)*sizeof(int));
        MPIR_CHKLMEM_MALLOC(local_gpids,MPIDI_Gpid*,local_size*sizeof(MPIDI_Gpid), mpi_errno,"local_gpids");
        MPIR_CHKLMEM_MALLOC(local_lpids,int*,local_size*sizeof(int), mpi_errno,"local_lpids");

        mpi_errno = MPIDI_GPID_GetAllInComm( local_comm_ptr, local_size, local_gpids, &singlePG );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* Exchange the lpid arrays */
        mpi_errno = MPIC_Sendrecv( local_gpids, local_size*sizeof(MPIDI_Gpid), MPI_BYTE,
                                      remote_leader, cts_tag,
                                      remote_gpids, (*remote_size)*sizeof(MPIDI_Gpid), MPI_BYTE,
                                      remote_leader, cts_tag, peer_comm_ptr,
                                      MPI_STATUS_IGNORE, &errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);


        /* Convert the remote gpids to the lpids */
        mpi_errno = MPIDI_GPID_ToLpidArray( *remote_size, remote_gpids, *remote_lpids );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* Get our own lpids */
        mpi_errno = MPIDI_LPID_GetAllInComm( local_comm_ptr, local_size, local_lpids );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

#       ifdef HAVE_ERROR_CHECKING
        {
            MPID_BEGIN_ERROR_CHECKS;
            {
                /* Now that we have both the local and remote processes,
                   check for any overlap */
                mpi_errno = MPIDI_CheckDisjointLpids( local_lpids, local_size, *remote_lpids, *remote_size );
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            MPID_END_ERROR_CHECKS;
        }
#       endif /* HAVE_ERROR_CHECKING */

        /* Make an arbitrary decision about which group of processs is
           the low group.  The LEADERS do this by comparing the
           local process ids of the 0th member of the two groups */
        (*is_low_group) = local_lpids[0] < (*remote_lpids)[0];

        /* At this point, we're done with the local lpids; they'll
           be freed with the other local memory on exit */

    } /* End of the first phase of the leader communication */
    /* Leaders can now swap context ids and then broadcast the value
       to the local group of processes */
    if (local_comm_ptr->rank == local_leader) {
        /* Now, send all of our local processes the remote_lpids,
           along with the final context id */
        comm_info[0] = *remote_size;
        comm_info[1] = *is_low_group;
        MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"About to bcast on local_comm");
        mpi_errno = MPID_Bcast( comm_info, 2, MPI_INT, local_leader, local_comm_ptr, &errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        mpi_errno = MPID_Bcast( remote_gpids, (*remote_size)*sizeof(MPIDI_Gpid), MPI_BYTE, local_leader,
                                     local_comm_ptr, &errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,VERBOSE,"end of bcast on local_comm of size %d",
                       local_comm_ptr->local_size );
    }
    else
    {
        /* we're the other processes */
        MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"About to receive bcast on local_comm");
        mpi_errno = MPID_Bcast( comm_info, 2, MPI_INT, local_leader, local_comm_ptr, &errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        *remote_size = comm_info[0];
        MPIR_CHKLMEM_MALLOC(remote_gpids,MPIDI_Gpid*,(*remote_size)*sizeof(MPIDI_Gpid), mpi_errno,"remote_gpids");
        *remote_lpids = (int*) MPL_malloc((*remote_size)*sizeof(int));
        mpi_errno = MPID_Bcast( remote_gpids, (*remote_size)*sizeof(MPIDI_Gpid), MPI_BYTE, local_leader,
                                     local_comm_ptr, &errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* Extract the context and group sign informatin */
        *is_low_group     = comm_info[1];
    }

    /* Finish up by giving the device the opportunity to update
       any other infomration among these processes.  Note that the
       new intercomm has not been set up; in fact, we haven't yet
       attempted to set up the connection tables.

       In the case of the ch3 device, this calls MPID_PG_ForwardPGInfo
       to ensure that all processes have the information about all
       process groups.  This must be done before the call
       to MPID_GPID_ToLpidArray, as that call needs to know about
       all of the process groups.
    */
    MPIDI_PG_ForwardPGInfo( peer_comm_ptr, local_comm_ptr,
                            *remote_size, (const MPIDI_Gpid*)remote_gpids, local_leader );


    /* Finally, if we are not the local leader, we need to
       convert the remote gpids to local pids.  This must be done
       after we allow the device to handle any steps that it needs to
       take to ensure that all processes contain the necessary process
       group information */
    if (local_comm_ptr->rank != local_leader) {
        mpi_errno = MPIDI_GPID_ToLpidArray( *remote_size, remote_gpids, *remote_lpids );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
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
int MPID_Create_intercomm_from_lpids( MPIR_Comm *newcomm_ptr,
			    int size, const int lpids[] )
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *commworld_ptr;
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
#define FUNCNAME MPIDI_PG_ForwardPGInfo
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_PG_ForwardPGInfo( MPIR_Comm *peer_ptr, MPIR_Comm *comm_ptr,
			   int nPGids, const MPIDI_Gpid in_gpids[],
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
    vc->handle  = HANDLE_SET_MPI_KIND(0, MPIR_VCONN);
    MPIR_Object_set_ref(vc, 0);
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
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
    {
        int err;
        MPID_Thread_mutex_create(&vc->pobj_mutex,&err);
        MPIR_Assert(err == 0);
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
int MPID_Get_node_id(MPIR_Comm *comm, int rank, MPID_Node_id_t *id_p)
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
int MPID_Get_max_node_id(MPIR_Comm *comm, MPID_Node_id_t *max_id_p)
{
    /* easiest way to implement this is to track it at PG create/destroy time */
    *max_id_p = g_max_node_id;
    MPIR_Assert(*max_id_p >= 0);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Populate_vc_node_ids
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_Populate_vc_node_ids(MPIDI_PG_t *pg, int our_pg_rank)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPID_Node_id_t *out_nodemap;
    out_nodemap = (MPID_Node_id_t *) MPL_malloc(pg->size * sizeof(MPID_Node_id_t));

    mpi_errno = MPIR_NODEMAP_build_nodemap(pg->size, our_pg_rank, out_nodemap, &g_max_node_id);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_Assert(g_max_node_id >= 0);

    for (i = 0; i < pg->size; i++) {
        pg->vct[i].node_id = out_nodemap[i];
    }

fn_exit:
    MPL_free(out_nodemap);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

