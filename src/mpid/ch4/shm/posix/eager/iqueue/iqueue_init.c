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
      default     : 16384
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Size of each cell.

    - name        : MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_NT_MEMCPY
      category    : CH4
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Size of each cell.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPIDI_POSIX_eager_iqueue_global_t MPIDI_POSIX_eager_iqueue_global;

static int init_transport(int vci_src, int vci_dst)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    transport = MPIDI_POSIX_eager_iqueue_get_transport(vci_src, vci_dst);

    transport->num_cells = MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_NUM_CELLS;
    transport->size_of_cell = MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE;

    mpi_errno = MPIDU_genq_shmem_pool_create(transport->size_of_cell, transport->num_cells,
                                             MPIDI_POSIX_global.num_local,
                                             MPIDI_POSIX_global.my_local_rank,
                                             &transport->cell_pool);
    MPIR_ERR_CHECK(mpi_errno);

    size_t size_of_terminals;
    /* Create one terminal for each process with which we will be able to communicate. */
    size_of_terminals = (size_t) MPIDI_POSIX_global.num_local * sizeof(MPIDU_genq_shmem_queue_u);

    /* Create the shared memory regions that will be used for the iqueue cells and terminals. */
    mpi_errno = MPIDU_Init_shm_alloc(size_of_terminals, (void *) &transport->terminals);
    MPIR_ERR_CHECK(mpi_errno);

    transport->my_terminal = &transport->terminals[MPIDI_POSIX_global.my_local_rank];

    mpi_errno = MPIDU_genq_shmem_queue_init(transport->my_terminal,
                                            MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_iqueue_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* ensure max alignment for payload */
    MPIR_Assert((MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE & (MAX_ALIGNMENT - 1)) == 0);
    MPIR_Assert((sizeof(MPIDI_POSIX_eager_iqueue_cell_t) & (MAX_ALIGNMENT - 1)) == 0);

    /* Init vci 0. Communication on vci 0 is enabled afterwards. */
    MPIDI_POSIX_eager_iqueue_global.max_vcis = 1;

    mpi_errno = init_transport(0, 0);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_iqueue_post_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* gather max_vcis */
    int max_vcis = 0;
    max_vcis = 0;
    MPIDU_Init_shm_put(&MPIDI_POSIX_global.num_vcis, sizeof(int));
    MPIDU_Init_shm_barrier();
    for (int i = 0; i < MPIDI_POSIX_global.num_local; i++) {
        int num;
        MPIDU_Init_shm_get(i, sizeof(int), &num);
        if (max_vcis < num) {
            max_vcis = num;
        }
    }
    MPIDU_Init_shm_barrier();

    MPIDI_POSIX_eager_iqueue_global.max_vcis = max_vcis;

    for (int vci_src = 0; vci_src < max_vcis; vci_src++) {
        for (int vci_dst = 0; vci_dst < max_vcis; vci_dst++) {
            if (vci_src == 0 && vci_dst == 0) {
                continue;
            }
            mpi_errno = init_transport(vci_src, vci_dst);
            MPIR_ERR_CHECK(mpi_errno);

        }
    }

    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_iqueue_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    int max_vcis = MPIDI_POSIX_eager_iqueue_global.max_vcis;
    for (int vci_src = 0; vci_src < max_vcis; vci_src++) {
        for (int vci_dst = 0; vci_dst < max_vcis; vci_dst++) {
            MPIDI_POSIX_eager_iqueue_transport_t *transport;
            transport = MPIDI_POSIX_eager_iqueue_get_transport(vci_src, vci_dst);

            mpi_errno = MPIDU_Init_shm_free(transport->terminals);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIDU_genq_shmem_pool_destroy(transport->cell_pool);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
