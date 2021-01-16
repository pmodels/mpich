/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_subarray */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_subarray = PMPI_Type_create_subarray
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_subarray  MPI_Type_create_subarray
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_subarray as PMPI_Type_create_subarray
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_create_subarray(int ndims, const int array_of_sizes[],
                             const int array_of_subsizes[], const int array_of_starts[],
                             int order, MPI_Datatype oldtype, MPI_Datatype * newtype)
    __attribute__ ((weak, alias("PMPI_Type_create_subarray")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_create_subarray
#define MPI_Type_create_subarray PMPI_Type_create_subarray

#endif

/*@
   MPI_Type_create_subarray - Create a datatype for a subarray of a regular,
    multidimensional array

Input Parameters:
+ ndims - number of array dimensions (positive integer)
. array_of_sizes - number of elements of type oldtype in each dimension of the
  full array (array of positive integers)
. array_of_subsizes - number of elements of type oldtype in each dimension of
  the subarray (array of positive integers)
. array_of_starts - starting coordinates of the subarray in each dimension
  (array of nonnegative integers)
. order - array storage order flag (state)
- oldtype - array element datatype (handle)

Output Parameters:
. newtype - new datatype (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_create_subarray(int ndims,
                             const int array_of_sizes[],
                             const int array_of_subsizes[],
                             const int array_of_starts[],
                             int order, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS, i;

    MPI_Aint extent;
#ifdef HAVE_ERROR_CHECKING
    MPI_Aint size_with_aint;
    MPI_Offset size_with_offset;
#endif

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_SUBARRAY);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_CREATE_SUBARRAY);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Datatype *datatype_ptr = NULL;

            /* Check parameters */
            MPIR_ERRTEST_ARGNONPOS(ndims, "ndims", mpi_errno, MPI_ERR_DIMS);
            MPIR_ERRTEST_ARGNULL(array_of_sizes, "array_of_sizes", mpi_errno);
            MPIR_ERRTEST_ARGNULL(array_of_subsizes, "array_of_subsizes", mpi_errno);
            MPIR_ERRTEST_ARGNULL(array_of_starts, "array_of_starts", mpi_errno);
            for (i = 0; mpi_errno == MPI_SUCCESS && i < ndims; i++) {
                MPIR_ERRTEST_ARGNONPOS(array_of_sizes[i], "size", mpi_errno, MPI_ERR_ARG);
                MPIR_ERRTEST_ARGNONPOS(array_of_subsizes[i], "subsize", mpi_errno, MPI_ERR_ARG);
                MPIR_ERRTEST_ARGNEG(array_of_starts[i], "start", mpi_errno);
                if (array_of_subsizes[i] > array_of_sizes[i]) {
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                     MPIR_ERR_RECOVERABLE,
                                                     __func__,
                                                     __LINE__,
                                                     MPI_ERR_ARG,
                                                     "**argrange",
                                                     "**argrange %s %d %d",
                                                     "array_of_subsizes",
                                                     array_of_subsizes[i], array_of_sizes[i]);
                    goto fn_fail;
                }
                if (array_of_starts[i] > (array_of_sizes[i] - array_of_subsizes[i])) {
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                     MPIR_ERR_RECOVERABLE,
                                                     __func__,
                                                     __LINE__,
                                                     MPI_ERR_ARG,
                                                     "**argrange",
                                                     "**argrange %s %d %d",
                                                     "array_of_starts",
                                                     array_of_starts[i],
                                                     array_of_sizes[i] - array_of_subsizes[i]);
                    goto fn_fail;
                }
            }
            if (order != MPI_ORDER_FORTRAN && order != MPI_ORDER_C) {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                 MPIR_ERR_RECOVERABLE,
                                                 __func__,
                                                 __LINE__,
                                                 MPI_ERR_ARG, "**storageorder",
                                                 "**storageorder %d", order);
                goto fn_fail;
            }

            MPIR_Datatype_get_extent_macro(oldtype, extent);

            /* check if MPI_Aint is large enough for size of global array.
             * if not, complain. */

            size_with_aint = extent;
            for (i = 0; i < ndims; i++)
                size_with_aint *= array_of_sizes[i];
            size_with_offset = extent;
            for (i = 0; i < ndims; i++)
                size_with_offset *= array_of_sizes[i];
            if (size_with_aint != size_with_offset) {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                 MPIR_ERR_FATAL,
                                                 __func__,
                                                 __LINE__,
                                                 MPI_ERR_ARG,
                                                 "**subarrayoflow",
                                                 "**subarrayoflow %L", size_with_offset);
                goto fn_fail;
            }

            /* Get handles to MPI objects. */
            MPIR_Datatype_get_ptr(oldtype, datatype_ptr);

            /* Validate datatype_ptr */
            MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
            /* If datatype_ptr is not valid, it will be reset to null */
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_Type_create_subarray(ndims, array_of_sizes, array_of_subsizes,
                                          array_of_starts, order, oldtype, newtype);
    if (mpi_errno) {
        goto fn_fail;
    }

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_CREATE_SUBARRAY);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_type_create_subarray",
                                 "**mpi_type_create_subarray %d %p %p %p %d %D %p", ndims,
                                 array_of_sizes, array_of_subsizes, array_of_starts, order, oldtype,
                                 newtype);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
