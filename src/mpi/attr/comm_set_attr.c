/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_set_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_set_attr = PMPI_Comm_set_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_set_attr  MPI_Comm_set_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_set_attr as PMPI_Comm_set_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val)
    __attribute__ ((weak, alias("PMPI_Comm_set_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_set_attr
#define MPI_Comm_set_attr PMPI_Comm_set_attr

int MPII_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val,
                       MPIR_Attr_type attr_type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPII_Keyval *keyval_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPII_COMM_SET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPII_COMM_SET_ATTR);

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

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, TRUE);
            /* If comm_ptr is not valid, it will be reset to null */
            /* Validate keyval_ptr */
            MPIR_ERR_CHKANDJUMP(comm_keyval == MPI_KEYVAL_INVALID, mpi_errno, MPI_ERR_KEYVAL,
                                "**keyvalinvalid");

            MPII_Keyval_get_ptr(comm_keyval, keyval_ptr);
            MPII_Keyval_valid_ptr(keyval_ptr, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Comm_set_attr_impl(comm_ptr, keyval_ptr, attribute_val, attr_type);
    if (mpi_errno)
        goto fn_fail;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPII_COMM_SET_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_comm_set_attr", "**mpi_comm_set_attr %C %d %p", comm,
                                 comm_keyval, attribute_val);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
#endif /* MPICH_MPI_FROM_PMPI */

/*@
   MPI_Comm_set_attr - Stores attribute value associated with a key

Input Parameters:
+ comm - communicator to which attribute will be attached (handle)
. comm_keyval - key value, as returned by  'MPI_Comm_create_keyval' (integer)
- attribute_val - attribute value

Notes:
Values of the permanent attributes 'MPI_TAG_UB', 'MPI_HOST', 'MPI_IO',
'MPI_WTIME_IS_GLOBAL', 'MPI_UNIVERSE_SIZE', 'MPI_LASTUSEDCODE', and
'MPI_APPNUM' may not be changed.

The type of the attribute value depends on whether C, C++, or Fortran
is being used.
In C and C++, an attribute value is a pointer ('void *'); in Fortran, it is an
address-sized integer.

If an attribute is already present, the delete function (specified when the
corresponding keyval was created) will be called.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_KEYVAL
.N MPI_ERR_PERM_KEY

.seealso MPI_Comm_create_keyval, MPI_Comm_delete_attr
@*/
int MPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_SET_ATTR);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_SET_ATTR);

    mpi_errno = MPII_Comm_set_attr(comm, comm_keyval, attribute_val, MPIR_ATTR_PTR);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_SET_ATTR);
    return mpi_errno;
}
