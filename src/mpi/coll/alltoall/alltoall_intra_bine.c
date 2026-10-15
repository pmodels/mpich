/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_bine.h"

/*
 * Algorithm: BINE
 *
 * Restrictions: power-of-two no. of processes
 *
 * Based on "BINE Trees: Enhancing Collective Operations
 * by Optimizing Communication Locality" Nov 2025
 * paper: https://doi.org/10.1145/3712285.3759835
 *
 * The alltoall can be viewed as a small vector allreduce,
 * where ranks send at each step 𝑛/2 bytes, and the received
 * data is concatenated with their own rather than being aggregated.
 * At each step, each rank moves the data it wants to keep to the
 * left of its buffer and the data it needs to send to the right, similar
 * to the rotations in Bruck’s algorithm. Each rank tracks the block
 * stored in each buffer position. At the end of the algorithm, a final
 * permutation step is needed to put the data in the correct order.
 *
 * Cost = lgp.alpha + (n/2).lgp.beta
 */
int MPIR_Alltoall_intra_bine(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                             MPIR_Comm *comm_ptr, int coll_attr) {

    int rank, comm_size, recvtype_sz, mpi_errno = MPI_SUCCESS;
    int inverse_mask, mask = 0x1, block_first_mask;
    int partner, ntbn, rotated_i, repr, index;
    MPI_Aint recvtype_extent;
    MPI_Aint i;
    MPI_Aint num_resident_blocks, num_resident_blocks_next, min_block_s,
             max_block_s;
    MPI_Aint block_recvd_cnt, block_send_cnt, offset_keep, offset_send;
    MPI_Aint block, remap_block, offset, offset_src, offset_dst;
    MPI_Aint sbuf_size, tmpbuf_size, tmpbuf_size_real;
    char *tmpbuf = NULL;
    MPI_Aint *resident_block, *resident_block_next;
    /* resident_block[i] contains the id of a block that is resident in the
     * current rank (for i < num_resident_blocks)
     * resident_block_next[i] contains the id of a block that is resident in
     * the current rank in the next step (for i < num_resident_blocks_next)
     */
    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf != MPI_IN_PLACE);
#endif /* HAVE_ERROR_CHECKING */

    /* Special case for comm_size == 1 */
    if (comm_size == 1) {
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* Get extent of send and recv types */
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    num_resident_blocks = comm_size;
    num_resident_blocks_next = 0;

    /* allocate temporary buffer */
    MPIR_Datatype_get_size_macro(recvtype, recvtype_sz);
    sbuf_size = recvcount * recvtype_sz;
    tmpbuf_size = sbuf_size * comm_size;
    tmpbuf_size_real = tmpbuf_size + sizeof(MPI_Aint) * comm_size + sizeof(MPI_Aint) * comm_size;

    MPIR_CHKLMEM_MALLOC(tmpbuf, tmpbuf_size_real);
    resident_block = (MPI_Aint *)(tmpbuf + tmpbuf_size);
    resident_block_next = (MPI_Aint *)(tmpbuf + tmpbuf_size + sizeof(MPI_Aint) * comm_size);

    /* At the beginning I only have my blocks */
    for (i = 0; i < comm_size; i++) {
        resident_block[i] = i;
    }

    mpi_errno = MPIR_Localcopy(sendbuf, sendcount * comm_size, sendtype,
                               tmpbuf, tmpbuf_size, MPIR_BYTE_INTERNAL);
    MPIR_ERR_CHECK(mpi_errno);

    /* We use recvbuf to receive/send the data, and tmpbuf to organize the data
     * to send at the next step. By doing so, we avoid a copy from tmpbuf to
     * recvbuf at the end
     */
    inverse_mask = 0x1 << (int)(MPL_log2(comm_size) - 1);
    block_first_mask = ~(inverse_mask - 1);

    while (mask < comm_size) {
        ntbn = MPII_Bine_negabinary_to_binary((mask << 1) - 1);
        if (rank % 2 == 0) {
            partner = MPII_Bine_mod(rank + ntbn, comm_size);
        } else {
            partner = MPII_Bine_mod(rank - ntbn, comm_size);
        }
        min_block_s = MPII_Bine_remap_rank(comm_size, partner) & block_first_mask;
        max_block_s = min_block_s + inverse_mask - 1;

        block_recvd_cnt = 0, block_send_cnt = 0;
        offset_send = 0, offset_keep = 0;
        num_resident_blocks_next = 0;
        for (i = 0; i < comm_size; i++) {
            block = resident_block[i % num_resident_blocks];
            /* Shall I send this block? Check the negabinary thing */
            remap_block = MPII_Bine_remap_rank(comm_size, block);
            offset = i * sbuf_size;

            /* I move to the beginning of tmpbuf the blocks I want to keep, */
            /* and I move to recvbuf the blocks I want to send. */
            if (remap_block >= min_block_s && remap_block <= max_block_s) {
                mpi_errno = MPIR_Localcopy(tmpbuf + offset, sbuf_size, MPIR_BYTE_INTERNAL,
                                           (char *)recvbuf + offset_send, sbuf_size,
                                           MPIR_BYTE_INTERNAL);
                MPIR_ERR_CHECK(mpi_errno);
                offset_send += sbuf_size;
                block_send_cnt++;
            } else {
                /* Copy the blocks we are not sending to the second half of
                 * recvbuf
                 */
                if (offset != offset_keep) {
                    mpi_errno = MPIR_Localcopy(tmpbuf + offset, sbuf_size, MPIR_BYTE_INTERNAL,
                                               tmpbuf + offset_keep, sbuf_size, MPIR_BYTE_INTERNAL);
                    MPIR_ERR_CHECK(mpi_errno);
                }
                offset_keep += sbuf_size;
                block_recvd_cnt++;

                resident_block_next[num_resident_blocks_next] = block;
                num_resident_blocks_next++;
            }
        }
        num_resident_blocks >>= 1;

        /* I receive data in the second half of tmpbuf (the first half contains */
        /* the blocks I am keeping from previous iteration) */
        mpi_errno = MPIC_Sendrecv((char *)recvbuf, sbuf_size * block_send_cnt,
                                  MPIR_BYTE_INTERNAL, partner, MPIR_ALLTOALL_TAG,
                                  tmpbuf + (comm_size / 2) * sbuf_size,
                                  sbuf_size * block_send_cnt, MPIR_BYTE_INTERNAL,
                                  partner, MPIR_ALLTOALL_TAG, comm_ptr,
                                  MPI_STATUS_IGNORE, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);

        /* Update resident blocks */
        mpi_errno = MPIR_Localcopy(resident_block_next, num_resident_blocks * sizeof(MPI_Aint),
                                   MPIR_BYTE_INTERNAL, resident_block,
                                   num_resident_blocks * sizeof(MPI_Aint), MPIR_BYTE_INTERNAL);
        MPIR_ERR_CHECK(mpi_errno);

        mask <<= 1;
        inverse_mask >>= 1;
        block_first_mask >>= 1;
    }

    /* Now I need to permute tmpbuf into recvbuf
     * Since I always received the new block on the right, and moved the blocks
     * I wanted to keep to the left, they are now sorted in the same order they
     * reached this rank from their corresponding source ranks. I.e., I should
     * consider the "reverse tree" (with this rank at the bottom and all the
     * other ranks on top), which represent how the data arrived here. This tree
     * is basically the opposite that I used to send the data I should consider
     * the decreasing tree, and vice-versa.
     */
    for (i = 0; i < comm_size; i++) {
        if ((rank % 2) == 0) {
            rotated_i = MPII_Bine_mod(i - rank, comm_size);
        } else {
            rotated_i = MPII_Bine_mod(rank - i, comm_size);
        }
        if (MPII_Bine_in_range(rotated_i, MPL_log2(comm_size))) {
            repr = MPII_Bine_binary_to_negabinary(rotated_i);
        } else {
            repr = MPII_Bine_binary_to_negabinary(rotated_i - comm_size);
        }
        index = MPII_Bine_remap_distance_doubling(repr);

        offset_src = index * sbuf_size;
        offset_dst = i * recvcount * recvtype_extent;
        mpi_errno = MPIR_Localcopy(tmpbuf + offset_src, sbuf_size, MPIR_BYTE_INTERNAL,
                                   (char *)recvbuf + offset_dst, recvcount, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}