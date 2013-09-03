/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_category_get_num */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_category_get_num = PMPI_T_category_get_num
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_category_get_num  MPI_T_category_get_num
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_category_get_num as PMPI_T_category_get_num
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_category_get_num
#define MPI_T_category_get_num PMPI_T_category_get_num
#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_category_get_num
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPI_T_category_get_num - XXX description here

Output Parameters:
. num_cat - current number of categories (integer)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_T_category_get_num(int *num_cat)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_CATEGORY_GET_NUM);
    MPIR_T_FAIL_IF_UNINITIALIZED();
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_CATEGORY_GET_NUM);

    /* ... body of routine ...  */

    if (*num_cat)
        *num_cat = utarray_len(cat_table);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_CATEGORY_GET_NUM);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_category_get_num", "**mpi_t_category_get_num %p", num_cat);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
