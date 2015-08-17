/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpl_utlist.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_set_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_set_info = PMPI_Comm_set_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_set_info  MPI_Comm_set_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_set_info as PMPI_Comm_set_info
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info) __attribute__((weak,alias("PMPI_Comm_set_info")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_set_info
#define MPI_Comm_set_info PMPI_Comm_set_info

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_set_info_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_set_info_impl(MPID_Comm * comm_ptr, MPID_Info * info_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *curr_info = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_SET_INFO_IMPL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_SET_INFO_IMPL);

    mpi_errno = MPIR_Comm_apply_hints(comm_ptr, info_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    if (comm_ptr->info == NULL) {
        /* Always have at least a blank info hint. */
        mpi_errno = MPIU_Info_alloc(&(comm_ptr->info));
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
    }

    /* MPIR_Info_set_impl will do an O(n) search to prevent duplicate keys, so
     * this _FOREACH loop will cost O(m*n) time, where "m" is the number of keys
     * in info_ptr and "n" is the number of keys in comm_ptr->info. */
    MPL_LL_FOREACH(info_ptr, curr_info) {
        /* Have we hit the default, empty info hint? */
        if (curr_info->key == NULL) continue;

        mpi_errno = MPIR_Info_set_impl(comm_ptr->info, curr_info->key, curr_info->value);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_SET_INFO_IMPL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Comm_set_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Comm_set_info - Set new values for the hints of the
   communicator associated with comm.  The call is collective on the
   group of comm.  The info object may be different on each process,
   but any info entries that an implementation requires to be the same
   on all processes must appear with the same value in each process''
   info object.

Input Parameters:
+ comm - communicator object (handle)
- info - info argument (handle)

.N ThreadSafe
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_INFO
.N MPI_ERR_OTHER
@*/
int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SET_INFO);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SET_INFO);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INFO(info, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr(comm, comm_ptr);
    MPID_Info_get_ptr(info, info_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate pointers */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Comm_set_info_impl(comm_ptr, info_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_INFO);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                 MPI_ERR_OTHER, "**mpi_comm_set_info",
                                 "**mpi_comm_set_info %W %p", comm, info);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
