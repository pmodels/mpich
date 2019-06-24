/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_EAGER_IQUEUE_INIT_H_INCLUDED
#define POSIX_EAGER_IQUEUE_INIT_H_INCLUDED

#include "iqueue_types.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_NUM_CELLS
      category    : CH4
      type        : int
      default     : 32
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The number of cells used for the depth of the iqueue.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    size_t size_of_terminals;
    size_t size_of_cells;
    size_t size_of_shared_memory;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_EAGER_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_EAGER_INIT);

    /* Get the internal data structure to describe the iqueues */
    transport = MPIDI_POSIX_eager_iqueue_get_transport();

    transport->num_cells = MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_NUM_CELLS;
    transport->size_of_cell = MPIDI_POSIX_EAGER_IQUEUE_DEFAULT_CELL_SIZE;

    /* Create one terminal for each process with which we will be able to communicate. */
    size_of_terminals =
        (size_t) MPIDI_POSIX_global.num_local * sizeof(MPIDI_POSIX_eager_iqueue_terminal_t);

    /* Behind each terminal is a series of cells. We have `num_cells` per queue/terminal per
     * communicating process. */
    size_of_cells = (size_t) MPIDI_POSIX_global.num_local * (size_t) transport->num_cells
        * (size_t) transport->size_of_cell;

    size_of_shared_memory = size_of_terminals + size_of_cells;

    /* Create the shared memory regions that will be used for the iqueue cells and terminals. */
    mpi_errno = MPIDU_shm_seg_alloc(size_of_shared_memory,
                                    &transport->pointer_to_shared_memory, MPL_MEM_SHM);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIDU_shm_seg_commit(&transport->memory, &MPIDI_POSIX_global.barrier,
                                     MPIDI_POSIX_global.num_local, MPIDI_POSIX_global.my_local_rank,
                                     MPIDI_POSIX_global.local_rank_0, rank, MPL_MEM_SHM);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

    /* Set up the appropriate pointers for each of the parts of the queues. */
    transport->terminals =
        (MPIDI_POSIX_eager_iqueue_terminal_t *) ((char *) transport->pointer_to_shared_memory);

    transport->cells = (char *) transport->pointer_to_shared_memory
        + size_of_terminals +
        (size_t) MPIDI_POSIX_global.my_local_rank * (size_t) transport->num_cells *
        (size_t) transport->size_of_cell;

    transport->terminals[MPIDI_POSIX_global.my_local_rank].head = 0;

    /* Do the pointer arithmetic and initialize each of the cell data structures. */
    for (i = 0; i < transport->num_cells; i++) {
        MPIDI_POSIX_eager_iqueue_cell_t *cell = (MPIDI_POSIX_eager_iqueue_cell_t *)
            ((char *) transport->cells + (size_t) transport->size_of_cell * i);
        cell->type = MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_NULL;
        cell->from = MPIDI_POSIX_global.my_local_rank;
        cell->next = NULL;
        cell->prev = 0;
        cell->payload_size = 0;
    }

    /* Run local procs barrier */
    mpi_errno = MPIDU_shm_barrier(MPIDI_POSIX_global.barrier, MPIDI_POSIX_global.num_local);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_EAGER_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_finalize()
{
    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_EAGER_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_EAGER_FINALIZE);

    transport = MPIDI_POSIX_eager_iqueue_get_transport();

    /* Make sure all local processes are ready to destroy the segment together */
    mpi_errno = MPIDU_shm_barrier(MPIDI_POSIX_global.barrier, MPIDI_POSIX_global.num_local);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIDU_shm_seg_destroy(&transport->memory, MPIDI_POSIX_global.num_local);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_EAGER_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* POSIX_EAGER_IQUEUE_INIT_H_INCLUDED */
