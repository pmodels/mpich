/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "datatype.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_name = PMPI_Type_get_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_name  MPI_Type_get_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_name as PMPI_Type_get_name
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen)
    __attribute__ ((weak, alias("PMPI_Type_get_name")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_get_name
#define MPI_Type_get_name PMPI_Type_get_name

#endif

/*@
   MPI_Type_get_name - Get the print name for a datatype

Input Parameters:
. datatype - datatype whose name is to be returned (handle)

Output Parameters:
+ type_name - the name previously stored on the datatype, or a empty string
  if no such name exists (string)
- resultlen - length of returned name (integer)

.N ThreadSafeNoUpdate

.N NULL

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *datatype_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_GET_NAME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_GET_NAME);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Note that MPI_DATATYPE_NULL is invalid input to this routine;
     * it must not return a string for MPI_DATATYPE_NULL.  Instead,
     * it must return an error indicating an invalid datatype argument */

    /* Convert MPI object handles to object pointers */
    MPIR_Datatype_get_ptr(datatype, datatype_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate datatype_ptr */
            MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
            /* If datatype_ptr is not valid, it will be reset to null */
            MPIR_ERRTEST_ARGNULL(type_name, "type_name", mpi_errno);
            MPIR_ERRTEST_ARGNULL(resultlen, "resultlen", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    /* Include the null in MPI_MAX_OBJECT_NAME */
    MPL_strncpy(type_name, datatype_ptr->name, MPI_MAX_OBJECT_NAME);
    *resultlen = (int) strlen(type_name);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_GET_NAME);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_type_get_name", "**mpi_type_get_name %D %p %p", datatype,
                                 type_name, resultlen);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
