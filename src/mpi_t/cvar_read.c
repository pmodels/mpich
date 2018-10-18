/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_cvar_read */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_cvar_read = PMPI_T_cvar_read
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_cvar_read  MPI_T_cvar_read
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_cvar_read as PMPI_T_cvar_read
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf)
    __attribute__ ((weak, alias("PMPI_T_cvar_read")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_cvar_read
#define MPI_T_cvar_read PMPI_T_cvar_read

/* any non-MPI functions go here, especially non-static ones */

int MPIR_T_cvar_read_impl(MPI_T_cvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    int i, count;
    void *addr;
    MPIR_T_cvar_handle_t *hnd = handle;

    count = hnd->count;
    addr = hnd->addr;
    MPIT_Assert(addr != NULL);

    switch (hnd->datatype) {
        case MPI_INT:
            for (i = 0; i < count; i++)
                ((int *) buf)[i] = ((int *) addr)[i];
            break;
        case MPI_UNSIGNED:
            for (i = 0; i < count; i++)
                ((unsigned *) buf)[i] = ((unsigned *) addr)[i];
            break;
        case MPI_UNSIGNED_LONG:
            for (i = 0; i < count; i++)
                ((unsigned long *) buf)[i] = ((unsigned long *) addr)[i];
            break;
        case MPI_UNSIGNED_LONG_LONG:
            for (i = 0; i < count; i++)
                ((unsigned long long *) buf)[i] = ((unsigned long long *) addr)[i];
            break;
        case MPI_DOUBLE:
            for (i = 0; i < count; i++)
                ((double *) buf)[i] = ((double *) addr)[i];
            break;
        case MPI_CHAR:
            MPL_strncpy(buf, addr, count);
            break;
        default:
            mpi_errno = MPI_T_ERR_INVALID;
            goto fn_fail;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_T_cvar_read - Read the value of a control variable

Input Parameters:
. handle - handle to the control variable to be read (handle)

Output Parameters:
. buf - initial address of storage location for variable value

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_HANDLE
@*/
int MPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_CVAR_READ);
    MPIT_ERRTEST_MPIT_INITIALIZED();
    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_CVAR_READ);

    /* Validate parameters */
    MPIT_ERRTEST_CVAR_HANDLE(handle);
    MPIT_ERRTEST_ARGNULL(buf);

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_cvar_read_impl(handle, buf);
    MPIR_ERR_CHECK(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_CVAR_READ);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
