/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Pcontrol */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Pcontrol = PMPI_Pcontrol
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Pcontrol  MPI_Pcontrol
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Pcontrol as PMPI_Pcontrol
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Pcontrol(const int level, ...) __attribute__ ((weak, alias("PMPI_Pcontrol")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Pcontrol
#define MPI_Pcontrol PMPI_Pcontrol

#endif


int MPI_Pcontrol(const int level, ...)
{
    QMPI_Context context;
    QMPI_Pcontrol_t *fn_ptr;

    context.storage_stack = NULL;

    va_list args;
    va_start(args, level);
    if (MPIR_QMPI_num_tools == 0)
        return QMPI_Pcontrol(context, 0, level, args);

    fn_ptr = (QMPI_Pcontrol_t *) MPIR_QMPI_first_fn_ptrs[MPI_PCONTROL_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_PCONTROL_T], level, args);
}

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
int QMPI_Pcontrol(QMPI_Context context, int tool_id, const int level, ...)
{
    int mpi_errno = MPI_SUCCESS;
    va_list list;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_PCONTROL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_PCONTROL);

    /* ... body of routine ...  */

    /* This is a dummy routine that does nothing.  It is intended for
     * use by the user (or a tool) with the profiling interface */
    /* We include a reference to va_start and va_end to (a) quiet some
     * compilers that warn when they are not present and (b) show how to
     * access any optional arguments */
    va_start(list, level);
    va_end(list);

    /* ... end of body of routine ... */
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_PCONTROL);
    return mpi_errno;
    /* There should never be any fn_fail case; this suppresses warnings from
     * compilers that object to unused labels */
}
