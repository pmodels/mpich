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
 * Reduction is performed as a reverse of the bcast schedule, specifically,
 * 1. iterate i from (n+q+x-2) down to x
 * 2. send to (r-Skip[k]) the block R[k]
 * 3. recv from (r+Skip[k]) the block S[k]
 * 4. perform reduction after each recv
 *
 * Legends:
 *     p - number of processes, comm_size
 *     r - the effective rank of the process
 *     q - ceil(log2(p)).
 *     n - total number of chunks (or blocks).
 *     k - the i-th round is referred as round k, where k = i % q. k = 0, 1, ..., q-1.
 *
 *     Skip[q+1] - each round process r send to r-Skip[k] nd receive from r+Skip[k], with a "MOD p" assumption.
 *     S[q] - in round k, this process receive the block S[k] from r+Skip[k]
 *     R[q] - in round k, this process send the block R[k] to r-Skip[k]
 */

static int calc_chunks(MPI_Aint data_size, MPI_Aint type_size, MPI_Aint chunk_size,
                       int *chunk_count_out, int *last_msg_size_out);

int MPIR_Reduce_intra_circ_graph(const void *sendbuf, void *recvbuf,
                                 MPI_Aint count, MPI_Datatype datatype,
                                 MPI_Op op, int root, MPIR_Comm * comm,
                                 int chunk_size, int q_len, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    int rank, comm_size;
    MPIR_COMM_RANK_SIZE(comm, rank, comm_size);

    /* minimum q_len is 2.
     * Consider the case p=10, k=2, when rank 1 sends to rank 3, and 3->5, 5->7, 7->9, 9->1,
     * which forms a send ring. In a rndv protocol, the send only can complete once the
     * corresponding recv is posted. Thus, in order to prevent deadlock when q_len is 1,
     * one of the process need post recv before send while other processes posts send before
     * recv. Because as p and k varies, the potential ring varies, and it require additional
     * complexity to post send and recv in each round correctly. Instead, when q_len >= 2,
     * the send and recv dependency within a round disappears.
     */
    if (q_len < 2) {
        q_len = 2;
    }

    MPIR_Assert(MPIR_Op_is_commutative(op));

    /* Create a temporary buffer */

    MPI_Aint true_lb, true_extent, extent;
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPI_Aint buf_size = (count - 1) * extent + true_extent;

    /* allocate tmp_buf to receive data */
    void *tmp_buf;
    MPIR_CHKLMEM_MALLOC(tmp_buf, buf_size);
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    /* non-root need allocate a recvbuf */
    if (rank != root) {
        MPIR_CHKLMEM_MALLOC(recvbuf, buf_size);
        recvbuf = (void *) ((char *) recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* calculate the schedule */
    MPII_circ_graph cga;
    int relative_rank = (rank - root + comm_size) % comm_size;
    mpi_errno = MPII_circ_graph_create(&cga, comm_size, relative_rank);
    MPIR_ERR_CHECK(mpi_errno);

    /* Handle pipeline chunks */
    MPI_Aint type_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);

    int chunk_count, last_msg_count;
    int n = calc_chunks(count, type_size, chunk_size, &chunk_count, &last_msg_count);

    /* Run schedule */
    MPII_cga_request_queue queue;
    mpi_errno = MPII_cga_init_reduce_queue(&queue, q_len, tmp_buf, recvbuf, n,
                                           chunk_count, last_msg_count, datatype, extent,
                                           op, comm, coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    /* reduction is the reverse of bcast schedule. Specifically -
     * 1. Iterate from last round down to x
     * 2. Send to r-Skip[k] block R[k]
     * 3. Recv from r+Skip[k] block S[k]
     */

    int p = cga.p;
    int q = cga.q;
    int x = (q - ((n - 1) % q)) % q;
    int offset = n - 1;

    for (int i = n + q + x - 2; i >= x; i--) {
        int k = i % q;

        int send_block = cga.R[k] + offset;
        if (send_block >= 0) {
            int peer = (rank - cga.Skip[k] + p) % p;
            if (rank != root) {
                if (send_block >= n) {
                    send_block = n - 1;
                }

                /* note: issue_send will wait for the receive if send_block is in receive or the queue is full */
                mpi_errno = MPII_cga_issue_send(&queue, send_block, peer);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }

        int recv_block = cga.S[k] + offset;
        if (recv_block >= 0) {
            int peer = (rank + cga.Skip[k]) % p;
            if (peer != root) {
                if (recv_block >= n) {
                    recv_block = n - 1;
                }

                /* note: issue_recv will wait for a request to complete if the queue is full */
                mpi_errno = MPII_cga_issue_recv(&queue, recv_block, peer);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }

        if (k == 0) {
            offset -= q;
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

static int calc_chunks(MPI_Aint count, MPI_Aint type_size, MPI_Aint chunk_size,
                       int *chunk_count_out, int *last_msg_count_out)
{
    int n;

    if (chunk_size == 0) {
        n = 1;
        *chunk_count_out = count;
        *last_msg_count_out = count;
    } else {
        MPI_Aint chunk_count = chunk_size / type_size;
        if (chunk_size % type_size > 0) {
            chunk_count++;
        }

        *chunk_count_out = chunk_count;
        n = (count / chunk_count);

        if (count % chunk_count == 0) {
            *last_msg_count_out = chunk_count;
        } else {
            n++;
            *last_msg_count_out = count % chunk_count;
        }
    }

    return n;
}
