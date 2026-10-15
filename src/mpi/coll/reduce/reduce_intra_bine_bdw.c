/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_bine.h"

/*
 * Algorithm: BINE Bandwidth
 *
 * Based on "BINE Trees: Enhancing Collective Operations
 * by Optimizing Communication Locality" Nov 2025
 * paper: https://doi.org/10.1145/3712285.3759835
 *
 * For large vectors, we
 * uses a distance-doubling Bine butterfly for the reduce-scatter and
 * a distance-halving Bine tree for the gather.
 *
 * Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma
 */
int MPIR_Reduce_intra_bine_bdw(const void *sendbuf, void *recvbuf,
                               MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                               int root, MPIR_Comm *comm_ptr, int coll_attr) {

    int comm_size, rank, vrank, adjsize, mpi_errno = MPI_SUCCESS, steps,
                                                 step;
    int extra_ranks, new_rank, loop_flag;
    int count_per_rank, rem, mask = 0x1, inverse_mask;
    int block_first_mask, remapped_rank, receiving_mask;
    int *rindex = NULL, *sindex = NULL, *rcount = NULL, *scount = NULL;
    char *tmpbuf = NULL;
    int partner, abs_partner, nbtb;
    int send_block_first, send_block_last, recv_block_first, recv_block_last;
    MPI_Aint true_lb = 0, true_extent = 0, extent = 0;
    MPIR_CHKLMEM_DECL();

    if (count == 0) {
        goto fn_exit;
    }

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    if (comm_size == 1) {
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count,
                                       datatype);
            MPIR_ERR_CHECK(mpi_errno);
        }
        goto fn_exit;
    }

    MPIR_Assert(MPIR_Op_is_commutative(op));

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    /* Determine nearest power of two less than or equal to comm_size
     * and return an error if comm_size is 0
     */
    steps = MPII_Bine_hibit(comm_size, (int)(sizeof(comm_size) * CHAR_BIT) - 1);
    MPIR_ERR_CHKANDJUMP(steps == -1, mpi_errno, MPI_ERR_ARG, "**arg");
    adjsize = 1 << steps; /* Largest power of two <= comm_size */

    /* mod computes math modulo rather than reminder */
    vrank = MPII_Bine_mod(rank - root, comm_size);

    count_per_rank = count / adjsize;
    rem = count % adjsize;

    MPIR_CHKLMEM_MALLOC(tmpbuf, count * (MPL_MAX(extent, true_extent)));
    tmpbuf = (void *)((char *)tmpbuf - true_lb);

    /* If I'm not the root, then my recvbuf may not be valid, therefore
     * I have to allocate a temporary one */
    if (rank != root) {
        MPIR_CHKLMEM_MALLOC(recvbuf, count * (MPL_MAX(extent, true_extent)));
        recvbuf = (void *) ((char *) recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        mpi_errno =
            MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Number of nodes that exceed the largest power of two less than or equal
     * to comm_size
     */
    extra_ranks = comm_size - adjsize;

    /* First part of computation to get a 2^n number of nodes.
     * What happens is that first #extra_rank even nodes sends their
     * data to the successive node and do not partecipate in the general
     * collective call operation.
     * All the nodes that do not stop their computation will receive an alias
     * called new_node, used to calculate their correct destination wrt this
     * new "cut" topology.
     */
    new_rank = vrank;
    loop_flag = 0;
    if (vrank < (2 * extra_ranks)) {
        if (0 != (vrank % 2)) {
            mpi_errno = MPIC_Send(recvbuf, count, datatype,
                                  MPII_Bine_mod((vrank - 1) + root, comm_size),
                                  MPIR_REDUCE_TAG, comm_ptr, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
            loop_flag = 1;
        } else {
            mpi_errno = MPIC_Recv(tmpbuf, count, datatype,
                                  MPII_Bine_mod((vrank + 1) + root, comm_size),
                                  MPIR_REDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_Reduce_local((char *)tmpbuf, (char *)recvbuf, count,
                                          datatype, op);
            MPIR_ERR_CHECK(mpi_errno);
            new_rank = vrank >> 1;
        }
    } else {
        new_rank = vrank - extra_ranks;
    }

    mask = 0x1;
    inverse_mask = 0x1 << (int)(MPL_log2(adjsize) - 1);
    block_first_mask = ~(inverse_mask - 1);
    remapped_rank = MPII_Bine_remap_rank(adjsize, new_rank);

    /***** Reduce_scatter *****/
    MPIR_CHKLMEM_MALLOC(rindex, sizeof(int) * steps);
    MPIR_CHKLMEM_MALLOC(sindex, sizeof(int) * steps);
    MPIR_CHKLMEM_MALLOC(rcount, sizeof(int) * steps);
    MPIR_CHKLMEM_MALLOC(scount, sizeof(int) * steps);
    step = 0;
    if (!loop_flag) {
        while (mask < adjsize) {
            nbtb = MPII_Bine_negabinary_to_binary((mask << 1) - 1);
            if (new_rank % 2 == 0) {
                partner = MPII_Bine_mod(new_rank + nbtb, adjsize);
            } else {
                partner = MPII_Bine_mod(new_rank - nbtb, adjsize);
            }
            abs_partner = (partner < extra_ranks) ? (partner * 2)
                                                  : (partner + extra_ranks);
            /* Compute the absolute rank */
            abs_partner = MPII_Bine_mod(abs_partner + root, comm_size);

            /* Compute send block boundaries inline */
            send_block_first =
                MPII_Bine_remap_rank(adjsize, partner) & block_first_mask;
            send_block_last = send_block_first + inverse_mask - 1;
            sindex[step] = count_per_rank * send_block_first +
                           (send_block_first < rem ? send_block_first : rem);
            scount[step] =
                count_per_rank * (send_block_last - send_block_first + 1) +
                (MPL_MIN(send_block_last, rem) -
                 MPL_MIN(send_block_first, rem)) +
                (send_block_last < rem ? 1 : 0);

            /* Compute recv block boundaries inline */
            recv_block_first = remapped_rank & block_first_mask;
            recv_block_last = recv_block_first + inverse_mask - 1;
            rindex[step] = count_per_rank * recv_block_first +
                           (recv_block_first < rem ? recv_block_first : rem);
            rcount[step] =
                count_per_rank * (recv_block_last - recv_block_first + 1) +
                (MPL_MIN(recv_block_last, rem) -
                 MPL_MIN(recv_block_first, rem)) +
                (recv_block_last < rem ? 1 : 0);

            mpi_errno = MPIC_Sendrecv(
                (char *)recvbuf + sindex[step] * extent, scount[step], datatype,
                abs_partner, MPIR_REDUCE_TAG, (char *)tmpbuf + rindex[step] * extent,
                rcount[step], datatype, abs_partner, MPIR_REDUCE_TAG, comm_ptr,
                MPI_STATUS_IGNORE, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_Reduce_local((char *)tmpbuf + rindex[step] * extent,
                                          (char *)recvbuf + rindex[step] * extent,
                                          rcount[step], datatype, op);
            MPIR_ERR_CHECK(mpi_errno);

            mask <<= 1;
            inverse_mask >>= 1;
            block_first_mask >>= 1;
            step++;
        }

        /***** Gather *****/
        mask >>= 1;
        inverse_mask = 0x1;
        block_first_mask = ~0x0;
        /* I send in the step corresponding to the position (starting from
         * right) of the first 1 in my remapped rank -- this indicates the step
         * when the data reaches me in a scatter
         */
        receiving_mask = 0; /* Root never sends in gather */
        if (rank != root) {
            receiving_mask = 0x1
                             << (MPII_Bine_ffs(remapped_rank) -
                                 1); /* ffs starts counting from 1, thus -1 */
        }
        step = steps - 1;
        while (mask > 0) {
            nbtb = MPII_Bine_negabinary_to_binary((mask << 1) - 1);
            if (new_rank % 2 == 0) {
                partner = MPII_Bine_mod(new_rank + nbtb, adjsize);
            } else {
                partner = MPII_Bine_mod(new_rank - nbtb, adjsize);
            }
            abs_partner = (partner < extra_ranks) ? (partner * 2)
                                                  : (partner + extra_ranks);
            /* Compute the absolute rank */
            abs_partner = MPII_Bine_mod(abs_partner + root, comm_size);

            /* Only the one with 0 in the i-th bit starting from the left (i is
             * the step) survives
             */
            if (inverse_mask & receiving_mask) {
                mpi_errno = MPIC_Send((char *)recvbuf + rindex[step] * extent,
                                      rcount[step], datatype, abs_partner,
                                      MPIR_REDUCE_TAG, comm_ptr, coll_attr);
                MPIR_ERR_CHECK(mpi_errno);
                break;
            } else {
                /* Something similar for the block to recv.
                 * I receive my partner's block, but aligned to the power of two
                 */
                mpi_errno = MPIC_Recv(
                    (char *)recvbuf + sindex[step] * extent, scount[step], datatype,
                    abs_partner, MPIR_REDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE);
                MPIR_ERR_CHECK(mpi_errno);
            }

            mask >>= 1;
            inverse_mask <<= 1;
            block_first_mask <<= 1;
            step--;
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
