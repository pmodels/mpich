/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

/* circular FIFO queue functions */

#define AM_stack MPIDI_UCX_global.am_ready_queue
#define AM_head  MPIDI_UCX_global.am_ready_head
#define AM_tail  MPIDI_UCX_global.am_ready_tail

MPL_STATIC_INLINE_PREFIX bool am_queue_is_empty(void)
{
    return (AM_head == AM_tail);
}

MPL_STATIC_INLINE_PREFIX void am_queue_push(int idx)
{
    AM_stack[AM_tail] = idx;
    AM_tail = (AM_tail + 1) & MPIDI_UCX_AM_QUEUE_MASK;
}

MPL_STATIC_INLINE_PREFIX int am_queue_pop(void)
{
    int idx = AM_stack[AM_head];
    AM_head = (AM_head + 1) & MPIDI_UCX_AM_QUEUE_MASK;
    return idx;
}

/* exposed functions */

int MPIDI_UCX_am_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* Make sure the circular queue will work */
    MPIR_Assert(MPIDI_UCX_AM_QUEUE_SIZE > MPIDI_UCX_AM_BUF_COUNT);

    return mpi_errno;
}

int MPIDI_UCX_am_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

void MPIDI_UCX_am_progress(int vni)
{
}
