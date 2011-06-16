/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Foo */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Foo = PMPI_Foo
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Foo  MPI_Foo
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Foo as PMPI_Foo
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
#ifndef MPICH_MPI_FROM_PMPI
/* The PMPI routine is built using a CPP macro to rename the MPI routine
   implemented below.  The MPI name must be undefined first to prevent any
   conflicts with previous renamings, such as those put in place by the
   globus device when it is building on top of a vendor MPI. */
#undef MPI_Foo
#define MPI_Foo PMPI_Foo

/* Any internal routines can go here.  Make them static if possible.  If they
   are used by both the MPI and PMPI versions, use PMPI_LOCAL instead of 
   static; this macro expands into "static" if weak symbols are supported and
   into nothing otherwise. */
int MPIR_Foo_util( int a, MPID_Comm *comm )
{
...
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Foo

/*@
   MPI_Foo - short description

   Input Arguments:
+  first - 
.  middle - 
-  last - 

   Output Arguments:

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPI_Foo( MPI_Comm comm, MPI_Datatype dataype, int a ) 
{
    static const char FCNAME[] = "MPI_Foo";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FOO);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FOO);

    /* Validate handle parameters needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Datatype *datatype_ptr = NULL;

            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;

            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FOO);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
	    MPI_ERR_OTHER, "**mpi_foo",
	    "**mpi_foo %C %D %d", comm, datatype, a);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
