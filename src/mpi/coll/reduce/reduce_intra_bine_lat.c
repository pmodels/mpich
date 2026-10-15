/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_bine.h"

/*
 * Algorithm: BINE Latency
 *
 * Based on "BINE Trees: Enhancing Collective Operations
 * by Optimizing Communication Locality" Nov 2025
 * paper: https://doi.org/10.1145/3712285.3759835
 *
 * This function implements a BINE tree reduce.
 *
 * Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma
 */
int MPIR_Reduce_intra_bine_lat(const void *sendbuf, void *recvbuf,
                               MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                               int root, MPIR_Comm *comm_ptr, int coll_attr) {

    int comm_size, rank, vrank, mask, mpi_errno = MPI_SUCCESS;
    int steps, adjsize, extra_ranks, loop_flag, new_rank;
    int partner, abs_partner, btnb_vrank, mask_lsbs, lsbs, equal_lsbs;
    MPI_Aint true_lb, true_extent, extent;
    char *tmpbuf = NULL;
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

    /* If I'm not the root, then my recvbuf may not be valid, therefore
     * I have to allocate a temporary one */
    if (rank != root) {
        MPIR_CHKLMEM_MALLOC(recvbuf, count * (MPL_MAX(extent, true_extent)));
        recvbuf = (void *)((char *)recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        mpi_errno =
            MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_CHKLMEM_MALLOC(tmpbuf, count * (MPL_MAX(extent, true_extent)));
    /* adjust for potential negative lower bound in datatype */
    tmpbuf = (void *)((char *)tmpbuf - true_lb);

    /* mod computes math modulo rather than reminder */
    vrank = MPII_Bine_mod(rank - root, comm_size);

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
    btnb_vrank = MPII_Bine_binary_to_negabinary(new_rank);
    if (!loop_flag) {
        while (mask < adjsize) {
            partner = btnb_vrank ^ ((mask << 1) - 1);
            partner =
                MPII_Bine_mod(MPII_Bine_negabinary_to_binary(partner), adjsize);
            /* Compute absolute partner */
            abs_partner = (partner < extra_ranks) ? (partner * 2)
                                                  : (partner + extra_ranks);
            abs_partner = MPII_Bine_mod(abs_partner + root, comm_size);
            mask_lsbs = (mask << 2) - 1; /* Mask with step + 2 LSBs set to 1 */
            lsbs = btnb_vrank & mask_lsbs; /* Extract k LSBs */
            equal_lsbs = (lsbs == 0 || lsbs == mask_lsbs);

            if (!equal_lsbs || ((mask << 1) >= adjsize && (rank != root))) {
                mpi_errno = MPIC_Send(recvbuf, count, datatype, abs_partner,
                                      MPIR_REDUCE_TAG, comm_ptr, coll_attr);
                MPIR_ERR_CHECK(mpi_errno);
                break;
            } else {
                mpi_errno =
                    MPIC_Recv(tmpbuf, count, datatype, abs_partner,
                              MPIR_REDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE);
                MPIR_ERR_CHECK(mpi_errno);
                mpi_errno =
                    MPIR_Reduce_local(tmpbuf, recvbuf, count, datatype, op);
                MPIR_ERR_CHECK(mpi_errno);
            }
            mask <<= 1;
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}