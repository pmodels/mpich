/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_bine.h"

/* Algortihm: BINE
 *
 * Restrictions: power-of-two no. of processes
 *
 * Based on "BINE Trees: Enhancing Collective Operations
 * by Optimizing Communication Locality" Nov 2025
 * paper: https://doi.org/10.1145/3712285.3759835
 *
 * We use this algorithm for both short and
 * long messages. At nodes other than leaf nodes we need to allocate
 * a temporary buffer to store the incoming message. If the root is
 * not rank 0, we reorder the sendbuf in order of relative ranks by
 * copying it into a temporary buffer, so that all the sends from the
 * root are contiguous and in the right order.
 *
 * At each step, each rank sends either the bottom or the top half of its
 * remaining buffer [a, b]. If a rank has k blocks of data,
 * it can either send the blocks [a, (a + k/2) mod p], or the blocks
 * [(b - k/2) mod p, b]
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta
 */
int MPIR_Scatter_intra_bine(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                            void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                            int root, MPIR_Comm *comm_ptr, int coll_attr)
{
    int comm_size, rank, mpi_errno = MPI_SUCCESS;
    int halving_direction, mask, recvd = 0, is_leaf = 0;
    int sbuf_offset, vrank, vrank_nb;
    int partner, mask_lsbs, lsbs, equal_lsbs;
    int nbytes = 0;
    int tmp_buf_size;
    MPI_Aint min_resident_block, max_resident_block;
    MPI_Aint top_start, top_end, bottom_start, bottom_end;
    MPI_Aint send_start, send_end, recv_start, recv_end;
    MPI_Aint stext = 0;
    MPI_Aint num_blocks;
    char *tmp_buf = NULL, *sbuf = NULL, *rbuf = NULL;

    MPIR_CHKLMEM_DECL();

    if (sendcount == 0 || (recvcount == 0 && recvbuf != MPI_IN_PLACE)) {
        goto fn_exit;
    }

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /* Special case for comm_size == 1 */
    if (comm_size == 1) {
        if (recvbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype);
            MPIR_ERR_CHECK(mpi_errno);
        }
        goto fn_exit;
    }

    if (rank == root) {
        MPIR_Datatype_get_extent_macro(sendtype, stext);
    }

    /* MPII_Bine_mod computes math modulo rather than reminder */
    vrank = MPII_Bine_mod(rank - root, comm_size);

    if (rank == root) {
        /* We separate the two cases (root and non-root) because
         * in the event of recvbuf=MPI_IN_PLACE on the root,
         * recvcount and recvtype are not valid */
        MPI_Aint stsize;
        MPIR_Datatype_get_size_macro(sendtype, stsize);
        nbytes = stsize * sendcount;
    } else {
        MPI_Aint rtsize;
        MPIR_Datatype_get_size_macro(recvtype, rtsize);
        nbytes = rtsize * recvcount;
    }

    /* Down -- send bottom half */
    halving_direction = 1;
    if (vrank % 2) {
        /* Up -- send top half */
        halving_direction = -1;
    }
    /* The gather started with these directions. Thus this will
     * be the direction they ended up with if we have an odd number
     * of steps. If not, invert.
     */
    if (MPL_log2(comm_size) % 2 == 0) {
        halving_direction *= -1;
    }

    /* I need to do the opposite of what I did in the gather.
     * Thus, I need to know where min_resident_block and max_resident_block
     * ended up after the last step.
     * Even ranks added 2^0, 2^2, 2^4, ... to max_resident_block
     *   and subtracted 2^1, 2^3, 2^5, ... from min_resident_block
     * Odd ranks subtracted 2^0, 2^2, 2^4, ... from min_resident_block
     *      and added 2^1, 2^3, 2^5, ... to max_resident_block
     */
    if (vrank % 2 == 0) {
        max_resident_block =
            MPII_Bine_mod((vrank + 0x55555555) & ((0x1 << (int) MPL_log2(comm_size)) - 1),
                          comm_size);
        min_resident_block =
            MPII_Bine_mod((vrank - 0xAAAAAAAA) & ((0x1 << (int) MPL_log2(comm_size)) - 1),
                          comm_size);
    } else {
        min_resident_block =
            MPII_Bine_mod((vrank - 0x55555555) & ((0x1 << (int) MPL_log2(comm_size)) - 1),
                          comm_size);
        max_resident_block =
            MPII_Bine_mod((vrank + 0xAAAAAAAA) & ((0x1 << (int) MPL_log2(comm_size)) - 1),
                          comm_size);
    }

    /* In the case comm_size == 1 we set the mask to 0 to skip
     * the while loop and copy only the sendbuf into the recvbuf
     * as the final step.
     */
    mask = 0x1 << (int) (MPL_log2(comm_size) - 1);
    sbuf_offset = vrank;
    if (root == rank) {
        recvd = 1;
        sbuf = (char *) sendbuf;
    }

    /* if the root is not rank 0, we reorder the sendbuf in order of
     * relative ranks and copy it into a temporary buffer, so that
     * all the sends from the root are contiguous and in the right
     * order. */
    if (rank == root) {
        if (root != 0) {
            tmp_buf_size = nbytes * comm_size;
            MPIR_CHKLMEM_MALLOC(tmp_buf, tmp_buf_size);

            if (recvbuf != MPI_IN_PLACE) {
                mpi_errno = MPIR_Localcopy(((char *) sendbuf + stext * sendcount * rank),
                                           sendcount * (comm_size - rank), sendtype, tmp_buf,
                                           nbytes * (comm_size - rank), MPIR_BYTE_INTERNAL);
            } else {
                mpi_errno = MPIR_Localcopy(((char *) sendbuf + stext * sendcount * (rank + 1)),
                                           sendcount * (comm_size - rank - 1),
                                           sendtype, (char *) tmp_buf + nbytes,
                                           nbytes * (comm_size - rank - 1), MPIR_BYTE_INTERNAL);
            }
            MPIR_ERR_CHECK(mpi_errno);

            mpi_errno = MPIR_Localcopy(sendbuf, sendcount * rank, sendtype,
                                       ((char *) tmp_buf + nbytes * (comm_size - rank)),
                                       nbytes * rank, MPIR_BYTE_INTERNAL);
            MPIR_ERR_CHECK(mpi_errno);

            sbuf = (char *) tmp_buf;
        } else {
            sbuf = (char *) sendbuf;
        }
    }

    vrank_nb = MPII_Bine_binary_to_negabinary(vrank);
    while (mask > 0) {
        partner = vrank_nb ^ ((mask << 1) - 1);
        partner = MPII_Bine_mod(MPII_Bine_negabinary_to_binary(partner) + root, comm_size);
        /* Mask with num_steps - step + 1 LSBs set to 1 */
        mask_lsbs = (mask << 1) - 1;
        /* Extract k LSBs */
        lsbs = vrank_nb & mask_lsbs;
        equal_lsbs = (lsbs == 0 || lsbs == mask_lsbs);

        top_start = min_resident_block;
        top_end = MPII_Bine_mod(min_resident_block + mask - 1, comm_size);
        bottom_start = MPII_Bine_mod(top_end + 1, comm_size);
        bottom_end = max_resident_block;
        if (halving_direction == 1) {
            /* Send bottom half [..., size - 1] */
            send_start = bottom_start;
            send_end = bottom_end;
            recv_start = top_start;
            recv_end = top_end;
            max_resident_block = MPII_Bine_mod(max_resident_block - mask, comm_size);
        } else {
            /* Send top half [0, ...] */
            send_start = top_start;
            send_end = top_end;
            recv_start = bottom_start;
            recv_end = bottom_end;
            min_resident_block = MPII_Bine_mod(min_resident_block + mask, comm_size);
        }

        if (recvd) {
            /* Since intermediate nodes forward data using tmp_buf and use MPIR_BYTE_INTERNAL
             * as datatype, we distinguish the MPIC_Send()
             * performed by the root (which use the original buffer and datatype) from those
             * performed by intermediate nodes.
             */
            if (rank == root && root == 0) {
                mpi_errno = MPIC_Send((char *) sbuf + send_start * sendcount * stext,
                                      sendcount * (send_end - send_start + 1), sendtype,
                                      partner, MPIR_SCATTER_TAG, comm_ptr, coll_attr);
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                mpi_errno = MPIC_Send((char *) sbuf + send_start * nbytes,
                                      nbytes * (send_end - send_start + 1), MPIR_BYTE_INTERNAL,
                                      partner, MPIR_SCATTER_TAG, comm_ptr, coll_attr);
                MPIR_ERR_CHECK(mpi_errno);
            }
        } else if (equal_lsbs) {
            /* Setup the buffers to be used from now on
             * How large should the tmpbuf be?
             * It must be large enough to hold a number of blocks
             * equal to the number of children in the tree rooted in me.
             */
            num_blocks = MPII_Bine_mod((recv_end - recv_start + 1), comm_size);
            if (recv_start == recv_end) {
                /* I am a leaf and this is the last step, I do not need a tmpbuf */
                rbuf = (char *) recvbuf;
                is_leaf = 1;
            } else {
                MPIR_CHKLMEM_MALLOC(tmp_buf, nbytes * num_blocks);
                sbuf = (char *) tmp_buf;
                rbuf = (char *) tmp_buf;

                /* Adjust min and max resident blocks */
                min_resident_block = 0;
                max_resident_block = num_blocks - 1;

                sbuf_offset = MPII_Bine_mod(vrank - recv_start, comm_size);
            }
            if (recv_end >= recv_start || partner != root) {
                if (!is_leaf) {
                    mpi_errno =
                        MPIC_Recv((char *) rbuf, nbytes * num_blocks,
                                  MPIR_BYTE_INTERNAL, partner, MPIR_SCATTER_TAG, comm_ptr,
                                  MPI_STATUS_IGNORE);
                } else {
                    /* The leaf receives directly to the recv buffer */
                    mpi_errno = MPIC_Recv((char *) rbuf, recvcount * num_blocks, recvtype,
                                          partner, MPIR_SCATTER_TAG, comm_ptr, MPI_STATUS_IGNORE);
                }
                MPIR_ERR_CHECK(mpi_errno);
            }
            recvd = 1;
        }
        mask >>= 1;
        halving_direction *= -1;
    }

    if ((rank == root) && (root == 0) && (recvbuf != MPI_IN_PLACE)) {
        /* put rank's data in recvbuf if not MPI_IN_PLACE */
        mpi_errno =
            MPIR_Localcopy((char *) sbuf + sbuf_offset * stext * sendcount, sendcount, sendtype,
                           (char *) recvbuf, recvcount, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (!is_leaf && (recvbuf != MPI_IN_PLACE)) {
        /* non-leaf nodes copy the data from tmp_buf to recvbuf if not MPI_IN_PLACE */
        mpi_errno = MPIR_Localcopy((char *) sbuf + sbuf_offset * nbytes, nbytes,
                                   MPIR_BYTE_INTERNAL, (char *) recvbuf,
                                   recvcount, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
