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

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPIDI_POSIX_eager_iqueue_global_t MPIDI_POSIX_eager_iqueue_global;

static int init_transport(void *slab, int vci_src, int vci_dst)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    transport = MPIDI_POSIX_eager_iqueue_get_transport(vci_src, vci_dst);

    transport->num_cells = MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_NUM_CELLS;
    transport->size_of_cell = MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE;

    if (MPIR_CVAR_CH4_SHM_POSIX_TOPO_ENABLE) {
        int queue_types[2] = {
            MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC,
            MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPMC
        };
        mpi_errno = MPIDU_genq_shmem_pool_create(slab, MPIDI_POSIX_eager_iqueue_global.slab_size,
                                                 transport->size_of_cell, transport->num_cells,
                                                 MPIR_Process.local_size,
                                                 MPIR_Process.local_rank,
                                                 2, queue_types, &transport->cell_pool);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        int queue_type = MPIDU_GENQ_SHMEM_QUEUE_TYPE__MPSC;
        mpi_errno = MPIDU_genq_shmem_pool_create(slab, MPIDI_POSIX_eager_iqueue_global.slab_size,
                                                 transport->size_of_cell, transport->num_cells,
                                                 MPIR_Process.local_size,
                                                 MPIR_Process.local_rank,
                                                 1, &queue_type, &transport->cell_pool);
        MPIR_ERR_CHECK(mpi_errno);
    }

    transport->terminals = (void *) ((char *) slab +
                                     MPIDI_POSIX_eager_iqueue_global.terminal_offset);
    transport->my_terminal = &transport->terminals[MPIR_Process.local_rank];

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

    /* calculate needed shmem size per (vci_src, vci_dst) */
    int num_free_queue = MPIR_CVAR_CH4_SHM_POSIX_TOPO_ENABLE ? 2 : 1;
    int cell_size = MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE;
    int num_cells = MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_NUM_CELLS;
    int nprocs = MPIR_Process.local_size;

    int pool_size = MPIDU_genq_shmem_pool_size(cell_size, num_cells, nprocs, num_free_queue);
    int terminal_size = nprocs * sizeof(MPIDU_genq_shmem_queue_u);

    int slab_size = pool_size + terminal_size;

    /* Create the shared memory regions that will be used for the iqueue cells and terminals. */
    void *slab;
    mpi_errno = MPIDU_Init_shm_alloc(slab_size, (void *) &slab);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_POSIX_eager_iqueue_global.slab_size = slab_size;
    MPIDI_POSIX_eager_iqueue_global.terminal_offset = pool_size;
    MPIDI_POSIX_eager_iqueue_global.root_slab = slab;

    mpi_errno = init_transport(slab, 0, 0);
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
    return mpi_errno;
}

int MPIDI_POSIX_iqueue_set_vcis(MPIR_Comm * comm, int max_vcis)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_Assert(MPIDI_POSIX_eager_iqueue_global.all_vci_slab == NULL);

    MPIDU_Init_shm_barrier();

    int slab_size = MPIDI_POSIX_eager_iqueue_global.slab_size * max_vcis * max_vcis;
    /* Create the shared memory regions for all vcis */
    void *slab;
    mpi_errno = MPIDU_Init_shm_comm_alloc(comm, slab_size, (void *) &slab);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_POSIX_eager_iqueue_global.all_vci_slab = slab;

    for (int vci_src = 0; vci_src < max_vcis; vci_src++) {
        for (int vci_dst = 0; vci_dst < max_vcis; vci_dst++) {
            if (vci_src == 0 && vci_dst == 0) {
                continue;
            }
            void *p = (char *) slab + (vci_src * max_vcis + vci_dst) *
                MPIDI_POSIX_eager_iqueue_global.slab_size;
            mpi_errno = init_transport(p, vci_src, vci_dst);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    MPIDI_POSIX_eager_iqueue_global.max_vcis = max_vcis;

    if (comm->node_comm) {
        mpi_errno = MPIR_Barrier_impl(comm->node_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_iqueue_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (MPIDI_POSIX_eager_iqueue_global.root_slab) {
        MPIDI_POSIX_eager_iqueue_transport_t *transport;
        transport = MPIDI_POSIX_eager_iqueue_get_transport(0, 0);

        mpi_errno = MPIDU_genq_shmem_pool_destroy(transport->cell_pool);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIDU_Init_shm_free(MPIDI_POSIX_eager_iqueue_global.root_slab);
        MPIR_ERR_CHECK(mpi_errno);
        MPIDI_POSIX_eager_iqueue_global.root_slab = NULL;
    }

    if (MPIDI_POSIX_eager_iqueue_global.all_vci_slab) {
        int max_vcis = MPIDI_POSIX_eager_iqueue_global.max_vcis;
        for (int vci_src = 0; vci_src < max_vcis; vci_src++) {
            for (int vci_dst = 0; vci_dst < max_vcis; vci_dst++) {
                if (vci_src == 0 && vci_dst == 0) {
                    continue;
                }
                MPIDI_POSIX_eager_iqueue_transport_t *transport;
                transport = MPIDI_POSIX_eager_iqueue_get_transport(vci_src, vci_dst);

                mpi_errno = MPIDU_genq_shmem_pool_destroy(transport->cell_pool);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
        mpi_errno = MPIDU_Init_shm_free(MPIDI_POSIX_eager_iqueue_global.all_vci_slab);
        MPIR_ERR_CHECK(mpi_errno);
        MPIDI_POSIX_eager_iqueue_global.all_vci_slab = NULL;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
