/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Wtick */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Wtick = PMPI_Wtick
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Wtick  MPI_Wtick
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Wtick as PMPI_Wtick
#elif defined(HAVE_WEAK_ATTRIBUTE)
double MPI_Wtick(void) __attribute__ ((weak, alias("PMPI_Wtick")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Wtick
#define MPI_Wtick PMPI_Wtick

#endif


double MPI_Wtick(void)
{
    QMPI_Context context;
    QMPI_Wtick_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_Wtick(context, 0);

    fn_ptr = (QMPI_Wtick_t *) MPIR_QMPI_first_fn_ptrs[MPI_WTICK_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_WTICK_T]);
}

/*@
  MPI_Wtick - Returns the resolution of MPI_Wtime

  Return value:
  Time in seconds of resolution of MPI_Wtime

  Notes for Fortran:
  This is a function, declared as 'DOUBLE PRECISION MPI_WTICK()' in Fortran.

.see also: MPI_Wtime, MPI_Comm_get_attr, MPI_Attr_get
@*/
double QMPI_Wtick(QMPI_Context context, int tool_id)
{
    double tick;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WTICK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_WTICK);
    MPL_wtick(&tick);
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_WTICK);

    return tick;
}
