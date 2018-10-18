/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_cvar_handle_alloc */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_cvar_handle_alloc = PMPI_T_cvar_handle_alloc
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_cvar_handle_alloc  MPI_T_cvar_handle_alloc
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_cvar_handle_alloc as PMPI_T_cvar_handle_alloc
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle, MPI_T_cvar_handle * handle,
                            int *count) __attribute__ ((weak, alias("PMPI_T_cvar_handle_alloc")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_cvar_handle_alloc
#define MPI_T_cvar_handle_alloc PMPI_T_cvar_handle_alloc

/* any non-MPI functions go here, especially non-static ones */

int MPIR_T_cvar_handle_alloc_impl(int cvar_index, void *obj_handle, MPI_T_cvar_handle * handle,
                                  int *count)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_cvar_handle_t *hnd;

    MPIT_CHKPMEM_DECL(1);

    cvar_table_entry_t *cvar = (cvar_table_entry_t *) utarray_eltptr(cvar_table, cvar_index);

    /* Allocate handle memory */
    MPIT_CHKPMEM_MALLOC(hnd, MPIR_T_cvar_handle_t *, sizeof(*hnd), MPL_MEM_MPIT);
    hnd->kind = MPIR_T_CVAR_HANDLE;

    /* It is time to fix addr and count if they are unknown */
    if (cvar->get_count != NULL)
        cvar->get_count(obj_handle, count);
    else
        *count = cvar->count;

    hnd->count = *count;

    if (cvar->get_addr != NULL)
        cvar->get_addr(obj_handle, &(hnd->addr));
    else
        hnd->addr = cvar->addr;

    /* Cache other fields */
    hnd->datatype = cvar->datatype;
    hnd->scope = cvar->scope;

    *handle = hnd;

    MPIT_CHKPMEM_COMMIT();
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIT_CHKPMEM_REAP();
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_T_cvar_handle_alloc - Allocate a handle for a control variable

Input Parameters:
+ cvar_index - index of control variable for which handle is to be allocated (index)
- obj_handle - reference to a handle of the MPI object to which this variable is supposed to be bound (pointer)

Output Parameters:
+ handle - allocated handle (handle)
- count - number of elements used to represent this variable (integer)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_INDEX
.N MPI_T_ERR_INVALID_HANDLE
.N MPI_T_ERR_OUT_OF_HANDLES
@*/
int MPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle, MPI_T_cvar_handle * handle,
                            int *count)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_CVAR_HANDLE_ALLOC);
    MPIT_ERRTEST_MPIT_INITIALIZED();
    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_CVAR_HANDLE_ALLOC);

    /* Validate parameters */
    MPIT_ERRTEST_CVAR_INDEX(cvar_index);
    /* obj_handle is ignored if cvar has no binding, so no
     * TEST_ARGNULL for it */
    MPIT_ERRTEST_ARGNULL(handle);
    MPIT_ERRTEST_ARGNULL(count);

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_cvar_handle_alloc_impl(cvar_index, obj_handle, handle, count);
    MPIR_ERR_CHECK(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_CVAR_HANDLE_ALLOC);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
