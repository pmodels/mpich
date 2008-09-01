/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_create = PMPI_Comm_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_create  MPI_Comm_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_create as PMPI_Comm_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_create
#define MPI_Comm_create PMPI_Comm_create

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_create

/*@

MPI_Comm_create - Creates a new communicator

Input Parameters:
+ comm - communicator (handle) 
- group - group, which is a subset of the group of 'comm'  (handle) 

Output Parameter:
. comm_out - new communicator (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_GROUP

.seealso: MPI_Comm_free
@*/
int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
    static const char FCNAME[] = "MPI_Comm_create";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int i, j, n, *mapping = 0, *remote_mapping = 0, remote_size = -1;
    MPIR_Context_id_t new_context_id;
    MPID_Comm *newcomm_ptr;
    MPID_Group *group_ptr;
    MPIU_CHKLMEM_DECL(3);
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("comm");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE);
    MPIU_THREADPRIV_GET;

    /* Validate parameters, and convert MPI object handles to object pointers */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
	
	MPID_Comm_get_ptr( comm, comm_ptr );
	
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */

	    MPIR_ERRTEST_GROUP(group, mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
	
	MPID_Group_get_ptr( group, group_ptr );
    
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Check the group ptr */
	    MPID_Group_valid_ptr( group_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   else
    {
	MPID_Comm_get_ptr( comm, comm_ptr );
	MPID_Group_get_ptr( group, group_ptr );
    }
#   endif

    /* ... body of routine ...  */
    
    /* Create a new communicator from the specified group members */

    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
       use the appropriate algorithm to get a new context id. 
       Creating the context id is collective over the *input* communicator,
       so it must be created before we decide if this process is a 
       member of the group */
    /* In the multi-threaded case, MPIR_Get_contextid assumes that the
       calling routine already holds the single criticial section */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	if (!comm_ptr->local_comm) {
	    MPIR_Setup_intercomm_localcomm( comm_ptr );
	}
        mpi_errno = MPIR_Get_contextid( comm_ptr->local_comm, &new_context_id );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        mpi_errno = MPIR_Get_contextid( comm_ptr, &new_context_id );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    MPIU_Assert(new_context_id != 0);

    /* Make sure that the processes for this group are contained within
       the input communicator.  Also identify the mapping from the ranks of 
       the old communicator to the new communicator.
       We do this by matching the lpids of the members of the group
       with the lpids of the members of the input communicator.
       It is an error if the group contains a reference to an lpid that 
       does not exist in the communicator.
       
       An important special case is groups (and communicators) that
       are subsets of MPI_COMM_WORLD.  This this case, the lpids are
       exactly the same as the ranks in comm world.  Currently, we
       don't take this into account, but if the code to handle the general 
       case is too messy, we'll add this in.
    */
    if (group_ptr->rank != MPI_UNDEFINED) {
	MPID_VCR *vcr;
	int      vcr_size;

	n = group_ptr->size;
	MPIU_CHKLMEM_MALLOC(mapping,int*,n*sizeof(int),mpi_errno,"mapping");
	if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	    vcr      = comm_ptr->local_vcr;
	    vcr_size = comm_ptr->local_size;
	}
	else {
	    vcr      = comm_ptr->vcr;
	    vcr_size = comm_ptr->remote_size;
	}
	
	for (i=0; i<n; i++) {
	    /* Mapping[i] is the rank in the communicator of the process that
	       is the ith element of the group */
	    /* We use the appropriate vcr, depending on whether this is
	       an intercomm (use the local vcr) or an intracomm (remote vcr) 
	       Note that this is really the local mapping for intercomm
	       and remote mapping for the intracomm */
	    /* FIXME : BUBBLE SORT */
	    /* FIXME : NEEDS COMM_WORLD SPECIALIZATION */
	    mapping[i] = -1;
	    for (j=0; j<vcr_size; j++) {
		int comm_lpid;
		MPID_VCR_Get_lpid( vcr[j], &comm_lpid );
		if (comm_lpid == group_ptr->lrank_to_lpid[i].lpid) {
		    mapping[i] = j;
		    break;
		}
	    }
	    MPIU_ERR_CHKANDJUMP1(mapping[i] == -1,mpi_errno,
				 MPI_ERR_GROUP,
				 "**groupnotincomm", "**groupnotincomm %d", i );
	}
	if (comm_ptr->comm_kind == MPID_INTRACOMM) {
	    /* If this is an intra comm, we've really determined the
	       remote mapping */
	    remote_mapping = mapping;
	    remote_size    = n;
	    mapping        = 0;
	}

	/* Get the new communicator structure and context id */
	
	mpi_errno = MPIR_Comm_create( &newcomm_ptr );
	if (mpi_errno) goto fn_fail;

	newcomm_ptr->context_id	    = new_context_id;
	newcomm_ptr->rank	    = group_ptr->rank;
	newcomm_ptr->comm_kind	    = comm_ptr->comm_kind;
	/* Since the group has been provided, let the new communicator know
	   about the group */
        newcomm_ptr->local_comm	    = 0;
	newcomm_ptr->local_group    = group_ptr;
	MPIR_Group_add_ref( group_ptr );

	if (comm_ptr->comm_kind == MPID_INTRACOMM) {
	    newcomm_ptr->remote_group   = group_ptr;
	    MPIR_Group_add_ref( group_ptr );
	    newcomm_ptr->recvcontext_id = new_context_id;
	    newcomm_ptr->remote_size    = newcomm_ptr->local_size = n;
	}
	else { /* comm_ptr->comm_kind == MPID_INTERCOMM */ 
	    int rinfo[2];
	    newcomm_ptr->local_size   = group_ptr->size;
	    newcomm_ptr->remote_group = 0;
	    /* There is an additional step.  We must communicate the information
	       on the local context id and the group members, given by the ranks
	       so that the remote process can construct the appropriate VCRT 
	       First we exchange group sizes and context ids.  Then the 
	       ranks in the remote group, from which the remote VCRT can 
	       be constructed.  We can't use NMPI_Sendrecv since we need to 
	       use the "collective" context in the original intercommunicator */
	    if (comm_ptr->rank == 0) {
		int info[2];
		info[0] = new_context_id;
		info[1] = group_ptr->size;

		mpi_errno = MPIC_Sendrecv(info, 2, MPI_INT, 
					  0, 0, 
					  rinfo, 2, MPI_INT,
					  0, 0, comm, MPI_STATUS_IGNORE );
		if (mpi_errno) { MPIU_ERR_POP( mpi_errno ); }
		newcomm_ptr->recvcontext_id = rinfo[0];
		remote_size                 = rinfo[1];
		    
		MPIU_CHKLMEM_MALLOC(remote_mapping,int*,
				    remote_size*sizeof(int),
				    mpi_errno,"remote_mapping");

		/* Populate and exchange the ranks */
		mpi_errno = MPIC_Sendrecv( mapping, group_ptr->size, MPI_INT, 
					   0, 0, 
					   remote_mapping, remote_size, MPI_INT,
					   0, 0, comm, MPI_STATUS_IGNORE );
		if (mpi_errno) { MPIU_ERR_POP( mpi_errno ); }

		/* Broadcast to the other members of the local group */
                MPIR_Nest_incr();
		NMPI_Bcast( rinfo, 2, MPI_INT, 0, 
			    comm_ptr->local_comm->handle );
		NMPI_Bcast( remote_mapping, remote_size, MPI_INT, 0, 
			    comm_ptr->local_comm->handle );
                MPIR_Nest_decr();
	    }
	    else {
		/* The other processes */
		/* Broadcast to the other members of the local group */
                MPIR_Nest_incr();
		NMPI_Bcast( rinfo, 2, MPI_INT, 0, 
			    comm_ptr->local_comm->handle );
		newcomm_ptr->recvcontext_id = rinfo[0];
		remote_size                 = rinfo[1];
		MPIU_CHKLMEM_MALLOC(remote_mapping,int*,
				    remote_size*sizeof(int),
				    mpi_errno,"remote_mapping");
		NMPI_Bcast( remote_mapping, remote_size, MPI_INT, 0, 
			    comm_ptr->local_comm->handle );
                MPIR_Nest_decr();
	    }
	    newcomm_ptr->remote_size    = remote_size;
	    /* Now, everyone has the remote_mapping, and can apply that to 
	       the vcr table.  That step is done below, as it is the 
	       same step as needed for the intracommunicator */

	    /* Setup the communicator's local vc table from the local 
	       mapping. */
	    n = group_ptr->size;
	    MPID_VCRT_Create( n, &newcomm_ptr->local_vcrt );
	    MPID_VCRT_Get_ptr( newcomm_ptr->local_vcrt, 
			       &newcomm_ptr->local_vcr );
	    for (i=0; i<n; i++) {
		/* For rank i in the new communicator, find the corresponding
		   rank in the input communicator */
		MPID_VCR_Dup( comm_ptr->local_vcr[mapping[i]], 
			      &newcomm_ptr->local_vcr[i] );
		
	    }
	}

 	/* Setup the communicator's vc table.  This is for the remote group,
	   which is the same as the local group for intracommunicators */
	n = remote_size;
	MPID_VCRT_Create( n, &newcomm_ptr->vcrt );
	MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
	for (i=0; i<n; i++) {
	    /* For rank i in the new communicator, find the corresponding
	       rank in the input communicator */
	    MPID_VCR_Dup( comm_ptr->vcr[remote_mapping[i]], 
			  &newcomm_ptr->vcr[i] );
	}


	/* Notify the device of this new communicator */
	MPID_Dev_comm_create_hook( newcomm_ptr );
	
	*newcomm = newcomm_ptr->handle;
    }
    else {
	/* This process is not in the group */
	MPIR_Free_contextid( new_context_id );
	*newcomm = MPI_COMM_NULL;

	/* Dummy to complete collective ops in the intercomm case */
	if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	    int rinfo[2], ic_remote_size, *ic_remote_mapping = 0;
            MPIR_Nest_incr();
	    NMPI_Bcast( rinfo, 2, MPI_INT, 0, 
			comm_ptr->local_comm->handle );
	    ic_remote_size = rinfo[1];
	    MPIU_CHKLMEM_MALLOC(ic_remote_mapping,int*,
				ic_remote_size*sizeof(int),
				mpi_errno,"ic_remote_mapping");
	    NMPI_Bcast( ic_remote_mapping, ic_remote_size, MPI_INT, 0, 
			comm_ptr->local_comm->handle );
            MPIR_Nest_decr();
	}
    }
    
    /* ... end of body of routine ... */

    /* mpi_errno = MPID_Comm_create(); */
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE);
    MPIU_THREAD_SINGLE_CS_EXIT("comm");
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_create",
	    "**mpi_comm_create %C %G %p", comm, group, newcomm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

