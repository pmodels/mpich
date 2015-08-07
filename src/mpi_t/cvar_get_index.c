/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_cvar_get_index */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_cvar_get_index = PMPI_T_cvar_get_index
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_cvar_get_index  MPI_T_cvar_get_index
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_cvar_get_index as PMPI_T_cvar_get_index
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_cvar_get_index(const char *name, int *cvar_index) __attribute__((weak,alias("PMPI_T_cvar_get_index")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_cvar_get_index
#define MPI_T_cvar_get_index PMPI_T_cvar_get_index
#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_cvar_get_index
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_T_cvar_get_index - Get the index of a control variable

Output Parameters:
. name - name of the control variable (string)

Output Parameters:
. cvar_index - index of the control variable (integer)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_INVALID_NAME
.N MPI_T_ERR_NOT_INITIALIZED
@*/
int MPI_T_cvar_get_index(const char *name, int *cvar_index)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_CVAR_GET_INDEX);
    MPIR_ERRTEST_MPIT_INITIALIZED(mpi_errno);
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_CVAR_GET_INDEX);

    /* Validate parameters */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_ARGNULL(name, "name", mpi_errno);
            MPIR_ERRTEST_ARGNULL(cvar_index, "cvar_index", mpi_errno);
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    name2index_hash_t *hash_entry;

    /* Do hash lookup by the name */
    HASH_FIND_STR(cvar_hash, name, hash_entry);
    if (hash_entry != NULL) {
        *cvar_index = hash_entry->idx;
    } else {
        mpi_errno = MPI_T_ERR_INVALID_NAME;
        goto fn_fail;
    }

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_CVAR_GET_INDEX);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    goto fn_exit;
}
