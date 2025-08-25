/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "posix_types.h"

int MPIDI_POSIX_comm_set_vcis(MPIR_Comm * comm, int num_vcis)
{
    int mpi_errno = MPI_SUCCESS;

    /* We only set up vcis once */
    MPIR_Assert(MPIDI_POSIX_global.num_vcis == 1);

    MPIR_Comm *node_comm = MPIR_Comm_get_node_comm(comm);
    MPIR_Assert(node_comm);

    int max_vcis;
    mpi_errno = MPIR_Allreduce_impl(&num_vcis, &max_vcis, 1, MPIR_INT_INTERNAL, MPI_MAX,
                                    node_comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    if (max_vcis > 1) {
        for (int i = 1; i < max_vcis; i++) {
            mpi_errno = MPIDI_POSIX_init_vci(i);
            MPIR_ERR_CHECK(mpi_errno);
        }

        void *slab;
        int slab_size = MPIDI_POSIX_eager_shm_vci_size(MPIR_Process.local_size, max_vcis);
#ifdef MPL_HAVE_INITSHM
        slab = MPL_initshm_open(MPIDI_POSIX_global.shm_vci_name, slab_size, NULL);
        MPIR_ERR_CHKANDJUMP(!slab, mpi_errno, MPI_ERR_OTHER, "**nomem");

        mpi_errno = MPIR_Barrier_impl(node_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
#else
        mpi_errno = MPIDU_Init_shm_comm_alloc(comm, slab_size, (void *) &slab);
        MPIR_ERR_CHECK(mpi_errno);
#endif
        MPIDI_POSIX_global.shm_vci_slab = slab;

        mpi_errno = MPIDI_POSIX_eager_set_vcis(MPIDI_POSIX_global.shm_vci_slab, comm, max_vcis);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Barrier_impl(node_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIDI_POSIX_global.num_vcis = max_vcis;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
