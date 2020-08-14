/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "iqueue_noinline.h"
#include "mpidu_genq.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_NUM_CELLS
      category    : CH4
      type        : int
      default     : 64
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The number of cells used for the depth of the iqueue.

    - name        : MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE
      category    : CH4
      type        : int
      default     : 69632
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Size of each cell. 4KB * 17 is default to avoid a cache aliasing issue.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIDI_POSIX_iqueue_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    size_t size_of_terminals;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_IQUEUE_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_IQUEUE_INIT);

    /* Get the internal data structure to describe the iqueues */
    transport = MPIDI_POSIX_eager_iqueue_get_transport();

    transport->num_cells = MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_NUM_CELLS;
    transport->size_of_cell = MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE;

    /* ensure max alignment for payload */
    MPIR_Assert((MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE & (MAX_ALIGNMENT - 1)) == 0);
    MPIR_Assert((sizeof(MPIDI_POSIX_eager_iqueue_cell_t) & (MAX_ALIGNMENT - 1)) == 0);

    mpi_errno = MPIDU_genq_shmem_pool_create_unsafe(transport->size_of_cell, transport->num_cells,
                                                    MPIDI_POSIX_global.num_local,
                                                    MPIDI_POSIX_global.my_local_rank,
                                                    &transport->cell_pool);
    MPIR_ERR_CHECK(mpi_errno);

    /* Create one terminal for each process with which we will be able to communicate. */
    size_of_terminals = (size_t) MPIDI_POSIX_global.num_local * sizeof(MPIDU_genq_shmem_queue_u);

    /* Create the shared memory regions that will be used for the iqueue cells and terminals. */
    mpi_errno = MPIDU_Init_shm_alloc(size_of_terminals, (void *) &transport->terminals);
    MPIR_ERR_CHECK(mpi_errno);

    transport->my_terminal = &transport->terminals[MPIDI_POSIX_global.my_local_rank];

    mpi_errno = MPIDU_genq_shmem_queue_init(transport->my_terminal,
                                            MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC);
    MPIR_ERR_CHECK(mpi_errno);

    /* Run local procs barrier */
    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_IQUEUE_INIT);
    return mpi_errno;
  fn_fail:
    MPIDU_Init_shm_free(transport->terminals);
    MPIDU_genq_shmem_pool_destroy_unsafe(transport->cell_pool);
    goto fn_exit;
}

int MPIDI_POSIX_iqueue_finalize(void)
{
    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_IQUEUE_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_IQUEUE_FINALIZE);

    transport = MPIDI_POSIX_eager_iqueue_get_transport();

    mpi_errno = MPIDU_Init_shm_free(transport->terminals);
    mpi_errno = MPIDU_genq_shmem_pool_destroy_unsafe(transport->cell_pool);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_IQUEUE_FINALIZE);
    return mpi_errno;
}
