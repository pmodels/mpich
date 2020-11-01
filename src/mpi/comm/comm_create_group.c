/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_create_group */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_create_group = PMPI_Comm_create_group
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_create_group  MPI_Comm_create_group
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_create_group as PMPI_Comm_create_group
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm * newcomm)
    __attribute__ ((weak, alias("PMPI_Comm_create_group")));
#endif
/* -- End Profiling Symbol Block */


/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_create_group
#define MPI_Comm_create_group PMPI_Comm_create_group
#endif /* !defined(MPICH_MPI_FROM_PMPI) */

/*@

MPI_Comm_create_group - Creates a new communicator

Input Parameters:
+ comm - communicator (handle)
. group - group, which is a subset of the group of 'comm'  (handle)
- tag - safe tag unused by other communication

Output Parameters:
. newcomm - new communicator (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_GROUP

.seealso: MPI_Comm_free
@*/
int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm * newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPIR_Group *group_ptr;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_CREATE_GROUP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_CREATE_GROUP);

    /* Validate parameters, and convert MPI object handles to object pointers */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;

        MPIR_Comm_get_ptr(comm, comm_ptr);

        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno)
                goto fn_fail;
            /* If comm_ptr is not valid, it will be reset to null */
            MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);

            /* only test for MPI_GROUP_NULL after attempting to convert the comm
             * so that any errhandlers on comm will (correctly) be invoked */
            MPIR_ERRTEST_GROUP(group, mpi_errno);
            MPIR_ERRTEST_COMM_TAG(tag, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;

        MPIR_Group_get_ptr(group, group_ptr);

        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Check the group ptr */
            MPIR_Group_valid_ptr(group_ptr, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
            MPIR_ERRTEST_ARGNULL(newcomm, "newcomm", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#else
    {
        MPIR_Comm_get_ptr(comm, comm_ptr);
        MPIR_Group_get_ptr(group, group_ptr);
    }
#endif

    /* ... body of routine ...  */
    mpi_errno = MPIR_Comm_create_group(comm_ptr, group_ptr, tag, &newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (newcomm_ptr)
        MPIR_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    else
        *newcomm = MPI_COMM_NULL;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_CREATE_GROUP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**mpi_comm_create_group",
                                 "**mpi_comm_create_group %C %G %d %p", comm, group, tag, newcomm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
