/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_category_get_pvars */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_category_get_pvars = PMPI_T_category_get_pvars
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_category_get_pvars  MPI_T_category_get_pvars
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_category_get_pvars as PMPI_T_category_get_pvars
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_category_get_pvars(int cat_index, int len, int indices[])
     __attribute__ ((weak, alias("PMPI_T_category_get_pvars")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_category_get_pvars
#define MPI_T_category_get_pvars PMPI_T_category_get_pvars

#endif

/*@
   MPI_T_category_get_pvars - Get performance variables in a category

Input Parameters:
+ cat_index - index of the category to be queried, in the range $0$ and $num_cat-1$ (integer)
- len - the length of the indices array (integer)

Output Parameters:
. indices - an integer array of size len, indicating performance variable indices (integer)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS

.N MPI_T_ERR_INVALID
.N MPI_T_ERR_INVALID_INDEX
.N MPI_T_ERR_NOT_INITIALIZED
@*/

int MPI_T_category_get_pvars(int cat_index, int len, int indices[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_CATEGORY_GET_PVARS);

    MPIT_ERRTEST_MPIT_INITIALIZED();

    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_CATEGORY_GET_PVARS);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIT_ERRTEST_CAT_INDEX(cat_index);
            MPIT_ERRTEST_ARGNEG(len);
            if (len != 0) {
                MPIT_ERRTEST_ARGNULL(indices);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    if (len == 0) {
        goto fn_exit;
    }

    /* ... body of routine ... */
    mpi_errno = MPIR_T_category_get_pvars_impl(cat_index, len, indices);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_CATEGORY_GET_PVARS);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
