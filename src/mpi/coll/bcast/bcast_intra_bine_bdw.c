/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_bine.h"

/* Algorithm: BINE Bandwidth
 *
 * Based on "BINE Trees: Enhancing Collective Operations
 * by Optimizing Communication Locality" Nov 2025
 * paper: https://doi.org/10.1145/3712285.3759835
 *
 * For large messages, we use a BINE algorithm implemented with a scatter
 * followed by an allgather.
 * We first scatter using a recursive doubling BINE tree. This costs
 * lgp.alpha + n.((p-1)/p).beta
 * For the allgather, we use a recursive halving BINE tree, which
 * may cost approximately lgp.alpha + n.((p-1)/p).beta.
 *
 * Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta.
 */
int MPIR_Bcast_intra_bine_bdw(void *buffer, MPI_Aint count,
                              MPI_Datatype datatype, int root,
                              MPIR_Comm *comm_ptr, int coll_attr)
{
    int comm_size, rank, mpi_errno = MPI_SUCCESS, vrank;
    int mask, inverse_mask, block_first_mask, remapped_rank, receiving_mask;
    int recvd, partner, spartner, rpartner;
    int abs_partner, abs_spartner, abs_rpartner;
    int send_block_first, send_block_last, recv_block_first, recv_block_last;
    MPI_Aint send_count, recv_count, rem, bytes_per_rank;
    MPI_Aint *displs = NULL, *recvcounts = NULL;
    int is_contig;
    MPI_Aint true_extent, true_lb;
    MPI_Aint nbytes = 0, type_size;
    void *tmp_buf = NULL;

    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    if (comm_size == 1) {
        goto fn_exit;
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);
    if (HANDLE_IS_BUILTIN(datatype)) {
        is_contig = 1;
    } else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }

    nbytes = type_size * count;
    if (nbytes == 0) {
        goto fn_exit;
    }

    if (is_contig) {
        /* contiguous. no need to pack. */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        tmp_buf = MPIR_get_contig_ptr(buffer, true_lb);
    } else {
        MPIR_CHKLMEM_MALLOC(tmp_buf, nbytes);
        if (rank == root) {
            mpi_errno = MPIR_Localcopy(buffer, count, datatype,
                                       tmp_buf, nbytes, MPIR_BYTE_INTERNAL);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    MPIR_CHKLMEM_MALLOC(displs, comm_size * sizeof(MPI_Aint));
    MPIR_CHKLMEM_MALLOC(recvcounts, comm_size * sizeof(MPI_Aint));
    /* Distribute work in bytes rather than in elements, to improve
     * workload balancing when the datatype is not a builtin. */
    bytes_per_rank = nbytes / comm_size;
    rem = nbytes % comm_size;
    for (int i = 0; i < comm_size; i++) {
        displs[i] = bytes_per_rank * i + (i < rem ? i : rem);
        recvcounts[i] = bytes_per_rank + (i < rem ? 1 : 0);
    }

    vrank = MPII_Bine_mod(rank - root, comm_size);
    mask = 0x1;
    inverse_mask = 0x1 << (int) (MPL_log2(comm_size) - 1);
    block_first_mask = ~(inverse_mask - 1);
    remapped_rank = MPII_Bine_remap_rank(comm_size, vrank);
    /* Root never receives. By having a large mask inverse_mask will always be < receiving_mask */
    receiving_mask = inverse_mask << 1;

    /* I receive in the step corresponding to the position (starting from right)
     * of the first 1 in my remapped rank -- this indicates the step when the data reaches me
     */
    if (rank != root) {
        /* ffs starts counting from 1, thus -1 */
        receiving_mask = 0x1 << (MPII_Bine_ffs(remapped_rank) - 1);
    }

    /***** Scatter *****/
    recvd = (root == rank);
    while (mask < comm_size) {
        if (vrank % 2 == 0) {
            partner =
                MPII_Bine_mod(vrank + MPII_Bine_negabinary_to_binary((mask << 1) - 1), comm_size);
        } else {
            partner =
                MPII_Bine_mod(vrank - MPII_Bine_negabinary_to_binary((mask << 1) - 1), comm_size);
        }

        /* For sure I need to send my (remapped) partner's data
         * the actual start block however must be aligned to
         * the power of two
         */
        send_block_first = MPII_Bine_remap_rank(comm_size, partner) & block_first_mask;
        send_block_last = send_block_first + inverse_mask - 1;
        send_count =
            displs[send_block_last] - displs[send_block_first] + recvcounts[send_block_last];

        /* Something similar for the block to recv. */
        /* I receive my block, but aligned to the power of two */
        recv_block_first = remapped_rank & block_first_mask;
        recv_block_last = recv_block_first + inverse_mask - 1;
        recv_count =
            displs[recv_block_last] - displs[recv_block_first] + recvcounts[recv_block_last];

        abs_partner = MPII_Bine_mod(partner + root, comm_size);

        if (recvd) {
            mpi_errno =
                MPIC_Send((char *) tmp_buf + (MPI_Aint) displs[send_block_first],
                         (MPI_Aint) send_count, MPIR_BYTE_INTERNAL, abs_partner,
                         MPIR_BCAST_TAG, comm_ptr, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        } else if (inverse_mask == receiving_mask || partner == 0) {
            mpi_errno =
                MPIC_Recv((char *) tmp_buf + (MPI_Aint) displs[recv_block_first],
                          (MPI_Aint) recv_count, MPIR_BYTE_INTERNAL, abs_partner,
                          MPIR_BCAST_TAG, comm_ptr, MPI_STATUS_IGNORE);
            MPIR_ERR_CHECK(mpi_errno);
            recvd = 1;
        }

        mask <<= 1;
        inverse_mask >>= 1;
        block_first_mask >>= 1;
    }

    /***** Allgather *****/
    mask >>= 1;
    inverse_mask = 0x1;
    block_first_mask = ~0x0;
    while (mask > 0) {
        send_block_first = 0;
        send_block_last = 0;
        recv_block_first = 0;
        recv_block_last = 0;
        send_count = 0;
        recv_count = 0;

        if (vrank % 2 == 0) {
            partner =
                MPII_Bine_mod(vrank + MPII_Bine_negabinary_to_binary((mask << 1) - 1), comm_size);
        } else {
            partner =
                MPII_Bine_mod(vrank - MPII_Bine_negabinary_to_binary((mask << 1) - 1), comm_size);
        }

        rpartner = (inverse_mask < receiving_mask) ? MPI_PROC_NULL : partner;
        spartner = (inverse_mask == receiving_mask) ? MPI_PROC_NULL : partner;

        if (spartner != MPI_PROC_NULL) {
            send_block_first = remapped_rank & block_first_mask;
            send_block_last = send_block_first + inverse_mask - 1;
            send_count =
                displs[send_block_last] - displs[send_block_first] + recvcounts[send_block_last];
        }
        if (rpartner != MPI_PROC_NULL) {
            recv_block_first = MPII_Bine_remap_rank(comm_size, rpartner) & block_first_mask;
            recv_block_last = recv_block_first + inverse_mask - 1;
            recv_count =
                displs[recv_block_last] - displs[recv_block_first] + recvcounts[recv_block_last];
        }

        abs_rpartner =
            (rpartner != MPI_PROC_NULL) ? MPII_Bine_mod(rpartner + root, comm_size) : MPI_PROC_NULL;
        abs_spartner =
            (spartner != MPI_PROC_NULL) ? MPII_Bine_mod(spartner + root, comm_size) : MPI_PROC_NULL;

        mpi_errno = MPIC_Sendrecv((char *) tmp_buf + (MPI_Aint) displs[send_block_first],
                                  (MPI_Aint) send_count, MPIR_BYTE_INTERNAL, abs_spartner,
                                  MPIR_BCAST_TAG,
                                  (char *) tmp_buf + (MPI_Aint) displs[recv_block_first],
                                  (MPI_Aint) recv_count, MPIR_BYTE_INTERNAL, abs_rpartner,
                                  MPIR_BCAST_TAG, comm_ptr, MPI_STATUS_IGNORE, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);

        mask >>= 1;
        inverse_mask <<= 1;
        block_first_mask <<= 1;
    }

    if (!is_contig) {
        if (rank != root) {
            mpi_errno = MPIR_Localcopy(tmp_buf, nbytes, MPIR_BYTE_INTERNAL,
                                       buffer, count, datatype);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}