/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_T_category_get_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_T_category_get_info = PMPIX_T_category_get_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_T_category_get_info  MPIX_T_category_get_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_T_category_get_info as PMPIX_T_category_get_info
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_T_category_get_info
#define MPIX_T_category_get_info PMPIX_T_category_get_info

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_category_get_info_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_T_category_get_info_impl(int cat_index, char *name, int *name_len, char *desc, int *desc_len, int *num_controlvars, int *num_pvars, int *num_categories)
{
    int mpi_errno = MPI_SUCCESS;

    /* TODO implement this function */

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPIX_T_category_get_info
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_T_category_get_info - XXX description here

Input/Output Parameters:
+ name_len - length of the string and/or buffer for name (integer)
- desc_len - length of the string and/or buffer for desc (integer)

Input Parameters:
. cat_index - index of the category to be queried (integer)

Output Parameters:
+ name - buffer to return the string containing the name of the category (string)
. desc - buffer to return the string containing the description of the category (string)
. num_controlvars - number of control variables contained in the category (integer)
. num_pvars - number of performance variables contained in the category (integer)
- num_categories - number of categories contained in the category (integer)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_T_category_get_info(int cat_index, char *name, int *name_len, char *desc, int *desc_len, int *num_controlvars, int *num_pvars, int *num_categories)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_T_CATEGORY_GET_INFO);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_T_CATEGORY_GET_INFO);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {

            /* TODO more checks may be appropriate */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_ARGNULL(name_len, "name_len", mpi_errno);
            MPIR_ERRTEST_ARGNULL(desc_len, "desc_len", mpi_errno);
            MPIR_ERRTEST_ARGNULL(num_controlvars, "num_controlvars", mpi_errno);
            MPIR_ERRTEST_ARGNULL(num_pvars, "num_pvars", mpi_errno);
            MPIR_ERRTEST_ARGNULL(num_categories, "num_categories", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_category_get_info_impl(cat_index, name, name_len, desc, desc_len, num_controlvars, num_pvars, num_categories);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_T_CATEGORY_GET_INFO);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_t_category_get_info", "**mpix_t_category_get_info %d %p %p %p %p %p %p %p", cat_index, name, name_len, desc, desc_len, num_controlvars, num_pvars, num_categories);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
