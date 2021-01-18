/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Op_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Op_create = PMPI_Op_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Op_create  MPI_Op_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Op_create as PMPI_Op_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Op_create(MPI_User_function * user_fn, int commute, MPI_Op * op)
    __attribute__ ((weak, alias("PMPI_Op_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Op_create
#define MPI_Op_create PMPI_Op_create

#endif /* MPICH_MPI_FROM_PMPI */

/*@
  MPI_Op_create - Creates a user-defined combination function handle

Input Parameters:
+ user_fn - user defined function (function)
- commute -  true if commutative;  false otherwise. (logical)

Output Parameters:
. op - operation (handle)

  Notes on the user function:
  The calling list for the user function type is
.vb
 typedef void (MPI_User_function) (void * a,
               void * b, int * len, MPI_Datatype *);
.ve
  where the operation is 'b[i] = a[i] op b[i]', for 'i=0,...,len-1'.  A pointer
  to the datatype given to the MPI collective computation routine (i.e.,
  'MPI_Reduce', 'MPI_Allreduce', 'MPI_Scan', or 'MPI_Reduce_scatter') is also
  passed to the user-specified routine.

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_SUCCESS

.seealso: MPI_Op_free
@*/
int MPI_Op_create(MPI_User_function * user_fn, int commute, MPI_Op * op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_OP_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_OP_CREATE);

    /* ... body of routine ...  */

    MPIR_Op *op_ptr = NULL;
    mpi_errno = MPIR_Op_create_impl(user_fn, commute, &op_ptr);
    if (mpi_errno)
        goto fn_fail;
    MPIR_OBJ_PUBLISH_HANDLE(*op, op_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_OP_CREATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_op_create", "**mpi_op_create %p %d %p", user_fn, commute,
                                 op);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
