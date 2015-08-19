/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_hindexed_block */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_hindexed_block = PMPI_Type_create_hindexed_block
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_hindexed_block  MPI_Type_create_hindexed_block
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_hindexed_block as PMPI_Type_create_hindexed_block
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_create_hindexed_block(int count, int blocklength,
                                   const MPI_Aint array_of_displacements[],
                                   MPI_Datatype oldtype, MPI_Datatype *newtype) __attribute__((weak,alias("PMPI_Type_create_hindexed_block")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_create_hindexed_block
#define MPI_Type_create_hindexed_block PMPI_Type_create_hindexed_block

#undef FUNCNAME
#define FUNCNAME MPIR_Type_create_hindexed_block_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Type_create_hindexed_block_impl(int count, int blocklength,
                                         const MPI_Aint array_of_displacements[],
                                         MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPID_Datatype *new_dtp;
    int ints[2];

    mpi_errno = MPID_Type_blockindexed(count, blocklength, array_of_displacements, 1,
                                       oldtype, &new_handle);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    ints[0] = count;
    ints[1] = blocklength;

    MPID_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
                                           MPI_COMBINER_HINDEXED_BLOCK,
                                           2,     /* ints */
                                           count, /* aints */
                                           1,     /* types */
                                           ints,
                                           array_of_displacements,
                                           &oldtype);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPID_OBJ_PUBLISH_HANDLE(*newtype, new_handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_hindexed_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Type_create_hindexed_block - Create an hindexed
     datatype with constant-sized blocks

Input Parameters:
+ count - length of array of displacements (integer)
. blocklength - size of block (integer)
. array_of_displacements - array of displacements (array of integer)
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
int MPI_Type_create_hindexed_block(int count,
                                    int blocklength,
                                    const MPI_Aint array_of_displacements[],
                                    MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_HINDEXED_BLOCK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_HINDEXED_BLOCK);

    /* Validate parameters and objects */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Datatype *datatype_ptr = NULL;

            MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_ARGNEG(blocklength, "blocklen", mpi_errno);
            if (count > 0) {
                MPIR_ERRTEST_ARGNULL(array_of_displacements, "indices", mpi_errno);
            }
            MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);

            if (HANDLE_GET_KIND(oldtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(oldtype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
            }

            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno =
        MPIR_Type_create_hindexed_block_impl(count, blocklength, array_of_displacements,
                                             oldtype, newtype);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_HINDEXED_BLOCK);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                 MPI_ERR_OTHER, "**mpi_type_create_hindexed_block",
                                 "**mpi_type_create_hindexed_block %d %d %p %D %p", count,
                                 blocklength, array_of_displacements, oldtype, newtype);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
