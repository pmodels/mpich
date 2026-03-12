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

int MPIR_Bcast_intra_circ_graph(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                int root, MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    int rank, comm_size;
    MPIR_COMM_RANK_SIZE(comm, rank, comm_size);

    if (comm_size < 2) {
        goto fn_exit;
    }

    /* calculate the schedule */
    MPII_circ_graph cga;
    int relative_rank = (rank - root + comm_size) % comm_size;
    mpi_errno = MPII_circ_graph_create(&cga, comm_size, relative_rank);
    MPIR_ERR_CHECK(mpi_errno);

    /* request queue */
    MPII_cga_request_queue queue;
    int min_pending_blocks = cga.q * 2;
    mpi_errno = MPII_cga_init_bcast_queue(&queue, min_pending_blocks,
                                          buffer, count, datatype, comm, coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    /* Run schedule */
    int p = cga.p;
    int q = cga.q;
    int n = queue.num_chunks;
    int x = (q - ((n - 1) % q)) % q;
    int offset = -x;

    for (int i = x; i < n - 1 + q + x; i++) {
        int k = i % q;

        int send_block = cga.S[k] + offset;
        if (send_block >= 0) {
            int peer = (rank + cga.Skip[k]) % p;
            if (peer != root) {
                if (send_block >= n) {
                    send_block = n - 1;
                }

                /* note: issue_send will wait for the receive if send_block is in receive or the queue is full */
                mpi_errno = MPII_cga_bcast_send(&queue, send_block, peer);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }

        int recv_block = cga.R[k] + offset;
        if (recv_block >= 0) {
            int peer = (rank - cga.Skip[k] + p) % p;
            if (rank != root) {
                if (recv_block >= n) {
                    recv_block = n - 1;
                }

                /* note: issue_recv will wait for a request to complete if the queue is full */
                mpi_errno = MPII_cga_bcast_recv(&queue, recv_block, peer);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }

        if (k == q - 1) {
            offset += q;
        }
    }

    /* wait for all pending requests */
    mpi_errno = MPII_cga_waitall(&queue);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_circ_graph_free(&cga);
    MPIR_CHKLMEM_FREEALL();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
