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

static int calc_chunks(MPI_Aint data_size, MPI_Aint chunk_size, int *last_msg_size_out);

int MPIR_Bcast_intra_circ_graph(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                int root, MPIR_Comm * comm,
                                int chunk_size, int q_len, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    int rank, comm_size;
    MPIR_COMM_RANK_SIZE(comm, rank, comm_size);

    if (comm_size < 2) {
        goto fn_exit;
    }
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

    /* Prepare the data blocks */
    MPI_Aint type_size;
    int is_contig;
    int buf_size;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    buf_size = count * type_size;

    if (buf_size == 0)
        goto fn_exit;

    MPIR_Datatype_is_contig(datatype, &is_contig);

    /* calculate the schedule */
    MPII_circ_graph cga;
    int relative_rank = (rank - root + comm_size) % comm_size;
    mpi_errno = MPII_circ_graph_create(&cga, comm_size, relative_rank);
    MPIR_ERR_CHECK(mpi_errno);

    /* if necessary, pack data in tmp_buf */
    void *tmp_buf;
    if (is_contig) {
        tmp_buf = buffer;
    } else {
        MPIR_CHKLMEM_MALLOC(tmp_buf, buf_size);
        if (rank == root) {
            MPI_Aint actual_pack_bytes;
            MPIR_Typerep_pack(buffer, count, datatype, 0,
                              tmp_buf, buf_size, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_Assert(actual_pack_bytes == buf_size);
        }
    }

    /* Handle pipeline chunks */
    int last_msg_size;
    int n = calc_chunks(buf_size, chunk_size, &last_msg_size);

    /* Run schedule */
    MPII_cga_request_queue queue;
    mpi_errno = MPII_cga_init_bcast_queue(&queue, q_len, tmp_buf, n, chunk_size, last_msg_size,
                                          comm, coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    int p = cga.p;
    int q = cga.q;
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
                mpi_errno = MPII_cga_issue_send(&queue, send_block, peer);
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
                mpi_errno = MPII_cga_issue_recv(&queue, recv_block, peer);
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

    if (!is_contig) {
        MPI_Aint actual_unpack_bytes;
        MPIR_Typerep_unpack(tmp_buf, buf_size, buffer, count, datatype,
                            0, &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(actual_unpack_bytes == buf_size);
    }

    MPII_circ_graph_free(&cga);
    MPIR_CHKLMEM_FREEALL();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int calc_chunks(MPI_Aint buf_size, MPI_Aint chunk_size, int *last_msg_size_out)
{
    int n;
    int last_msg_size;

    if (chunk_size == 0) {
        n = 1;
        last_msg_size = buf_size;
    } else {
        n = (buf_size / chunk_size);
        if (buf_size % chunk_size == 0) {
            last_msg_size = chunk_size;
        } else {
            n++;
            last_msg_size = buf_size % chunk_size;
        }
    }
    *last_msg_size_out = last_msg_size;
    return n;
}
