/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_delete_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_delete_attr = PMPI_Comm_delete_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_delete_attr  MPI_Comm_delete_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_delete_attr as PMPI_Comm_delete_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval)
    __attribute__ ((weak, alias("PMPI_Comm_delete_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_delete_attr
#define MPI_Comm_delete_attr PMPI_Comm_delete_attr

#endif

int MPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval)
{
    QMPI_Context context;
    QMPI_Comm_delete_attr_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_Comm_delete_attr(context, 0, comm, comm_keyval);

    fn_ptr = (QMPI_Comm_delete_attr_t *) MPIR_QMPI_first_fn_ptrs[MPI_COMM_DELETE_ATTR_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_COMM_DELETE_ATTR_T], comm, comm_keyval);
}

/*@
   MPI_Comm_delete_attr - Deletes an attribute value associated with a key on
   a  communicator

Input Parameters:
+ comm - communicator to which attribute is attached (handle)
- comm_keyval - The key value of the deleted attribute (integer)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_PERM_KEY

.seealso MPI_Comm_set_attr, MPI_Comm_create_keyval
@*/
int QMPI_Comm_delete_attr(QMPI_Context context, int tool_id, MPI_Comm comm, int comm_keyval)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPII_Keyval *keyval_ptr;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_DELETE_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_DELETE_ATTR);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
            MPIR_ERRTEST_KEYVAL(comm_keyval, MPIR_COMM, "communicator", mpi_errno);
            MPIR_ERRTEST_KEYVAL_PERM(comm_keyval, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);
    MPII_Keyval_get_ptr(comm_keyval, keyval_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, TRUE);
            /* If comm_ptr is not valid, it will be reset to null */
            /* Validate keyval_ptr */
            MPII_Keyval_valid_ptr(keyval_ptr, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Comm_delete_attr_impl(comm_ptr, keyval_ptr);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_DELETE_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_comm_delete_attr", "**mpi_comm_delete_attr %C %d", comm,
                                 comm_keyval);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
