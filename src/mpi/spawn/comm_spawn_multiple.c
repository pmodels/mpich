/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_spawn_multiple */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_spawn_multiple = PMPI_Comm_spawn_multiple
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_spawn_multiple  MPI_Comm_spawn_multiple
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_spawn_multiple as PMPI_Comm_spawn_multiple
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_spawn_multiple
#define MPI_Comm_spawn_multiple PMPI_Comm_spawn_multiple

/* Any internal routines can go here.  Make them static if possible */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_spawn_multiple

/*@
   MPI_Comm_spawn_multiple - short description

   Input Parameters:
+ count - number of commands (positive integer, significant to MPI only at 
  root 
. array_of_commands - programs to be executed (array of strings, significant 
  only at root) 
. array_of_argv - arguments for commands (array of array of strings, 
  significant only at root) 
. array_of_maxprocs - maximum number of processes to start for each command 
 (array of integer, significant only at root) 
. array_of_info - info objects telling the runtime system where and how to 
  start processes (array of handles, significant only at root) 
. root - rank of process in which previous arguments are examined (integer) 
- comm - intracommunicator containing group of spawning processes (handle) 

  Output Parameters:
+ intercomm - intercommunicator between original group and newly spawned group
  (handle) 
- array_of_errcodes - one error code per process (array of integer) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_ARG
.N MPI_ERR_INFO
.N MPI_ERR_SPAWN
@*/
int MPI_Comm_spawn_multiple(int count, char *array_of_commands[], 
			    char* *array_of_argv[], int array_of_maxprocs[], 
			    MPI_Info array_of_info[], int root, MPI_Comm comm, 
			    MPI_Comm *intercomm, int array_of_errcodes[]) 
{
    static const char FCNAME[] = "MPI_Comm_spawn_multiple";
    int mpi_errno = MPI_SUCCESS, i;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *intercomm_ptr = NULL;
    MPID_Info **array_of_info_ptrs = NULL;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SPAWN_MULTIPLE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SPAWN_MULTIPLE);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;

	    MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
	    MPIR_ERRTEST_RANK(comm_ptr, root, mpi_errno);

	    if (comm_ptr->rank == root) {
		MPIR_ERRTEST_ARGNULL(array_of_commands, "array_of_commands", mpi_errno);
		MPIR_ERRTEST_ARGNULL(array_of_maxprocs, "array_of_maxprocs", mpi_errno);
		MPIR_ERRTEST_ARGNONPOS(count, "count", mpi_errno);
		for (i = 0; i < count; i++)
		{
		    MPIR_ERRTEST_INFO_OR_NULL(array_of_info[i], mpi_errno);
		    MPIR_ERRTEST_ARGNULL(array_of_commands[i], "array_of_commands[i]", mpi_errno);
		    MPIR_ERRTEST_ARGNEG(array_of_maxprocs[i], "array_of_maxprocs[i]", mpi_errno);
		}
		if (mpi_errno) goto fn_fail;
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    if (comm_ptr->rank == root) {
	MPIU_CHKLMEM_MALLOC(array_of_info_ptrs, MPID_Info **, count * sizeof(MPID_Info*), mpi_errno, "array of info pointers");
	for (i=0; i<count; i++)
	{
	    MPID_Info_get_ptr(array_of_info[i], array_of_info_ptrs[i]);
	}
    }

    /* TODO: add error check to see if this collective function is
       being called from multiple threads. */
    mpi_errno = MPID_Comm_spawn_multiple(count, array_of_commands,
                                         array_of_argv,
                                         array_of_maxprocs,
                                         array_of_info_ptrs, root, 
                                         comm_ptr, &intercomm_ptr,
                                         array_of_errcodes);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIU_OBJ_PUBLISH_HANDLE(*intercomm, intercomm_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPAWN_MULTIPLE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_spawn_multiple",
	    "**mpi_comm_spawn_multiple %d %p %p %p %p %d %C %p %p", count, array_of_commands, array_of_argv, array_of_maxprocs,
	    array_of_info, root, comm, intercomm, array_of_errcodes);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
