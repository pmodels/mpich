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
    return MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE - sizeof(MPIDI_POSIX_eager_iqueue_cell_t)
        - MAX_ALIGNMENT;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_eager_buf_limit(void)
{
    return MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE;
}

MPL_STATIC_INLINE_PREFIX int
MPIDI_POSIX_eager_send_qp(int grank, MPIDI_POSIX_am_header_t * msg_hdr, const void *am_hdr,
                          MPI_Aint am_hdr_sz, const void *buf, MPI_Aint count,
                          MPI_Datatype datatype, MPI_Aint offset, int src_vci, int dst_vci,
                          MPI_Aint * bytes_sent)
{
    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    MPIDI_POSIX_eager_iqueue_cell_t *cell, *qcell;
    MPIDU_genq_shmem_queue_t terminal;
    size_t capacity, available;
    char *payload;
    int ret = MPIDI_POSIX_OK;
    MPI_Aint packed_size = 0;
    bool need_iov_buf = false;

    MPIR_FUNC_ENTER;

    MPI_Aint data_sz;
    MPIDI_Datatype_check_size(datatype, count, data_sz);

    /* Get the transport object that holds all of the global variables. */
    transport = MPIDI_POSIX_eager_iqueue_get_transport(src_vci, dst_vci);

    int dst_local_rank = MPIDI_SHM_global.local_ranks[grank];
    bool is_topo_local =
        (MPIDI_POSIX_global.local_rank_dist[dst_local_rank] == MPIDI_POSIX_DIST__LOCAL);

    MPIDI_POSIX_eager_iqueue_qp_t *qp = transport->qp[dst_local_rank];

    need_iov_buf = (data_sz - offset + MPL_ROUND_UP_ALIGN(am_hdr_sz, MAX_ALIGNMENT))
        > (qp->cell_size - sizeof(MPIDI_POSIX_eager_iqueue_cell_t));
    /* Try to get a new cell to hold the message. If a cell wasn't available,
     * let the caller know that we weren't able to send the message immediately.
     */
    qcell = (MPIDI_POSIX_eager_iqueue_cell_t *) MPIDI_POSIX_eager_iqueue_qp_get_send_cell(qp);
    if (qcell == NULL) {
        ret = MPIDI_POSIX_NOK;
        goto fn_exit;
    }

    if (need_iov_buf) {
        /* get handle of a shm buffer */
        if (is_topo_local) {
            MPIDU_genq_shmem_pool_cell_alloc(transport->cell_pool, (void **) &cell,
                                             MPIR_Process.local_rank, 0 /* intra NUMA */ , buf);
        } else {
            MPIDU_genq_shmem_pool_cell_alloc(transport->cell_pool, (void **) &cell, dst_local_rank,
                                             1 /* inter NUMA */ , buf);
        }
        if (cell == NULL) {
            ret = MPIDI_POSIX_NOK;
            goto fn_exit;
        }
        uint64_t handle = MPIDU_genq_shmem_pool_cell_to_handle(cell);

        qcell->type = MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_BUF;
        ((MPIDI_POSIX_eager_iqueue_cell_ext_t *) qcell)->buf_handle = handle;

        /* map handle to cell */
        capacity = transport->size_of_cell;
        payload = (char *) cell;
    } else {
        qcell->type = 0;

        cell = qcell;
        capacity = qp->cell_size - sizeof(MPIDI_POSIX_eager_iqueue_cell_t);
        payload = MPIDI_POSIX_EAGER_IQUEUE_CELL_PAYLOAD(cell);
    }

    available = capacity;

    qcell->from = MPIR_Process.local_rank;

    /* If this is the beginning of the message, mark it as the head. Otherwise it will be the
     * tail. */
    qcell->payload_size = 0;
    if (am_hdr) {
        MPI_Aint resized_am_hdr_sz = MPL_ROUND_UP_ALIGN(am_hdr_sz, MAX_ALIGNMENT);
        qcell->am_header = *msg_hdr;
        qcell->type |= MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_HDR;
        /* send am_hdr if this is the first segment */
        if (is_topo_local) {
            MPIR_Typerep_copy(payload, am_hdr, am_hdr_sz, MPIR_TYPEREP_FLAG_NONE);
        } else {
            MPIR_Typerep_copy(payload, am_hdr, am_hdr_sz, MPIR_TYPEREP_FLAG_STREAM);
        }
        /* make sure the data region starts at the boundary of MAX_ALIGNMENT */
        payload = payload + resized_am_hdr_sz;
        qcell->payload_size += resized_am_hdr_sz;
        qcell->am_header.am_hdr_sz = resized_am_hdr_sz;
        available -= qcell->am_header.am_hdr_sz;
    } else {
        qcell->type |= MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_DATA;
    }

    /* We want to skip packing of send buffer if there is no data to be sent . buf == NULL is
     * not a correct check here because derived datatype can use absolute address for displacement
     * which requires buffer address passed as MPI_BOTTOM which is usually NULL. count == 0 is also
     * not reliable because the derived datatype could have zero block size which contains no
     * data. */
    if (bytes_sent) {
        if (is_topo_local) {
            MPIR_Typerep_pack(buf, count, datatype, offset, payload, available, &packed_size,
                              MPIR_TYPEREP_FLAG_NONE);
        } else {
            MPIR_Typerep_pack(buf, count, datatype, offset, payload, available, &packed_size,
                              MPIR_TYPEREP_FLAG_STREAM);
        }
        qcell->payload_size += packed_size;
        *bytes_sent = packed_size;
    }

    MPIDI_POSIX_eager_iqueue_qp_send_commit(qp);

  fn_exit:
    MPIR_FUNC_EXIT;
    return ret;
}

/* This function attempts to send the next chunk of a message via the queue. If no cells are
 * available, this function will return and the caller is expected to queue the message for later
 * and retry.
 *
 * grank   - The global rank (the rank in MPI_COMM_WORLD) of the receiving process.
 * msg_hdr - The header of the message to be sent. This can be NULL if there is no header to be sent
 *           (such as if the header was sent in a previous chunk, am_hdr will be NULL too in this
 *           case.
 * am_hdr, am_hdr_sz    - am header this could be NULL if not sending the first chunk
 * buf, count, datatype - Data buffer and signature for the send buffer. They could be NULL in the
 *                        case of a header-only message
 * offset               - current offset.
 * bytes_sent           - output variable for how much data actually been sent, pass NULL if no data
 *                        need to be send
 */
MPL_STATIC_INLINE_PREFIX int
MPIDI_POSIX_eager_send(int grank, MPIDI_POSIX_am_header_t * msg_hdr, const void *am_hdr,
                       MPI_Aint am_hdr_sz, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                       MPI_Aint offset, int src_vci, int dst_vci, MPI_Aint * bytes_sent)
{
    MPIDI_POSIX_eager_iqueue_transport_t *transport;
    MPIDI_POSIX_eager_iqueue_cell_t *cell;
    MPIDU_genq_shmem_queue_t terminal;
    size_t capacity, available;
    char *payload;
    int ret = MPIDI_POSIX_OK;
    MPI_Aint packed_size = 0;

    MPIR_FUNC_ENTER;

    if (MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_QP_ENABLE) {
        return MPIDI_POSIX_eager_send_qp(grank, msg_hdr, am_hdr, am_hdr_sz, buf, count, datatype,
                                         offset, src_vci, dst_vci, bytes_sent);
    }

    /* Get the transport object that holds all of the global variables. */
    transport = MPIDI_POSIX_eager_iqueue_get_transport(src_vci, dst_vci);

    /* Try to get a new cell to hold the message */
    /* Select the appropriate free queue depending on whether we are using intra-NUMA or inter-NUMAfree
     * free queue. */
    int dst_local_rank = MPIDI_SHM_global.local_ranks[grank];
    bool is_topo_local =
        (MPIDI_POSIX_global.local_rank_dist[dst_local_rank] == MPIDI_POSIX_DIST__LOCAL);
    if (is_topo_local) {
        MPIDU_genq_shmem_pool_cell_alloc(transport->cell_pool, (void **) &cell,
                                         MPIR_Process.local_rank, 0 /* intra NUMA */ , buf);
    } else {
        MPIDU_genq_shmem_pool_cell_alloc(transport->cell_pool, (void **) &cell, dst_local_rank,
                                         1 /* inter NUMA */ , buf);
    }

    /* If a cell wasn't available, let the caller know that we weren't able to send the message
     * immediately. */
    if (unlikely(!cell)) {
        ret = MPIDI_POSIX_NOK;
        goto fn_exit;
    }

    /* Find the correct queue for this rank pair. */
    terminal = &transport->terminals[MPIDI_SHM_global.local_ranks[grank]];

    /* Get the memory allocated to be used for the message transportation. */
    payload = MPIDI_POSIX_EAGER_IQUEUE_CELL_PAYLOAD(cell);

    /* Figure out the capacity of each cell */
    capacity = MPIDI_POSIX_EAGER_IQUEUE_CELL_CAPACITY(transport);

    available = capacity;

    cell->from = MPIR_Process.local_rank;

    /* If this is the beginning of the message, mark it as the head. Otherwise it will be the
     * tail. */
    cell->payload_size = 0;
    if (am_hdr) {
        MPI_Aint resized_am_hdr_sz = MPL_ROUND_UP_ALIGN(am_hdr_sz, MAX_ALIGNMENT);
        cell->am_header = *msg_hdr;
        cell->type = MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_HDR;
        /* send am_hdr if this is the first segment */
        if (is_topo_local) {
            MPIR_Typerep_copy(payload, am_hdr, am_hdr_sz, MPIR_TYPEREP_FLAG_NONE);
        } else {
            MPIR_Typerep_copy(payload, am_hdr, am_hdr_sz, MPIR_TYPEREP_FLAG_STREAM);
        }
        /* make sure the data region starts at the boundary of MAX_ALIGNMENT */
        payload = payload + resized_am_hdr_sz;
        cell->payload_size += resized_am_hdr_sz;
        cell->am_header.am_hdr_sz = resized_am_hdr_sz;
        available -= cell->am_header.am_hdr_sz;
    } else {
        cell->type = MPIDI_POSIX_EAGER_IQUEUE_CELL_TYPE_DATA;
    }

    /* We want to skip packing of send buffer if there is no data to be sent . buf == NULL is
     * not a correct check here because derived datatype can use absolute address for displacement
     * which requires buffer address passed as MPI_BOTTOM which is usually NULL. count == 0 is also
     * not reliable because the derived datatype could have zero block size which contains no
     * data. */
    if (bytes_sent) {
        if (is_topo_local) {
            MPIR_Typerep_pack(buf, count, datatype, offset, payload, available, &packed_size,
                              MPIR_TYPEREP_FLAG_NONE);
        } else {
            MPIR_Typerep_pack(buf, count, datatype, offset, payload, available, &packed_size,
                              MPIR_TYPEREP_FLAG_STREAM);
        }
        cell->payload_size += packed_size;
        *bytes_sent = packed_size;
    }

    MPIDU_genq_shmem_queue_enqueue(transport->cell_pool, terminal, (void *) cell);

  fn_exit:
    MPIR_FUNC_EXIT;
    return ret;
}

#endif /* POSIX_EAGER_IQUEUE_SEND_H_INCLUDED */
