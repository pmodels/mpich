/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_delete_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_delete_attr = PMPI_Type_delete_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_delete_attr  MPI_Type_delete_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_delete_attr as PMPI_Type_delete_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval)
     __attribute__ ((weak, alias("PMPI_Type_delete_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_delete_attr
#define MPI_Type_delete_attr PMPI_Type_delete_attr

#endif

/*@
   MPI_Type_delete_attr - Deletes an attribute value associated with a key on a datatype

Input Parameters:
+ datatype - datatype from which the attribute is deleted (handle)
- type_keyval - key value (integer)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_KEYVAL
.N MPI_ERR_TYPE
@*/

int MPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *datatype_ptr = NULL;
    MPII_Keyval *type_keyval_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_DELETE_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_DELETE_ATTR);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_KEYVAL(type_keyval, MPIR_DATATYPE, "datatype", mpi_errno);
            MPIR_ERRTEST_KEYVAL_PERM(type_keyval, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Datatype_get_ptr(datatype, datatype_ptr);
    MPII_Keyval_get_ptr(type_keyval, type_keyval_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
            if (mpi_errno) {
                goto fn_fail;
            }
            MPII_Keyval_valid_ptr(type_keyval_ptr, mpi_errno);
            if (mpi_errno) {
                goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_Type_delete_attr_impl(datatype_ptr, type_keyval_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_DELETE_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_type_delete_attr", "**mpi_type_delete_attr %D %K", datatype,
                                     type_keyval);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
