/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_get_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_get_attr = PMPI_Comm_get_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_get_attr  MPI_Comm_get_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_get_attr as PMPI_Comm_get_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
     __attribute__ ((weak, alias("PMPI_Comm_get_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_get_attr
#define MPI_Comm_get_attr PMPI_Comm_get_attr

int MPII_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag,
                       MPIR_Attr_type attr_type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_GET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_GET_ATTR);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Comm_get_ptr(comm, comm_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, TRUE);
            if (mpi_errno) {
                goto fn_fail;
            }
            MPIR_ERRTEST_KEYVAL(comm_keyval, MPIR_COMM, "comm_keyval", mpi_errno);
            MPIR_ERRTEST_ARGNULL(attribute_val, "attribute_val", mpi_errno);
            MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_Comm_get_attr_impl(comm_ptr, comm_keyval, attribute_val, flag, attr_type);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_GET_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_comm_get_attr", "**mpi_comm_get_attr %C %K %p %p", comm,
                                     comm_keyval, attribute_val, flag);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
#endif /* MPICH_MPI_FROM_PMPI */

/*@
   MPI_Comm_get_attr - Retrieves attribute value by key

Input Parameters:
+ comm - communicator to which the attribute is attached (handle)
- comm_keyval - key value (integer)

Output Parameters:
+ attribute_val - attribute value, unless flag = false (None)
- flag - false if no attribute is associated with the key (logical)

Notes:
    Attributes must be extracted from the same language as they were inserted
    in with 'MPI_ATTR_PUT'.  The notes for C and Fortran below explain why.

.N ThreadSafe

Notes for C:
    Even though the 'attribute_val' argument is declared as 'void *', it is
    really the address of a void pointer (i.e., a 'void **').  Using
    a 'void *', however, is more in keeping with C idiom and allows the
    pointer to be passed without additional casts.

.N Fortran
    The 'attribute_val' in Fortran is a pointer to a Fortran integer, not
    a pointer to a 'void *'.

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_COMM
@*/

int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_GET_ATTR);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_GET_ATTR);

    mpi_errno = MPII_Comm_get_attr(comm, comm_keyval, attribute_val, flag, MPIR_ATTR_PTR);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_GET_ATTR);
    return mpi_errno;
}
