/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

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
                            int *num_cvars, int *num_pvars, int *num_categories)
                             __attribute__ ((weak, alias("PMPI_T_category_get_info")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_category_get_info
#define MPI_T_category_get_info PMPI_T_category_get_info

#endif

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
. num_cvars - number of control variables in the category (integer)
. num_pvars - number of performance variables in the category (integer)
- num_categories - number of categories contained in the category (integer)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS

.N MPI_T_ERR_INVALID_INDEX
.N MPI_T_ERR_NOT_INITIALIZED
@*/

int MPI_T_category_get_info(int cat_index, char *name, int *name_len, char *desc, int *desc_len,
                            int *num_cvars, int *num_pvars, int *num_categories)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_CATEGORY_GET_INFO);

    MPIT_ERRTEST_MPIT_INITIALIZED();

    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_CATEGORY_GET_INFO);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIT_ERRTEST_CAT_INDEX(cat_index);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    cat_table_entry_t *cat;
    cat = (cat_table_entry_t *) utarray_eltptr(cat_table, cat_index);
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
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_CATEGORY_GET_INFO);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
