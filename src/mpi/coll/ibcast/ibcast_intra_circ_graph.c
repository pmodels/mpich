/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "circ_graph.h"

/* Algorithm: Circulant graph bcast
 *
 * Based on the paper by Jesper Larsson Traff: https://dl.acm.org/doi/full/10.1145/3735139.
 *
 * For n + q - 1 rounds, each process send to (r+Skip[k]) the block S[k], and
 * receive from (r-Skip[k]) the block R[k], where -
 *
 *     p - number of processes, comm_size
 *     r - the effective rank of the process
 *     q - ceil(log2(p)).
 *     n - total number of chunks (or blocks).
 *     k - the i-th round is referred as round k, where k = i % q. k = 0, 1, ..., q-1.
 *
 *     Skip[q+1] - each round process r send to r+Skip[k] nd receive from r-Skip[k], with a "MOD p" assumption.
 *     S[q] - in round k, this process sends the block S[k] (to r+Skip[k])
 *     R[q] - in round k, this process receives the block R[k] (from r-Skip[k])
 */

struct cga_bcast {
    MPII_circ_graph cga;
    MPII_cga_request_queue queue;
    int root;
    int rank;
    int iter_cur;
    int iter_end;
    int iter_offset;
    /* each iter round has one send and one recv, thus we need a bool to mark where we are at */
    bool iter_sent;
    MPIR_Request *req_ptr;
};

static int cga_bcast_poll(MPIX_Async_thing thing)
{
    int mpi_errno = MPI_SUCCESS;
    struct cga_bcast *state = MPIR_Async_thing_get_state(thing);

    int p = state->cga.p;
    int q = state->cga.q;
    int n = state->queue.num_chunks;

    for (; state->iter_cur < state->iter_end; state->iter_cur++) {
        int k = state->iter_cur % q;

        int send_block = state->cga.S[k] + state->iter_offset;
        if (send_block >= 0 && !state->iter_sent) {
            int peer = (state->rank + state->cga.Skip[k]) % p;
            if (peer != state->root) {
                if (send_block >= n) {
                    send_block = n - 1;
                }

                /* note: issue_send will wait for the receive if send_block is in receive or the queue is full */
                bool flag = false;
                mpi_errno = MPII_cga_bcast_isend(&state->queue, send_block, peer, &flag);
                MPIR_ERR_CHECK(mpi_errno);
                if (!flag) {
                    goto fn_cont;
                }
            }
        }

        state->iter_sent = true;

        int recv_block = state->cga.R[k] + state->iter_offset;
        if (recv_block >= 0) {
            int peer = (state->rank - state->cga.Skip[k] + p) % p;
            if (state->rank != state->root) {
                if (recv_block >= n) {
                    recv_block = n - 1;
                }

                /* note: issue_recv will wait for a request to complete if the queue is full */
                bool flag = false;
                mpi_errno = MPII_cga_bcast_irecv(&state->queue, recv_block, peer, &flag);
                MPIR_ERR_CHECK(mpi_errno);
                if (!flag) {
                    goto fn_cont;
                }
            }
        }

        state->iter_sent = false;

        if (k == q - 1) {
            state->iter_offset += q;
        }
    }

    bool done;
    mpi_errno = MPII_cga_testall(&state->queue, &done);
    MPIR_ERR_CHECK(mpi_errno);
    if (done) {
        MPII_circ_graph_free(&state->cga);
        MPIR_Grequest_complete_impl(state->req_ptr);
        MPL_free(state);
        return MPIX_ASYNC_DONE;
    }

  fn_cont:
    return MPIX_ASYNC_NOPROGRESS;
  fn_fail:
    /* TODO: Error handling */
    MPIR_Assert(0);
    return MPIX_ASYNC_NOPROGRESS;
}

int MPIR_Ibcast_intra_circ_graph(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                 int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size;
    MPIR_COMM_RANK_SIZE(comm, rank, comm_size);

    if (comm_size < 2) {
        goto fn_exit;
    }

    struct cga_bcast *state = MPL_malloc(sizeof(struct cga_bcast), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!state, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* calculate the schedule */
    int relative_rank = (rank - root + comm_size) % comm_size;
    mpi_errno = MPII_circ_graph_create(&state->cga, comm_size, relative_rank);
    MPIR_ERR_CHECK(mpi_errno);

    int q = state->cga.q;

    /* request queue */
    int min_pending_blocks = q * 2;
    int coll_attr = 0;
    mpi_errno = MPII_cga_init_bcast_queue(&state->queue, min_pending_blocks,
                                          buffer, count, datatype, comm, coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    int n = state->queue.num_chunks;
    int x = (q - ((n - 1) % q)) % q;
    state->iter_cur = x;
    state->iter_end = n - 1 + q + x;
    state->iter_offset = -x;

    state->rank = rank;
    state->root = root;

    mpi_errno = MPIR_Grequest_start_impl(NULL, NULL, NULL, NULL, req);
    MPIR_ERR_CHECK(mpi_errno);
    state->req_ptr = *req;

    mpi_errno = MPIR_Async_things_add(cga_bcast_poll, state, NULL);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
