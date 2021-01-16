/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_darray */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_darray = PMPI_Type_create_darray
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_darray  MPI_Type_create_darray
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_darray as PMPI_Type_create_darray
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_create_darray(int size, int rank, int ndims, const int array_of_gsizes[],
                           const int array_of_distribs[], const int array_of_dargs[],
                           const int array_of_psizes[], int order, MPI_Datatype oldtype,
                           MPI_Datatype * newtype)
    __attribute__ ((weak, alias("PMPI_Type_create_darray")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_create_darray
#define MPI_Type_create_darray PMPI_Type_create_darray
#endif

/*@
   MPI_Type_create_darray - Create a datatype representing a distributed array

Input Parameters:
+ size - size of process group (positive integer)
. rank - rank in process group (nonnegative integer)
. ndims - number of array dimensions as well as process grid dimensions (positive integer)
. array_of_gsizes - number of elements of type oldtype in each dimension of global array (array of positive integers)
. array_of_distribs - distribution of array in each dimension (array of state)
. array_of_dargs - distribution argument in each dimension (array of positive integers)
. array_of_psizes - size of process grid in each dimension (array of positive integers)
. order - array storage order flag (state)
- oldtype - old datatype (handle)

Output Parameters:
. newtype - new datatype (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_create_darray(int size,
                           int rank,
                           int ndims,
                           const int array_of_gsizes[],
                           const int array_of_distribs[],
                           const int array_of_dargs[],
                           const int array_of_psizes[],
                           int order, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_Datatype *datatype_ptr = NULL;
    MPI_Aint orig_extent;
    int tmp_size;
#ifdef HAVE_ERROR_CHECKING
    MPI_Aint size_with_aint;
    MPI_Offset size_with_offset;
#endif

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_DARRAY);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_CREATE_DARRAY);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Datatype_get_ptr(oldtype, datatype_ptr);
    MPIR_Datatype_get_extent_macro(oldtype, orig_extent);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Check parameters */
            MPIR_ERRTEST_ARGNONPOS(size, "size", mpi_errno, MPI_ERR_ARG);
            /* use MPI_ERR_RANK class for PE-MPI compatibility */
            MPIR_ERR_CHKANDJUMP3((rank < 0 || rank >= size), mpi_errno, MPI_ERR_RANK,
                                 "**argrange", "**argrange %s %d %d", "rank", rank, (size - 1));
            MPIR_ERRTEST_ARGNONPOS(ndims, "ndims", mpi_errno, MPI_ERR_DIMS);

            MPIR_ERRTEST_ARGNULL(array_of_gsizes, "array_of_gsizes", mpi_errno);
            MPIR_ERRTEST_ARGNULL(array_of_distribs, "array_of_distribs", mpi_errno);
            MPIR_ERRTEST_ARGNULL(array_of_dargs, "array_of_dargs", mpi_errno);
            MPIR_ERRTEST_ARGNULL(array_of_psizes, "array_of_psizes", mpi_errno);
            if (order != MPI_ORDER_C && order != MPI_ORDER_FORTRAN) {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                 MPIR_ERR_RECOVERABLE,
                                                 __func__,
                                                 __LINE__,
                                                 MPI_ERR_ARG, "**storageorder",
                                                 "**storageorder %d", order);
                goto fn_fail;
            }

            tmp_size = 1;
            for (i = 0; mpi_errno == MPI_SUCCESS && i < ndims; i++) {
                MPIR_ERRTEST_ARGNONPOS(array_of_gsizes[i], "gsize", mpi_errno, MPI_ERR_ARG);
                MPIR_ERRTEST_ARGNONPOS(array_of_psizes[i], "psize", mpi_errno, MPI_ERR_ARG);

                if ((array_of_distribs[i] != MPI_DISTRIBUTE_NONE) &&
                    (array_of_distribs[i] != MPI_DISTRIBUTE_BLOCK) &&
                    (array_of_distribs[i] != MPI_DISTRIBUTE_CYCLIC)) {
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                     MPIR_ERR_RECOVERABLE,
                                                     __func__,
                                                     __LINE__, MPI_ERR_ARG, "**darrayunknown", 0);
                    goto fn_fail;
                }

                if ((array_of_dargs[i] != MPI_DISTRIBUTE_DFLT_DARG) && (array_of_dargs[i] <= 0)) {
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                     MPIR_ERR_RECOVERABLE,
                                                     __func__,
                                                     __LINE__,
                                                     MPI_ERR_ARG,
                                                     "**arg", "**arg %s", "array_of_dargs");
                    goto fn_fail;
                }

                if ((array_of_distribs[i] == MPI_DISTRIBUTE_NONE) && (array_of_psizes[i] != 1)) {
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                     MPIR_ERR_RECOVERABLE,
                                                     __func__,
                                                     __LINE__,
                                                     MPI_ERR_ARG,
                                                     "**darraydist",
                                                     "**darraydist %d %d", i, array_of_psizes[i]);
                    goto fn_fail;
                }

                tmp_size *= array_of_psizes[i];
            }

            MPIR_ERR_CHKANDJUMP1((tmp_size != size), mpi_errno, MPI_ERR_ARG,
                                 "**arg", "**arg %s", "array_of_psizes");

            /* TODO: GET THIS CHECK IN ALSO */

            /* check if MPI_Aint is large enough for size of global array.
             * if not, complain. */

            size_with_aint = orig_extent;
            for (i = 0; i < ndims; i++)
                size_with_aint *= array_of_gsizes[i];
            size_with_offset = orig_extent;
            for (i = 0; i < ndims; i++)
                size_with_offset *= array_of_gsizes[i];
            if (size_with_aint != size_with_offset) {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                 MPIR_ERR_FATAL,
                                                 __func__,
                                                 __LINE__,
                                                 MPI_ERR_ARG,
                                                 "**darrayoverflow",
                                                 "**darrayoverflow %L", size_with_offset);
                goto fn_fail;
            }

            /* Validate datatype_ptr */
            MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
            /* If datatype_ptr is not valid, it will be reset to null */
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
                goto fn_fail;
            /* --END ERROR HANDLING-- */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno =
        MPIR_Type_create_darray(size, rank, ndims, array_of_gsizes, array_of_distribs,
                                array_of_dargs, array_of_psizes, order, oldtype, newtype);
    if (mpi_errno)
        goto fn_fail;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_CREATE_DARRAY);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_type_create_darray",
                                 "**mpi_type_create_darray %d %d %d %p %p %p %p %d %D %p", size,
                                 rank, ndims, array_of_gsizes, array_of_distribs, array_of_dargs,
                                 array_of_psizes, order, oldtype, newtype);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
