/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "circ_graph.h"

static int wait_if_full(MPII_cga_request_queue * queue);
static int wait_for_request(MPII_cga_request_queue * queue, int i);

/* Routines for managing non-blocking send/recv of chunks  */
int MPII_cga_init_bcast_queue(MPII_cga_request_queue * queue,
                              int q_len, void *buf, int n, int chunk_size, int last_chunk_size,
                              MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;

    queue->q_len = q_len;
    queue->buf = buf;
    queue->n = n;
    /* only with contig chunks for now */
    queue->chunk_count = chunk_size;
    queue->last_chunk_count = last_chunk_size;
    queue->chunk_extent = chunk_size;
    queue->datatype = MPIR_BYTE_INTERNAL;

    queue->comm = comm;
    queue->coll_attr = coll_attr;

    queue->q_head = 0;
    queue->q_tail = 0;

    queue->can_send = MPL_malloc(n * sizeof(*queue->can_send), MPL_MEM_OTHER);
    if (!queue->can_send) {
        MPL_free(queue);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    queue->requests = MPL_malloc(q_len * sizeof(*queue->requests), MPL_MEM_OTHER);
    if (!queue->requests) {
        MPL_free(queue);
        MPL_free(queue->can_send);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    for (int i = 0; i < n; i++) {
        /* the schedule ensures a recv always precedes a send for the same block
         * unless the block is already available (e.g. root in bcast). So we can
         * initialize can_send[i] to true and reset it to false as we issue recvs.
         */
        queue->can_send[i] = true;
    }

    for (int i = 0; i < q_len; i++) {
        queue->requests[i].req = NULL;
        queue->requests[i].chunk_id = -1;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_issue_send(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    if (!queue->can_send[block]) {
        for (int i = 0; i < queue->q_len; i++) {
            if (queue->requests[i].chunk_id == block) {
                mpi_errno = wait_for_request(queue, i);
                MPIR_ERR_CHECK(mpi_errno);
                break;
            }
        }
    }
    MPIR_Assert(queue->can_send[block]);

    void *buf = (char *) queue->buf + block * queue->chunk_extent;
    MPI_Aint count = (block == queue->n - 1) ? queue->last_chunk_count : queue->chunk_count;

    mpi_errno = MPIC_Isend(buf, count, queue->datatype,
                           peer_rank, MPIR_BCAST_TAG, queue->comm,
                           &(queue->requests[queue->q_head].req), queue->coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    queue->requests[queue->q_head].chunk_id = -1;

    queue->q_head = (queue->q_head + 1) % queue->q_len;

    mpi_errno = wait_if_full(queue);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_issue_recv(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    void *buf = (char *) queue->buf + block * queue->chunk_extent;
    MPI_Aint msg_size = (block == queue->n - 1) ? queue->last_chunk_count : queue->chunk_count;

    mpi_errno = MPIC_Irecv(buf, msg_size, MPIR_BYTE_INTERNAL,
                           peer_rank, MPIR_BCAST_TAG, queue->comm,
                           &(queue->requests[queue->q_head].req));
    MPIR_ERR_CHECK(mpi_errno);

    queue->can_send[block] = false;
    queue->requests[queue->q_head].chunk_id = block;

    queue->q_head = (queue->q_head + 1) % queue->q_len;

    mpi_errno = wait_if_full(queue);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_waitall(MPII_cga_request_queue * queue)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 0; i < queue->q_len; i++) {
        if (queue->requests[i].req) {
            mpi_errno = MPIC_Wait(queue->requests[i].req);
            MPIR_ERR_CHECK(mpi_errno);

            MPIR_Request_free(queue->requests[i].req);
        }
    }
    MPL_free(queue->can_send);
    MPL_free(queue->requests);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* internal routines */

static int wait_if_full(MPII_cga_request_queue * queue)
{
    int mpi_errno = MPI_SUCCESS;

    if (queue->q_head == queue->q_tail) {
        if (queue->requests[queue->q_tail].req) {
            mpi_errno = wait_for_request(queue, queue->q_tail);
            MPIR_ERR_CHECK(mpi_errno);
        }

        queue->q_tail = (queue->q_tail + 1) % queue->q_len;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int wait_for_request(MPII_cga_request_queue * queue, int i)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIC_Wait(queue->requests[i].req);
    MPIR_ERR_CHECK(mpi_errno);

    if (queue->requests[i].chunk_id != -1) {
        /* it's a recv, update can_send */
        queue->can_send[queue->requests[i].chunk_id] = true;
    }
    MPIR_Request_free(queue->requests[i].req);
    queue->requests[i].req = NULL;
    queue->requests[i].chunk_id = -1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
