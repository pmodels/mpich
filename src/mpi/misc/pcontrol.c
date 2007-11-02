/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Pcontrol */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Pcontrol = PMPI_Pcontrol
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Pcontrol  MPI_Pcontrol
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Pcontrol as PMPI_Pcontrol
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Pcontrol
#define MPI_Pcontrol PMPI_Pcontrol

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Pcontrol

/*@
  MPI_Pcontrol - Controls profiling

  Input Parameters:
+ level - Profiling level 
-  ... - other arguments (see notes)

  Notes:
  This routine provides a common interface for profiling control.  The
  interpretation of 'level' and any other arguments is left to the
  profiling library.  The intention is that a profiling library will 
  provide a replacement for this routine and define the interpretation
  of the parameters.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Pcontrol(const int level, ...)
{
    int mpi_errno = MPI_SUCCESS;
    va_list list;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_PCONTROL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_PCONTROL);

    /* ... body of routine ...  */
    
    /* This is a dummy routine that does nothing.  It is intended for 
       use by the user (or a tool) with the profiling interface */
    /* We include a reference to va_start and va_end to (a) quiet some
       compilers that warn when they are not present and (b) show how to 
       access any optional arguments */
    va_start( list, level );
    va_end( list );

    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PCONTROL);
    return mpi_errno;
    /* There should never be any fn_fail case; this suppresses warnings from
       compilers that object to unused labels */
}
