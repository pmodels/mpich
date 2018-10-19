/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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

#undef FUNCNAME
#define FUNCNAME MPI_T_pvar_get_num
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_T_pvar_get_num - Get the number of performance variables

Output Parameters:
. num_pvar - returns number of performance variables (integer)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
@*/
int MPI_T_pvar_get_num(int *num_pvar)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_PVAR_GET_NUM);
    MPIT_ERRTEST_MPIT_INITIALIZED();
    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_PVAR_GET_NUM);

    /* Validate parameters */
    MPIT_ERRTEST_ARGNULL(num_pvar);

    /* ... body of routine ...  */

    *num_pvar = utarray_len(pvar_table);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_PVAR_GET_NUM);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
