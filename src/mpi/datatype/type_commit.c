/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_commit */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_commit = PMPI_Type_commit
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_commit  MPI_Type_commit
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_commit as PMPI_Type_commit
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_commit(MPI_Datatype * datatype) __attribute__ ((weak, alias("PMPI_Type_commit")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_commit
#define MPI_Type_commit PMPI_Type_commit

int MPIR_Type_commit(MPI_Datatype * datatype_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *datatype_ptr;

    MPIR_Assert(!HANDLE_IS_BUILTIN(*datatype_p));

    MPIR_Datatype_get_ptr(*datatype_p, datatype_ptr);

    if (datatype_ptr->is_committed == 0) {
        datatype_ptr->is_committed = 1;

        MPIR_Typerep_commit(*datatype_p);

        MPL_DBG_MSG_D(MPIR_DBG_DATATYPE, TERSE, "# contig blocks = %d\n",
                      (int) datatype_ptr->typerep.num_contig_blocks);

        MPID_Type_commit_hook(datatype_ptr);

    }
    return mpi_errno;
}

int MPIR_Type_commit_impl(MPI_Datatype * datatype)
{
    int mpi_errno = MPI_SUCCESS;

    if (HANDLE_IS_BUILTIN(*datatype))
        goto fn_exit;

    /* pair types stored as real types are a special case */
    if (*datatype == MPI_FLOAT_INT ||
        *datatype == MPI_DOUBLE_INT ||
        *datatype == MPI_LONG_INT || *datatype == MPI_SHORT_INT || *datatype == MPI_LONG_DOUBLE_INT)
        goto fn_exit;

    mpi_errno = MPIR_Type_commit(datatype);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

/*@
    MPI_Type_commit - Commits the datatype

Input Parameters:
. datatype - datatype (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
@*/
int MPI_Type_commit(MPI_Datatype * datatype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_COMMIT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_COMMIT);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_DATATYPE(*datatype, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Datatype *datatype_ptr = NULL;

            /* Convert MPI object handles to object pointers */
            MPIR_Datatype_get_ptr(*datatype, datatype_ptr);

            /* Validate datatype_ptr */
            MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPIR_Type_commit_impl(datatype);
    MPIR_ERR_CHECK(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_COMMIT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_type_commit", "**mpi_type_commit %p", datatype);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
