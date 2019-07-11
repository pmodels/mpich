/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidu_shm_impl.h"

static int sense;
static int barrier_init = 0;

int MPIDU_shm_barrier_init(MPIDU_shm_barrier_t * barrier_region,
                           MPIDU_shm_barrier_t ** barrier, int init_values)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SHM_BARRIER_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SHM_BARRIER_INIT);

    *barrier = barrier_region;
    if (init_values) {
        MPL_atomic_relaxed_store_int(&(*barrier)->val, 0);
        MPL_atomic_relaxed_store_int(&(*barrier)->wait, 0);
        MPL_atomic_write_barrier();
    }
    sense = 0;
    barrier_init = 1;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_SHM_BARRIER_INIT);

    return MPI_SUCCESS;
}

/* FIXME: this is not a scalable algorithm because everyone is polling on the same cacheline */
int MPIDU_shm_barrier(MPIDU_shm_barrier_t * barrier, int num_local)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_SHM_BARRIER);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_SHM_BARRIER);

    if (num_local == 1)
        goto fn_exit;

    MPIR_ERR_CHKINTERNAL(!barrier_init, mpi_errno, "barrier not initialized");

    if (MPL_atomic_fetch_add_int(&barrier->val, 1) == num_local - 1) {
        MPL_atomic_relaxed_store_int(&barrier->val, 0);
        MPL_atomic_relaxed_store_int(&barrier->wait, 1 - sense);
        MPL_atomic_write_barrier();
    } else {
        /* wait */
        while (MPL_atomic_relaxed_load_int(&barrier->wait) == sense)
            MPL_sched_yield();  /* skip */
    }
    sense = 1 - sense;

  fn_fail:
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_SHM_BARRIER);
    return mpi_errno;
}
