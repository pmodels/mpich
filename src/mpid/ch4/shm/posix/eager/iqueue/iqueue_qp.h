/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_EAGER_IQUEUE_QP_H_INCLUDED
#define POSIX_EAGER_IQUEUE_QP_H_INCLUDED

#include <mpidimpl.h>

MPIDI_POSIX_eager_iqueue_qp_t *MPIDI_POSIX_eager_iqueue_qp_init(void *send_slab, void *recv_slab,
                                                                int cell_size, int num_cells,
                                                                int peer_rank);
void MPIDI_POSIX_eager_iqueue_qp_free(MPIDI_POSIX_eager_iqueue_qp_t ** qp);

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_iqueue_rb_size(int cell_size, int num_cells)
{
    return MPL_ROUND_UP_ALIGN((sizeof(MPIDI_POSIX_eager_iqueue_rb_cntr_t) + num_cells * cell_size),
                              sysconf(_SC_PAGESIZE));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_iqueue_qp_size(int cell_size, int num_cells)
{
    return 2 * MPIDI_POSIX_eager_iqueue_rb_size(cell_size, num_cells);
}

MPL_STATIC_INLINE_PREFIX
    int MPIDI_POSIX_eager_iqueue_qp_cntr_to_idx(MPIDI_POSIX_eager_iqueue_qp_t * qp, uint64_t cntr)
{
    return cntr & (qp->num_cells - 1);
}

MPL_STATIC_INLINE_PREFIX void
*MPIDI_POSIX_eager_iqueue_qp_get_send_cell(MPIDI_POSIX_eager_iqueue_qp_t * qp)
{
    char *cell = NULL;
    if (qp->send.next_seq - qp->send.last_ack == qp->num_cells) {
        uint64_t new_ack = MPL_atomic_acquire_load_uint64(&qp->send.cntr->ack);
        if (new_ack == qp->send.last_ack) {
            return NULL;
        } else {
            for (int i = qp->send.last_ack; i < new_ack; i++) {
                MPIDI_POSIX_eager_iqueue_cell_ext_t *tmp = (MPIDI_POSIX_eager_iqueue_cell_ext_t *)
                    (qp->send.base +
                     MPIDI_POSIX_eager_iqueue_qp_cntr_to_idx(qp, i) * qp->cell_size);
                if (tmp->base.type & MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_BUF) {
                    cell = MPIDU_genq_shmem_pool_handle_to_cell(qp->cell_pool, tmp->buf_handle);
                    MPIDU_genq_shmem_pool_cell_free(qp->cell_pool, cell);
                }
            }
        }
        qp->send.last_ack = new_ack;
    }

    cell = qp->send.base + MPIDI_POSIX_eager_iqueue_qp_cntr_to_idx(qp, qp->send.next_seq)
        * qp->cell_size;

    return cell;
}

MPL_STATIC_INLINE_PREFIX void
*MPIDI_POSIX_eager_iqueue_qp_get_recv_cell(MPIDI_POSIX_eager_iqueue_qp_t * qp)
{
    char *cell = NULL;
    if (qp->recv.last_ack == qp->recv.next_seq) {
        uint64_t new_seq = MPL_atomic_acquire_load_uint64(&qp->recv.cntr->seq);
        if (new_seq == qp->recv.next_seq) {
            return NULL;
        }
        qp->recv.next_seq = new_seq;
    }

    cell = qp->recv.base + MPIDI_POSIX_eager_iqueue_qp_cntr_to_idx(qp, qp->recv.last_ack)
        * qp->cell_size;

    return cell;
}

MPL_STATIC_INLINE_PREFIX void
MPIDI_POSIX_eager_iqueue_qp_send_commit(MPIDI_POSIX_eager_iqueue_qp_t * qp)
{
    qp->send.next_seq++;
    MPL_atomic_release_store_uint64(&qp->send.cntr->seq, qp->send.next_seq);
}

MPL_STATIC_INLINE_PREFIX void
MPIDI_POSIX_eager_iqueue_qp_recv_complete(MPIDI_POSIX_eager_iqueue_qp_t * qp)
{
    qp->recv.last_ack++;
    MPL_atomic_release_store_uint64(&qp->recv.cntr->ack, qp->recv.last_ack);
}

#endif /* POSIX_EAGER_IQUEUE_QP_H_INCLUDED */
