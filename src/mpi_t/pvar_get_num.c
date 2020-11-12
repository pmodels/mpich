/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_get_num */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_get_num = PMPI_T_pvar_get_num
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_get_num  MPI_T_pvar_get_num
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_get_num as PMPI_T_pvar_get_num
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_get_num(int *num_pvar) __attribute__ ((weak, alias("PMPI_T_pvar_get_num")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_get_num
#define MPI_T_pvar_get_num PMPI_T_pvar_get_num
#endif /* MPICH_MPI_FROM_PMPI */

int MPI_T_pvar_get_num(int *num_pvar)
{
    QMPI_Context context;
    QMPI_T_pvar_get_num_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_T_pvar_get_num(context, 0, num_pvar);

    fn_ptr = (QMPI_T_pvar_get_num_t *) MPIR_QMPI_first_fn_ptrs[MPI_T_PVAR_GET_NUM_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_T_PVAR_GET_NUM_T], num_pvar);
}

/*@
MPI_T_pvar_get_num - Get the number of performance variables

Output Parameters:
. num_pvar - returns number of performance variables (integer)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
@*/
int QMPI_T_pvar_get_num(QMPI_Context context, int tool_id, int *num_pvar)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_PVAR_GET_NUM);
    MPIR_ERRTEST_MPIT_INITIALIZED(mpi_errno);
    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_PVAR_GET_NUM);

    /* Validate parameters */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(num_pvar, "num_pvar", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    *num_pvar = utarray_len(pvar_table);

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_PVAR_GET_NUM);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

#ifdef HAVE_ERROR_CHECKING
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_t_pvar_get_num", "**mpi_t_pvar_get_num %p", num_pvar);
    }
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
#endif
}
