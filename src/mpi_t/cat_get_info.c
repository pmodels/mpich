/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_category_get_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_category_get_info = PMPI_T_category_get_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_category_get_info  MPI_T_category_get_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_category_get_info as PMPI_T_category_get_info
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_category_get_info(int cat_index, char *name, int *name_len, char *desc, int *desc_len,
                            int *num_cvars, int *num_pvars, int *num_categories) __attribute__((weak,alias("PMPI_T_category_get_info")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_category_get_info
#define MPI_T_category_get_info PMPI_T_category_get_info
#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_category_get_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_T_category_get_info - Get the information about a category

Input/Output Parameters:
+ name_len - length of the string and/or buffer for name (integer)
- desc_len - length of the string and/or buffer for desc (integer)

Input Parameters:
. cat_index - index of the category to be queried (integer)

Output Parameters:
+ name - buffer to return the string containing the name of the category (string)
. desc - buffer to return the string containing the description of the category (string)
. num_cvars - number of control variables contained in the category (integer)
. num_pvars - number of performance variables contained in the category (integer)
- num_categories - number of categories contained in the category (integer)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_INDEX
@*/
int MPI_T_category_get_info(int cat_index, char *name, int *name_len, char *desc,
        int *desc_len, int *num_cvars, int *num_pvars, int *num_categories)
{
    int mpi_errno = MPI_SUCCESS;
    cat_table_entry_t *cat;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_CATEGORY_GET_INFO);
    MPIR_ERRTEST_MPIT_INITIALIZED(mpi_errno);
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_CATEGORY_GET_INFO);

    /* Validate parameters */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_CAT_INDEX(cat_index, mpi_errno);
            /* Do not do _TEST_ARGNULL for other arguments, since this is
             * allowed or will be allowed by MPI_T standard.
             */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    cat = (cat_table_entry_t *)utarray_eltptr(cat_table, cat_index);
    MPIR_T_strncpy(name, cat->name, name_len);
    MPIR_T_strncpy(desc, cat->desc, desc_len);

    if (num_cvars != NULL)
        *num_cvars = utarray_len(cat->cvar_indices);

    if (num_pvars != NULL)
        *num_pvars = utarray_len(cat->pvar_indices);

    if (num_categories != NULL)
        *num_categories = utarray_len(cat->subcat_indices);
    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_CATEGORY_GET_INFO);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_category_get_info", "**mpi_t_category_get_info %d %p %p %p %p %p %p %p",
            cat_index, name, name_len, desc, desc_len, num_cvars, num_pvars, num_categories);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
