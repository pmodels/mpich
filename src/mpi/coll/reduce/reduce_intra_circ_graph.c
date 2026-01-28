/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "circ_graph.h"

/* Algorithm: Circulant graph reduce
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

int MPIR_Reduce_intra_circ_graph(const void *sendbuf, void *recvbuf,
                                 MPI_Aint count, MPI_Datatype datatype,
                                 MPI_Op op, int root, MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    int rank, comm_size;
    MPIR_COMM_RANK_SIZE(comm, rank, comm_size);

    MPIR_Assert(MPIR_Op_is_commutative(op));

    /* non-root need allocate a recvbuf */
    if (rank != root) {
        MPI_Aint true_lb, true_extent, extent;
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPI_Aint buf_size = (count - 1) * extent + true_extent;

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

    /* Run schedule */
    MPII_cga_request_queue queue;
    mpi_errno = MPII_cga_init_reduce_queue(&queue, recvbuf, count, datatype, op, comm, coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    /* reduction is the reverse of bcast schedule. Specifically -
     * 1. Iterate from last round down to x
     * 2. Send to r-Skip[k] block R[k]
     * 3. Recv from r+Skip[k] block S[k]
     */

    int n = queue.num_chunks;
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
