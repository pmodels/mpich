/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_cvar_write */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_cvar_write = PMPI_T_cvar_write
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_cvar_write  MPI_T_cvar_write
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_cvar_write as PMPI_T_cvar_write
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf)
    __attribute__ ((weak, alias("PMPI_T_cvar_write")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_cvar_write
#define MPI_T_cvar_write PMPI_T_cvar_write

/* any non-MPI functions go here, especially non-static ones */

int MPIR_T_cvar_write_impl(MPI_T_cvar_handle handle, const void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned int i, count;
    void *addr;
    MPIR_T_cvar_handle_t *hnd = handle;

    if (hnd->scope == MPI_T_SCOPE_CONSTANT) {
        mpi_errno = MPI_T_ERR_CVAR_SET_NEVER;
        goto fn_fail;
    } else if (hnd->scope == MPI_T_SCOPE_READONLY) {
        mpi_errno = MPI_T_ERR_CVAR_SET_NOT_NOW;
        goto fn_fail;
    }

    count = hnd->count;
    addr = hnd->addr;
    MPIT_Assert(addr != NULL);

    switch (hnd->datatype) {
        case MPI_INT:
            for (i = 0; i < count; i++)
                ((int *) addr)[i] = ((int *) buf)[i];
            break;
        case MPI_UNSIGNED:
            for (i = 0; i < count; i++)
                ((unsigned *) addr)[i] = ((unsigned *) buf)[i];
            break;
        case MPI_UNSIGNED_LONG:
            for (i = 0; i < count; i++)
                ((unsigned long *) addr)[i] = ((unsigned long *) buf)[i];
            break;
        case MPI_UNSIGNED_LONG_LONG:
            for (i = 0; i < count; i++)
                ((unsigned long long *) addr)[i] = ((unsigned long long *) buf)[i];
            break;
        case MPI_DOUBLE:
            for (i = 0; i < count; i++)
                ((double *) addr)[i] = ((double *) buf)[i];
            break;
        case MPI_CHAR:
            MPIT_Assert(count > strlen(buf));   /* Make sure buf will not overflow this cvar */
            MPL_strncpy(addr, buf, count);
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
MPI_T_cvar_write - Write a control variable

Input Parameters:
+ handle - handle of the control variable to be written (handle)
- buf - initial address of storage location for variable value (choice)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_HANDLE
.N MPI_T_ERR_CVAR_SET_NOT_NOW
.N MPI_T_ERR_CVAR_SET_NEVER
@*/
int MPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_CVAR_WRITE);
    MPIT_ERRTEST_MPIT_INITIALIZED();
    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_CVAR_WRITE);

    /* Validate parameters */
    MPIT_ERRTEST_CVAR_HANDLE(handle);
    MPIT_ERRTEST_ARGNULL(buf);

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_cvar_write_impl(handle, buf);
    MPIR_ERR_CHECK(mpi_errno);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
