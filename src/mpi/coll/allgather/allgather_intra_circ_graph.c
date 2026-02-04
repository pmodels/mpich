/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "circ_graph.h"

/* Algorithm: Circulant graph allgather
 *
 * Based on the paper by Jesper Larsson Traff: https://dl.acm.org/doi/full/10.1145/3735139.
 *
 * First review bcast_intra_circ_graph.c. Allgather is essentially every rank as root and
 * bcast to every other processes.
 */

int MPIR_Allgather_intra_circ_graph(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                    MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    int rank, comm_size;
    MPIR_COMM_RANK_SIZE(comm, rank, comm_size);

    if (sendbuf != MPI_IN_PLACE) {
        MPI_Aint recvtype_extent;
        MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
        void *dstbuf = (char *) recvbuf + rank * recvcount * recvtype_extent;
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype, dstbuf, recvcount, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* calculate the schedule */
    MPII_circ_graph cga;
    mpi_errno = MPII_circ_graph_create_all(&cga, comm_size);
    MPIR_ERR_CHECK(mpi_errno);

    /* Run schedule */
    MPII_cga_request_queue queue;
    mpi_errno = MPII_cga_init_allgather_queue(&queue, recvbuf, recvcount, recvtype,
                                              comm, coll_attr, rank, comm_size);
    MPIR_ERR_CHECK(mpi_errno);

    int n = queue.num_chunks;
    int p = cga.p;
    int q = cga.q;
    int x = (q - ((n - 1) % q)) % q;
    int offset = -x;

    for (int i = x; i < n - 1 + q + x; i++) {
        int k = i % q;
        int peer_to = (rank + cga.Skip[k]) % p;
        int peer_from = (rank - cga.Skip[k] + p) % p;

        /* note: we assume the message is large enough that it makes sense to send individual blocks
         *       separately. For small messages, use brucks algorithm instead.
         */
        /* TODO: pack messages up to chunk_size */

        /* Run the bcast schedule assuming root from 0 to p - 1 */
        /* note: we do this to avoid deadlock with minimum q_len = 2 */
        for (int root = 0; root < p; root++) {
            int r = (rank - root + p) % p;
            int send_block = cga.S[r * q + k] + offset;
            /* skip negative blocks and skip sending to root */
            if (send_block >= 0 && peer_to != root) {
                if (send_block >= n) {
                    send_block = n - 1;
                }

                mpi_errno = MPII_cga_allgather_send(&queue, root, send_block, peer_to);
                MPIR_ERR_CHECK(mpi_errno);
            }

            int recv_block = cga.R[r * q + k] + offset;
            /* skip negative blocks and skip recving as root */
            if (recv_block >= 0 && r != 0) {
                if (recv_block >= n) {
                    recv_block = n - 1;
                }

                mpi_errno = MPII_cga_allgather_recv(&queue, root, recv_block, peer_from);
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
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
