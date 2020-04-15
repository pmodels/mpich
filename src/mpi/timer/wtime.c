/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* -- Begin Profiling Symbol Block for routine MPI_Wtime */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Wtime = PMPI_Wtime
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Wtime  MPI_Wtime
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Wtime as PMPI_Wtime
#elif defined(HAVE_WEAK_ATTRIBUTE)
double MPI_Wtime(void) __attribute__ ((weak, alias("PMPI_Wtime")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Wtime
#define MPI_Wtime PMPI_Wtime
#endif


/*@
  MPI_Wtime - Returns an elapsed time on the calling processor

  Return value:
  Time in seconds since an arbitrary time in the past.

  Notes:
  This is intended to be a high-resolution, elapsed (or wall) clock.
  See 'MPI_WTICK' to determine the resolution of 'MPI_WTIME'.
  If the attribute 'MPI_WTIME_IS_GLOBAL' is defined and true, then the
  value is synchronized across all processes in 'MPI_COMM_WORLD'.

  Notes for Fortran:
  This is a function, declared as 'DOUBLE PRECISION MPI_WTIME()' in Fortran.

.see also: MPI_Wtick, MPI_Comm_get_attr, MPI_Attr_get
@*/
double MPI_Wtime(void)
{
    double d;
    MPL_time_t t;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WTIME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WTIME);
    MPL_wtime(&t);
    MPL_wtime_todouble(&t, &d);
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WTIME);

    return d;
}
