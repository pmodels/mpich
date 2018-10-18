/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_category_get_num */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_category_get_num = PMPI_T_category_get_num
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_category_get_num  MPI_T_category_get_num
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_category_get_num as PMPI_T_category_get_num
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_category_get_num(int *num_cat) __attribute__ ((weak, alias("PMPI_T_category_get_num")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_category_get_num
#define MPI_T_category_get_num PMPI_T_category_get_num
#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_T_category_get_num - Get the number of categories

Output Parameters:
. num_cat - current number of categories (integer)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
@*/
int MPI_T_category_get_num(int *num_cat)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_CATEGORY_GET_NUM);
    MPIT_ERRTEST_MPIT_INITIALIZED();
    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_CATEGORY_GET_NUM);

    /* Validate parameters */
    MPIT_ERRTEST_ARGNULL(num_cat);

    /* ... body of routine ...  */

    *num_cat = utarray_len(cat_table);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_CATEGORY_GET_NUM);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
