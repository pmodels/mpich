/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_remote_group */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_remote_group = PMPI_Comm_remote_group
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_remote_group  MPI_Comm_remote_group
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_remote_group as PMPI_Comm_remote_group
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group) __attribute__((weak,alias("PMPI_Comm_remote_group")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_remote_group
#define MPI_Comm_remote_group PMPI_Comm_remote_group

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_remote_group_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_remote_group_impl(MPID_Comm *comm_ptr, MPID_Group **group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, lpid, n;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_REMOTE_GROUP_IMPL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_REMOTE_GROUP_IMPL);
    /* Create a group and populate it with the local process ids */
    if (!comm_ptr->remote_group) {
        n = comm_ptr->remote_size;
        mpi_errno = MPIR_Group_create( n, group_ptr );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        for (i=0; i<n; i++) {
            (void) MPID_Comm_get_lpid( comm_ptr, i, &lpid, TRUE );
            (*group_ptr)->lrank_to_lpid[i].lpid  = lpid;
            /* TODO calculate is_local_dense_monotonic */
        }
        (*group_ptr)->size = n;
        (*group_ptr)->rank = MPI_UNDEFINED;
        (*group_ptr)->idx_of_first_lpid = -1;
        
        comm_ptr->remote_group = *group_ptr;
    } else {
        *group_ptr = comm_ptr->remote_group;
    }
    MPIR_Group_add_ref( comm_ptr->remote_group );

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_REMOTE_GROUP_IMPL);
    return mpi_errno;
 fn_fail:

    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_remote_group
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Comm_remote_group - Accesses the remote group associated with 
                        the given inter-communicator

Input Parameters:
. comm - Communicator (must be an intercommunicator) (handle)

Output Parameters:
. group - remote group of communicator (handle)

Notes:
The user is responsible for freeing the group when it is no longer needed.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM

.seealso MPI_Group_free
@*/
int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Group *group_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_REMOTE_GROUP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_REMOTE_GROUP);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
	    /* If comm_ptr is not valid, it will be reset to null */
	    if (comm_ptr && comm_ptr->comm_kind != MPID_INTERCOMM) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                      MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM, 
						  "**commnotinter", 0 );
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Comm_remote_group_impl(comm_ptr, &group_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    *group = group_ptr->handle;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_REMOTE_GROUP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_remote_group",
	    "**mpi_comm_remote_group %C %p", comm, group);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

