/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Intercomm_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Intercomm_create = PMPI_Intercomm_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Intercomm_create  MPI_Intercomm_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Intercomm_create as PMPI_Intercomm_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifdef HAVE_ERROR_CHECKING
PMPI_LOCAL int MPIR_CheckDisjointLpids( int [], int, int [], int );
#endif /* HAVE_ERROR_CHECKING */
PMPI_LOCAL int MPID_LPID_GetAllInComm( MPID_Comm *comm_ptr, int local_size, 
				       int local_lpids[] );

#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Intercomm_create
#define MPI_Intercomm_create PMPI_Intercomm_create

/* 128 allows us to handle up to 4k processes */
#ifdef HAVE_ERROR_CHECKING
#define MAX_LPID32_ARRAY 128
#undef FUNCNAME
#define FUNCNAME MPIR_CheckDisjointLpids
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
PMPI_LOCAL int MPIR_CheckDisjointLpids( int lpids1[], int n1, 
					 int lpids2[], int n2 )
{
    int i, maxi, idx, bit, maxlpid = -1;
    int mpi_errno = MPI_SUCCESS;
    int32_t lpidmaskPrealloc[MAX_LPID32_ARRAY];
    int32_t *lpidmask;
    MPIU_CHKLMEM_DECL(1);

    /* Find the max lpid */
    for (i=0; i<n1; i++) {
	if (lpids1[i] > maxlpid) maxlpid = lpids1[i];
    }
    for (i=0; i<n2; i++) {
	if (lpids2[i] > maxlpid) maxlpid = lpids2[i];
    }

    /* Compute the max index and zero the pids array */
    maxi = (maxlpid + 31) / 32;

    if (maxi >= MAX_LPID32_ARRAY) {
	MPIU_CHKLMEM_MALLOC(lpidmask,int32_t*,maxi*sizeof(int32_t),
			    mpi_errno,"lpidmask");
    }
    else {
	lpidmask = lpidmaskPrealloc;
    }
    
    for (i=0; i<maxi; i++) lpidmask[i] = 0;

    /* Set the bits for the first array */
    for (i=0; i<n1; i++) {
	idx = lpids1[i] / 32;
	bit = lpids1[i] % 32;
	lpidmask[idx] = lpidmask[idx] | (1 << bit);
    }    

    /* Look for any duplicates in the second array */
    for (i=0; i<n2; i++) {
	idx = lpids2[i] / 32;
	bit = lpids2[i] % 32;
	if (lpidmask[idx] & (1 << bit)) {
	    MPIU_ERR_SET1(mpi_errno,MPI_ERR_COMM, 
			  "**dupprocesses", "**dupprocesses %d", lpids2[i] );
	    goto fn_fail;
	}
	/* Add a check on duplicates *within* group 2 */
	lpidmask[idx] = lpidmask[idx] | (1 << bit);
    }

    /* Also fall through for normal return */
 fn_fail:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
    
}
#endif /* HAVE_ERROR_CHECKING */

#ifndef HAVE_GPID_ROUTINES
/* FIXME: A temporary version for lpids within my comm world */
PMPI_LOCAL int MPID_GPID_GetAllInComm( MPID_Comm *comm_ptr, int local_size, 
				       int local_gpids[], int *singlePG )
{
    int i;
    int *gpid = local_gpids;
    
    for (i=0; i<comm_ptr->local_size; i++) {
	*gpid++ = 0;
	(void)MPID_VCR_Get_lpid( comm_ptr->vcr[i], gpid );
	gpid++;
    }
    *singlePG = 1;
    return 0;
}

/* FIXME: A temp for lpids within my comm world */
PMPI_LOCAL int MPID_GPID_ToLpidArray( int size, int gpid[], int lpid[] )
{
    int i;

    for (i=0; i<size; i++) {
	lpid[i] = *++gpid;  gpid++;
    }
    return 0;
}
/* FIXME: for MPI1, all process ids are relative to MPI_COMM_WORLD.
   For MPI2, we'll need to do something more complex */
PMPI_LOCAL int MPID_VCR_CommFromLpids( MPID_Comm *newcomm_ptr, 
				       int size, const int lpids[] )
{
    MPID_Comm *commworld_ptr;
    int i;

    commworld_ptr = MPIR_Process.comm_world;
    /* Setup the communicator's vc table: remote group */
    MPID_VCRT_Create( size, &newcomm_ptr->vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
    for (i=0; i<size; i++) {
	/* For rank i in the new communicator, find the corresponding
	   rank in the comm world (FIXME FOR MPI2) */
	/* printf( "[%d] Remote rank %d has lpid %d\n", 
	   MPIR_Process.comm_world->rank, i, lpids[i] ); */
	if (lpids[i] < commworld_ptr->remote_size) {
	    MPID_VCR_Dup( commworld_ptr->vcr[lpids[i]], 
			  &newcomm_ptr->vcr[i] );
	}
	else {
	    /* We must find the corresponding vcr for a given lpid */
	    /* FIXME: Error */
	    return 1;
	    /* MPID_VCR_Dup( ???, &newcomm_ptr->vcr[i] ); */
	}
    }
    return 0;
}

#endif /* HAVE_GPID_ROUTINES */

PMPI_LOCAL int MPID_LPID_GetAllInComm( MPID_Comm *comm_ptr, int local_size, 
				       int local_lpids[] )
{
    int i;
    
    /* FIXME: Should be using the local_size argument */
    MPIU_Assert( comm_ptr->local_size == local_size );
    for (i=0; i<comm_ptr->local_size; i++) {
	(void)MPID_VCR_Get_lpid( comm_ptr->vcr[i], &local_lpids[i] );
    }
    return 0;
}


#endif /* MPICH_MPI_FROM_PMPI */


#undef FUNCNAME
#define FUNCNAME MPI_Intercomm_create
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@

MPI_Intercomm_create - Creates an intercommuncator from two intracommunicators

Input Parameters:
+ local_comm - Local (intra)communicator
. local_leader - Rank in local_comm of leader (often 0)
. peer_comm - Communicator used to communicate between a 
              designated process in the other communicator.  
              Significant only at the process in 'local_comm' with
	      rank 'local_leader'.
. remote_leader - Rank in peer_comm of remote leader (often 0)
- tag - Message tag to use in constructing intercommunicator; if multiple
  'MPI_Intercomm_creates' are being made, they should use different tags (more
  precisely, ensure that the local and remote leaders are using different
  tags for each 'MPI_intercomm_create').

Output Parameter:
. comm_out - Created intercommunicator

Notes:
   'peer_comm' is significant only for the process designated the 
   'local_leader' in the 'local_comm'.

  The MPI 1.1 Standard contains two mutually exclusive comments on the
  input intercommunicators.  One says that their repective groups must be
  disjoint; the other that the leaders can be the same process.  After
  some discussion by the MPI Forum, it has been decided that the groups must
  be disjoint.  Note that the `reason` given for this in the standard is
  `not` the reason for this choice; rather, the `other` operations on 
  intercommunicators (like 'MPI_Intercomm_merge') do not make sense if the
  groups are not disjoint.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_TAG
.N MPI_ERR_EXHAUSTED
.N MPI_ERR_RANK

.seealso: MPI_Intercomm_merge, MPI_Comm_free, MPI_Comm_remote_group, 
          MPI_Comm_remote_size

@*/
int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, 
			 MPI_Comm peer_comm, int remote_leader, int tag, 
			 MPI_Comm *newintercomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *peer_comm_ptr = NULL;
    MPIR_Context_id_t final_context_id, recvcontext_id;
    int remote_size, *remote_lpids=0, *remote_gpids=0, singlePG;
    int local_size, *local_gpids=0, *local_lpids=0;
    int comm_info[3];
    int is_low_group = 0;
    int i;
    MPID_Comm *newcomm_ptr;
    int errflag = FALSE;
    MPIU_CHKLMEM_DECL(4);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INTERCOMM_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INTERCOMM_CREATE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(local_comm, mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( local_comm, comm_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    if (comm_ptr) {
		/*  Only check if comm_ptr valid */
		MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno );
		if ((local_leader < 0 || 
		     local_leader >= comm_ptr->local_size)) {
		    MPIU_ERR_SET2(mpi_errno,MPI_ERR_RANK, 
				  "**ranklocal", "**ranklocal %d %d", 
				  local_leader, comm_ptr->local_size );
		}
		if (comm_ptr->rank == local_leader) {
		    MPIR_ERRTEST_COMM(peer_comm, mpi_errno);
		}
	    }
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    
    /*
     * Error checking for this routine requires care.  Because this
     * routine is collective over two different sets of processes,
     * it is relatively easy for the user to try to create an 
     * intercommunicator from two overlapping groups of processes.
     * This is made more likely by inconsistencies in the MPI-1
     * specification (clarified in MPI-2) that seemed to allow
     * the groups to overlap.  Because of that, we first check that the
     * groups are in fact disjoint before performing any collective 
     * operations.  
     */

    /*printf( "comm_ptr->rank = %d, local_leader = %d\n", comm_ptr->rank,
      local_leader ); fflush(stdout);*/
    if (comm_ptr->rank == local_leader) {

	MPID_Comm_get_ptr( peer_comm, peer_comm_ptr );
#       ifdef HAVE_ERROR_CHECKING
	{
	    MPID_BEGIN_ERROR_CHECKS;
	    {
		MPID_Comm_valid_ptr( peer_comm_ptr, mpi_errno );
		/* Note: In MPI 1.0, peer_comm was restricted to 
		   intracommunicators.  In 1.1, it may be any communicator */

		/* In checking the rank of the remote leader, 
		   allow the peer_comm to be in intercommunicator
		   by checking against the remote size */
		if (!mpi_errno && peer_comm_ptr && 
		    (remote_leader < 0 || 
		     remote_leader >= peer_comm_ptr->remote_size)) {
		    MPIU_ERR_SET2(mpi_errno,MPI_ERR_RANK, 
				  "**rankremote", "**rankremote %d %d", 
				  remote_leader, peer_comm_ptr->remote_size );
		}
		/* Check that the local leader and the remote leader are 
		   different processes.  This test requires looking at
		   the lpid for the two ranks in their respective 
		   communicators.  However, an easy test is for 
		   the same ranks in an intracommunicator; we only
		   need the lpid comparison for intercommunicators */
		/* Here is the test.  We restrict this test to the
		   process that is the local leader (comm_ptr->rank == 
		   local_leader because we can then use peer_comm_ptr->rank
		   to get the rank in peer_comm of the local leader. */
		if (peer_comm_ptr->comm_kind == MPID_INTRACOMM &&
		    comm_ptr->rank == local_leader && 
		    peer_comm_ptr->rank == remote_leader) {
		    MPIU_ERR_SET(mpi_errno,MPI_ERR_RANK,"**ranksdistinct");
		}
		if (mpi_errno) goto fn_fail;
	    }
	    MPID_END_ERROR_CHECKS;
	}
#       endif /* HAVE_ERROR_CHECKING */
	
	/* First, exchange the group information.  If we were certain
	   that the groups were disjoint, we could exchange possible 
	   context ids at the same time, saving one communication.
	   But experience has shown that that is a risky assumption.
	*/
	/* Exchange information with my peer.  Use sendrecv */
	local_size = comm_ptr->local_size;

	/* printf( "About to sendrecv in intercomm_create\n" );fflush(stdout);*/
	MPIU_DBG_MSG_FMT(COMM,VERBOSE,
             (MPIU_DBG_FDEST,"rank %d sendrecv to rank %d", 
              peer_comm_ptr->rank, remote_leader));
	mpi_errno = MPIC_Sendrecv( &local_size,  1, MPI_INT, 
				   remote_leader, tag,
				   &remote_size, 1, MPI_INT, 
				   remote_leader, tag, 
				   peer_comm, MPI_STATUS_IGNORE );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

	MPIU_DBG_MSG_FMT(COMM,VERBOSE,
           (MPIU_DBG_FDEST, "local size = %d, remote size = %d", local_size, 
		      remote_size ));
	/* With this information, we can now send and receive the 
	   global process ids from the peer. */
	MPIU_CHKLMEM_MALLOC(remote_gpids,int*,2*remote_size*sizeof(int),
			    mpi_errno,"remote_gpids");
	MPIU_CHKLMEM_MALLOC(remote_lpids,int*,remote_size*sizeof(int),
			    mpi_errno,"remote_lpids");
	MPIU_CHKLMEM_MALLOC(local_gpids,int*,2*local_size*sizeof(int),
			    mpi_errno,"local_gpids");
	MPIU_CHKLMEM_MALLOC(local_lpids,int*,local_size*sizeof(int),
			    mpi_errno,"local_lpids");

	mpi_errno = MPID_GPID_GetAllInComm( comm_ptr, local_size, local_gpids, 
					    &singlePG );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

	/* Exchange the lpid arrays */
	mpi_errno = MPIC_Sendrecv( local_gpids, 2*local_size, MPI_INT, 
                                   remote_leader, tag,
                                   remote_gpids, 2*remote_size, MPI_INT, 
                                   remote_leader, tag, peer_comm, MPI_STATUS_IGNORE );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        
	/* Convert the remote gpids to the lpids */
	mpi_errno = MPID_GPID_ToLpidArray( remote_size, 
					   remote_gpids, remote_lpids );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

	/* Get our own lpids */
	mpi_errno = MPID_LPID_GetAllInComm( comm_ptr, local_size, local_lpids );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	
#       ifdef HAVE_ERROR_CHECKING
	{
	    MPID_BEGIN_ERROR_CHECKS;
	    {
		/* Now that we have both the local and remote processes,
		   check for any overlap */
		mpi_errno = MPIR_CheckDisjointLpids( local_lpids, local_size,
						   remote_lpids, remote_size );
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	    }
	    MPID_END_ERROR_CHECKS;
	}
#       endif /* HAVE_ERROR_CHECKING */
	
	/* Make an arbitrary decision about which group of processs is
	   the low group.  The LEADERS do this by comparing the
	   local process ids of the 0th member of the two groups */
	is_low_group = local_lpids[0] < remote_lpids[0];

	/* At this point, we're done with the local lpids; they'll
	   be freed with the other local memory on exit */

    } /* End of the first phase of the leader communication */

    /* 
     * Create the contexts.  Each group will have a context for sending 
     * to the other group. All processes must be involved.  Because 
     * we know that the local and remote groups are disjoint, this 
     * step will complete 
     */
    MPIU_DBG_MSG_FMT(COMM,VERBOSE,
          (MPIU_DBG_FDEST,"About to get contextid (local_size=%d) on rank %d",
		  comm_ptr->local_size, comm_ptr->rank ));
    /* In the multi-threaded case, MPIR_Get_contextid assumes that the
       calling routine already holds the single criticial section */
    mpi_errno = MPIR_Get_contextid( comm_ptr, &recvcontext_id );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(recvcontext_id != 0);
    MPIU_DBG_MSG_FMT(COMM,VERBOSE, (MPIU_DBG_FDEST,"Got contextid=%d", recvcontext_id));

    /* Leaders can now swap context ids and then broadcast the value
       to the local group of processes */
    if (comm_ptr->rank == local_leader) {
	MPIR_Context_id_t remote_context_id;

	mpi_errno = MPIC_Sendrecv( &recvcontext_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, remote_leader, tag,
                                   &remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, remote_leader, tag, 
                                   peer_comm, MPI_STATUS_IGNORE );
	if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        
	final_context_id = remote_context_id;

	/* Now, send all of our local processes the remote_lpids, 
	   along with the final context id */
	comm_info[0] = remote_size;
	comm_info[1] = final_context_id;
	comm_info[2] = is_low_group;
	MPIU_DBG_MSG(COMM,VERBOSE,"About to bcast on local_comm");
	mpi_errno = MPIR_Bcast_impl( comm_info, 3, MPI_INT, local_leader, comm_ptr, &errflag );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
	mpi_errno = MPIR_Bcast_impl( remote_gpids, 2*remote_size, MPI_INT, local_leader,
                                     comm_ptr, &errflag );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
	MPIU_DBG_MSG_D(COMM,VERBOSE,"end of bcast on local_comm of size %d",
		       comm_ptr->local_size );
    }
    else
    {
	/* we're the other processes */
	MPIU_DBG_MSG(COMM,VERBOSE,"About to receive bcast on local_comm");
	mpi_errno = MPIR_Bcast_impl( comm_info, 3, MPI_INT, local_leader, comm_ptr, &errflag );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
	remote_size = comm_info[0];
	MPIU_CHKLMEM_MALLOC(remote_gpids,int*,2*remote_size*sizeof(int),
			    mpi_errno,"remote_gpids");
	MPIU_CHKLMEM_MALLOC(remote_lpids,int*,remote_size*sizeof(int),
			    mpi_errno,"remote_lpids");
	mpi_errno = MPIR_Bcast_impl( remote_gpids, 2*remote_size, MPI_INT, local_leader, 
                                     comm_ptr, &errflag );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

	/* Extract the context and group sign informatin */
	final_context_id = comm_info[1];
	is_low_group     = comm_info[2];
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
#ifdef MPID_ICCREATE_REMOTECOMM_HOOK
    MPID_ICCREATE_REMOTECOMM_HOOK( peer_comm_ptr, comm_ptr,
				   remote_size, remote_gpids, local_leader );
				   
#endif

    /* Finally, if we are not the local leader, we need to 
       convert the remote gpids to local pids.  This must be done
       after we allow the device to handle any steps that it needs to 
       take to ensure that all processes contain the necessary process
       group information */
    if (comm_ptr->rank != local_leader) {
	mpi_errno = MPID_GPID_ToLpidArray( remote_size, remote_gpids, 
					   remote_lpids );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }


    /* At last, we now have the information that we need to build the 
       intercommunicator */

    /* All processes in the local_comm now build the communicator */

    mpi_errno = MPIR_Comm_create( &newcomm_ptr );
    if (mpi_errno) goto fn_fail;

    newcomm_ptr->context_id	= final_context_id;
    newcomm_ptr->recvcontext_id	= recvcontext_id;
    newcomm_ptr->remote_size	= remote_size;
    newcomm_ptr->local_size	= comm_ptr->local_size;
    newcomm_ptr->rank		= comm_ptr->rank;
    newcomm_ptr->comm_kind	= MPID_INTERCOMM;
    newcomm_ptr->local_comm	= 0;
    newcomm_ptr->is_low_group	= is_low_group;

    mpi_errno = MPID_VCR_CommFromLpids( newcomm_ptr, remote_size, remote_lpids );
    if (mpi_errno) goto fn_fail;

    /* Setup the communicator's vc table: local group.  This is
     just a duplicate of the local_comm's group */
    MPID_VCRT_Create( comm_ptr->local_size, &newcomm_ptr->local_vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->local_vcrt, &newcomm_ptr->local_vcr );
    for (i=0; i<comm_ptr->local_size; i++) {
	MPID_VCR_Dup( comm_ptr->vcr[i], &newcomm_ptr->local_vcr[i] );
    }

    /* Inherit the error handler (if any) */
    MPIU_THREAD_CS_ENTER(MPI_OBJ, comm_ptr);
    newcomm_ptr->errhandler = comm_ptr->errhandler;
    if (comm_ptr->errhandler) {
	MPIR_Errhandler_add_ref( comm_ptr->errhandler );
    }
    MPIU_THREAD_CS_EXIT(MPI_OBJ, comm_ptr);

    /* Notify the device of this new communicator */
    MPID_Dev_comm_create_hook( newcomm_ptr );
    mpi_errno = MPIR_Comm_commit(newcomm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_OBJ_PUBLISH_HANDLE(*newintercomm, newcomm_ptr->handle);
    /* ... end of body of routine ... */
    
  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_intercomm_create",
	    "**mpi_intercomm_create %C %d %C %d %d %p", local_comm, 
	    local_leader, peer_comm, remote_leader, tag, newintercomm);
    }
#   endif /* HAVE_ERROR_CHECKING */
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
