/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "circ_graph.h"

/* Algorithm: Circulant graph allreduce
 *
 * This algorithm is a combination of reduce_circ_graph + bcast_circ_graph.
 *
 * It is not round-efficient for small to medium messages, but can be efficient for
 * large message when there are enough chunks to saturate the pipeline.
 */

int MPIR_Allreduce_intra_circ_graph(const void *sendbuf, void *recvbuf,
                                    MPI_Aint count, MPI_Datatype datatype,
                                    MPI_Op op, MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    int rank, comm_size;
    MPIR_COMM_RANK_SIZE(comm, rank, comm_size);

    MPIR_Assert(MPIR_Op_is_commutative(op));

    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* calculate the schedule */
    MPII_circ_graph cga;
    mpi_errno = MPII_circ_graph_create(&cga, comm_size, rank);
    MPIR_ERR_CHECK(mpi_errno);

    /* Run schedule */
    MPII_cga_request_queue queue;
    int min_pending_blocks = cga.q * 2;
    mpi_errno = MPII_cga_init_reduce_queue(&queue, min_pending_blocks,
                                           recvbuf, count, datatype, op, comm, coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    /* First run the reduce schedule */
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
            if (rank != 0) {
                if (send_block >= n) {
                    send_block = n - 1;
                }

                mpi_errno = MPII_cga_reduce_send(&queue, send_block, peer);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }

        int recv_block = cga.S[k] + offset;
        if (recv_block >= 0) {
            int peer = (rank + cga.Skip[k]) % p;
            if (peer != 0) {
                if (recv_block >= n) {
                    recv_block = n - 1;
                }

                mpi_errno = MPII_cga_reduce_recv(&queue, recv_block, peer);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }

        if (k == 0) {
            offset -= q;
        }
    }

    MPII_cga_switch_coll_type(&queue, MPII_CGA_BCAST);
    offset = -x;

    /* Then run the bcast schedule */
    for (int i = x; i < n - 1 + q + x; i++) {
        int k = i % q;

        int send_block = cga.S[k] + offset;
        if (send_block >= 0) {
            int peer = (rank + cga.Skip[k]) % p;
            if (peer != 0) {
                if (send_block >= n) {
                    send_block = n - 1;
                }

                mpi_errno = MPII_cga_bcast_send(&queue, send_block, peer);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }

        int recv_block = cga.R[k] + offset;
        if (recv_block >= 0) {
            int peer = (rank - cga.Skip[k] + p) % p;
            if (rank != 0) {
                if (recv_block >= n) {
                    recv_block = n - 1;
                }

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
