/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_IQUEUE_SEND_H_INCLUDED
#define POSIX_EAGER_IQUEUE_SEND_H_INCLUDED

#include "iqueue_impl.h"
#include "mpidu_genq.h"

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_eager_payload_limit(void)
{
    return MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE - sizeof(MPIDI_POSIX_eager_iqueue_cell_t);
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_eager_buf_limit(void)
{
    return MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE;
}

/* This function attempts to allocate a cell and return the address and size of the payload portion
 * of the cell.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_get_buf(void **eager_buf, size_t * eager_buf_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_eager_iqueue_cell_t *cell = NULL;
    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    void *payload = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_SEND_SHORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_SEND_SHORT);

    /* Get the transport object that holds all of the global variables. */
    transport = MPIDI_POSIX_eager_iqueue_get_transport();

    /* Try to get a new cell to hold the message */
    mpi_errno = MPIDU_genq_shmem_pool_cell_alloc(transport->cell_pool, (void **) &cell);
    MPIR_ERR_CHECK(mpi_errno);
    if (!cell) {
        *eager_buf = NULL;
        *eager_buf_sz = 0;
        goto fn_exit;
    }

    /* Get the memory allocated to be used for the message transportation. */
    payload = MPIDI_POSIX_EAGER_IQUEUE_CELL_GET_PAYLOAD(cell);

    *eager_buf = payload;
    *eager_buf_sz = MPIDI_POSIX_EAGER_IQUEUE_CELL_CAPACITY(transport);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_SEND_SHORT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This function attempts to allocate a cell and return the address and size of the payload portion
 * of the cell.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_send(void *eager_buf, size_t data_sz, int grank,
                                                    MPIDI_POSIX_am_header_t * msg_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    MPIDI_POSIX_eager_iqueue_cell_t *cell = MPIDI_POSIX_EAGER_IQUEUE_PAYLOAD_GET_CELL(eager_buf);
    MPIDU_genq_shmem_queue_t terminal;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_SEND);

    /* Get the transport object that holds all of the global variables. */
    transport = MPIDI_POSIX_eager_iqueue_get_transport();

    /* Find the correct queue for this rank pair. */
    terminal = &transport->terminals[MPIDI_POSIX_global.local_ranks[grank]];

    cell->from = MPIDI_POSIX_global.my_local_rank;
    mpi_errno = MPIR_Typerep_copy(&(cell->am_header), msg_hdr, sizeof(MPIDI_POSIX_am_header_t));

    mpi_errno = MPIDU_genq_shmem_queue_enqueue(terminal, (void *) cell);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* POSIX_EAGER_IQUEUE_SEND_H_INCLUDED */
