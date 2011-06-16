/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#include "rma.h"
/* -- Begin Profiling Symbol Block for routine MPI_Put */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Put = PMPI_Put
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Put  MPI_Put
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Put as PMPI_Put
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Put
#define MPI_Put PMPI_Put

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Put

/*@
   MPI_Put - Put data into a memory window on a remote process

   Input Parameters:
+ origin_addr -initial address of origin buffer (choice) 
. origin_count -number of entries in origin buffer (nonnegative integer) 
. origin_datatype -datatype of each entry in origin buffer (handle) 
. target_rank -rank of target (nonnegative integer) 
. target_disp -displacement from start of window to target buffer (nonnegative integer) 
. target_count -number of entries in target buffer (nonnegative integer) 
. target_datatype -datatype of each entry in target buffer (handle) 

- win - window object used for communication (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_COUNT
.N MPI_ERR_RANK
.N MPI_ERR_TYPE
.N MPI_ERR_WIN
@*/
int MPI_Put(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPI_Win
            win)
{
    static const char FCNAME[] = "MPI_Put";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_PUT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_PUT);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_WIN(win, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Win_get_ptr( win, win_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Comm * comm_ptr;

            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;

	    MPIR_ERRTEST_COUNT(origin_count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(origin_datatype, "origin_datatype", 
				  mpi_errno);
	    MPIR_ERRTEST_COUNT(target_count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(target_datatype, "target_datatype",
				  mpi_errno);
	    MPIR_ERRTEST_DISP(target_disp, mpi_errno);

            if (HANDLE_GET_KIND(origin_datatype) != HANDLE_KIND_BUILTIN)
            {
                MPID_Datatype *datatype_ptr = NULL;
                
                MPID_Datatype_get_ptr(origin_datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
            }

            if (HANDLE_GET_KIND(target_datatype) != HANDLE_KIND_BUILTIN)
            {
                MPID_Datatype *datatype_ptr = NULL;
                
                MPID_Datatype_get_ptr(target_datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
            }

	    comm_ptr = win_ptr->comm_ptr;
	    MPIR_ERRTEST_SEND_RANK(comm_ptr, target_rank, mpi_errno);
	    
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    if (target_rank == MPI_PROC_NULL) goto fn_exit;

    mpi_errno = MPIU_RMA_CALL(win_ptr,
			      Put(origin_addr, origin_count, origin_datatype,
				  target_rank, target_disp, target_count,
				  target_datatype, win_ptr));
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_PUT);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_put", "**mpi_put %p %d %D %d %d %d %D %W",
	    origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

