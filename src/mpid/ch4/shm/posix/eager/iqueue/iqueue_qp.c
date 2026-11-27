/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "iqueue_noinline.h"

MPIDI_POSIX_eager_iqueue_qp_t *MPIDI_POSIX_eager_iqueue_qp_init(void *send_slab, void *recv_slab,
                                                                int cell_size, int num_cells,
                                                                int peer_rank)
{
    MPIDI_POSIX_eager_iqueue_qp_t *qp = NULL;

    MPIR_Assert(send_slab);
    MPIR_Assert(recv_slab);
    MPIR_Assert(MPL_CHECK_ALIGN((uintptr_t) send_slab, sysconf(_SC_PAGESIZE)));
    MPIR_Assert(MPL_CHECK_ALIGN((uintptr_t) recv_slab, sysconf(_SC_PAGESIZE)));
    MPIR_Assert(MPL_CHECK_ALIGN(cell_size, MPL_CACHELINE_SIZE));
    MPIR_Assert(MPL_is_pof2(num_cells));
    MPIR_Assert(peer_rank != MPIR_Process.local_rank);

    qp = (MPIDI_POSIX_eager_iqueue_qp_t *) MPL_malloc(sizeof(MPIDI_POSIX_eager_iqueue_qp_t),
                                                      MPL_MEM_SHM);
    if (qp == NULL) {
        goto fn_fail;
    }

    qp->cell_size = cell_size;
    qp->num_cells = num_cells;

    qp->send.cntr = (MPIDI_POSIX_eager_iqueue_rb_cntr_t *) send_slab;
    qp->send.base = send_slab + sizeof(MPIDI_POSIX_eager_iqueue_rb_cntr_t);
    qp->send.next_seq = 0;
    qp->send.last_ack = 0;

    qp->recv.cntr = (MPIDI_POSIX_eager_iqueue_rb_cntr_t *) recv_slab;
    qp->recv.base = recv_slab + sizeof(MPIDI_POSIX_eager_iqueue_rb_cntr_t);
    qp->recv.next_seq = 0;
    qp->recv.last_ack = 0;

    qp->peer_rank = peer_rank;

    /* init counter will do first-touch NUMA affinity for the recv slab */
    memset(recv_slab, 0, MPIDI_POSIX_eager_iqueue_rb_size(cell_size, num_cells));

  fn_exit:
    return qp;
  fn_fail:
    if (qp) {
        MPL_free(qp);
        qp = NULL;
    }
    goto fn_exit;
}

void MPIDI_POSIX_eager_iqueue_qp_free(MPIDI_POSIX_eager_iqueue_qp_t ** qp)
{
    if (*qp == NULL) {
        return;
    }

    MPL_free(*qp);
    qp = NULL;
}
