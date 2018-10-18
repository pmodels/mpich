/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_category_changed */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_category_changed = PMPI_T_category_changed
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_category_changed  MPI_T_category_changed
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_category_changed as PMPI_T_category_changed
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_category_changed(int *stamp) __attribute__ ((weak, alias("PMPI_T_category_changed")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_category_changed
#define MPI_T_category_changed PMPI_T_category_changed
#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_T_category_changed - Get the timestamp indicating the last change to the categories

Output Parameters:
. stamp - a virtual time stamp to indicate the last change to the categories (integer)

Notes:
If two subsequent calls to this routine return the same timestamp, it is guaranteed that
the category information has not changed between the two calls. If the timestamp retrieved
from the second call is higher, then some categories have been added or expanded.

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
@*/
int MPI_T_category_changed(int *stamp)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_CATEGORY_CHANGED);
    MPIT_ERRTEST_MPIT_INITIALIZED();
    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_CATEGORY_CHANGED);

    /* Validate parameters */
    MPIT_ERRTEST_ARGNULL(stamp);

    /* ... body of routine ...  */

    *stamp = cat_stamp;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_CATEGORY_CHANGED);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
