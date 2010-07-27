/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_spawn */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_spawn = PMPI_Comm_spawn
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_spawn  MPI_Comm_spawn
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_spawn as PMPI_Comm_spawn
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_spawn
#define MPI_Comm_spawn PMPI_Comm_spawn

/* Any internal routines can go here.  Make them static if possible */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_spawn

/*@
   MPI_Comm_spawn - Spawn up to maxprocs instances of a single MPI application

   Input Parameters:
+ command - name of program to be spawned (string, significant only at root) 
. argv - arguments to command (array of strings, significant only at root) 
. maxprocs - maximum number of processes to start (integer, significant only 
  at root) 
. info - a set of key-value pairs telling the runtime system where and how 
   to start the processes (handle, significant only at root) 
. root - rank of process in which previous arguments are examined (integer) 
- comm - intracommunicator containing group of spawning processes (handle) 

   Output Parameters:
+ intercomm - intercommunicator between original group and the 
   newly spawned group (handle) 
- array_of_errcodes - one code per process (array of integer) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_ARG
.N MPI_ERR_INFO
.N MPI_ERR_SPAWN
@*/
int MPI_Comm_spawn(char *command, char *argv[], int maxprocs, MPI_Info info, 
		   int root, MPI_Comm comm, MPI_Comm *intercomm,
		   int array_of_errcodes[])
{
    static const char FCNAME[] = "MPI_Comm_spawn";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL, *intercomm_ptr;
    MPID_Info *info_ptr=NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SPAWN);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SPAWN);

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
		MPIR_ERRTEST_INFO_OR_NULL(info, mpi_errno);
		MPIR_ERRTEST_ARGNULL(command, "command", mpi_errno);
		MPIR_ERRTEST_ARGNEG(maxprocs, "maxprocs", mpi_errno);
	    }

            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    if (comm_ptr->rank == root) {
	MPID_Info_get_ptr( info, info_ptr );
    }

    /* ... body of routine ...  */
    
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    mpi_errno = MPID_Comm_spawn_multiple(1, &command, &argv,
                                         &maxprocs, &info_ptr, root,  
                                         comm_ptr, &intercomm_ptr,
                                         array_of_errcodes); 
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIU_OBJ_PUBLISH_HANDLE(*intercomm, intercomm_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPAWN);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_spawn",
	    "**mpi_comm_spawn %s %p %d %I %d %C %p %p", command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
