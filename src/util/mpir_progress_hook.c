/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpl.h"

#define MAX_PROGRESS_HOOKS 4

typedef int (*progress_func_ptr_t) (int *made_progress);
typedef struct progress_hook_slot {
    progress_func_ptr_t func_ptr;
    MPL_atomic_int_t active;
} progress_hook_slot_t;

static int registered_progress_hooks = 0;
static progress_hook_slot_t progress_hooks[MAX_PROGRESS_HOOKS];

int MPIR_Progress_hook_exec_all(int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 0; i < registered_progress_hooks; i++) {
        int is_active = MPL_atomic_acquire_load_int(&progress_hooks[i].active);
        if (is_active == TRUE) {
            MPIR_Assert(progress_hooks[i].func_ptr != NULL);
            int tmp_progress = 0;
            mpi_errno = progress_hooks[i].func_ptr(&tmp_progress);
            MPIR_ERR_CHECK(mpi_errno);

            *made_progress |= tmp_progress;
        }
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Progress_hook_register(int (*progress_fn) (int *), int *id)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    for (i = 0; i < MAX_PROGRESS_HOOKS; i++) {
        if (progress_hooks[i].func_ptr == NULL) {
            progress_hooks[i].func_ptr = progress_fn;
            MPL_atomic_relaxed_store_int(&progress_hooks[i].active, FALSE);
            break;
        }
    }

    if (i >= MAX_PROGRESS_HOOKS)
        goto fn_fail;

    registered_progress_hooks++;

    (*id) = i;

  fn_exit:
    return mpi_errno;
  fn_fail:
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                     "MPID_Progress_register", __LINE__,
                                     MPI_ERR_INTERN, "**progresshookstoomany", 0);
    goto fn_exit;
}

int MPIR_Progress_hook_deregister(int id)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);
    MPIR_Assert(progress_hooks[id].func_ptr != NULL);
    progress_hooks[id].func_ptr = NULL;
    MPL_atomic_release_store_int(&progress_hooks[id].active, FALSE);

    registered_progress_hooks--;

    return mpi_errno;
}

/* The below functions assume that each progress hook is protected by
 * a mutex, which is also shared with other functions that modify the
 * global state of these hooks.  If we think of each hook as making
 * progress on a class, then we assume that the public functions to
 * that class are thread safe.
 *
 * In the below code, we only maintain atomicity for reading whether
 * the "active" field is set or not.  We intentionally avoid using a
 * critical section for performance reasons.  It is possible that a
 * different thread deactivates a progress hook after we check if it
 * is active, but before we execute the function pointer.  In that
 * case, we simply do an extra poll of the progress hook, which does
 * not affect correctness.  Note that the func_ptr itself is not
 * free'd till finalize. */

int MPIR_Progress_hook_activate(int id)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);

    MPL_atomic_release_store_int(&progress_hooks[id].active, TRUE);
    MPIR_Assert(progress_hooks[id].func_ptr != NULL);

    return mpi_errno;
}

int MPIR_Progress_hook_deactivate(int id)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(id >= 0);
    MPIR_Assert(id < MAX_PROGRESS_HOOKS);

    MPL_atomic_release_store_int(&progress_hooks[id].active, FALSE);
    MPIR_Assert(progress_hooks[id].func_ptr != NULL);

    return mpi_errno;
}
