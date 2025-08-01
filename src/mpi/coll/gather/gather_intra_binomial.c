/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_GATHER_VSMALL_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 1024
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        use a temporary buffer for intracommunicator MPI_Gather if the send
        buffer size is < this value (in bytes)
        (See also: MPIR_CVAR_GATHER_INTER_SHORT_MSG_SIZE)

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Algorithm: MPI_Gather
 *
 * We use a binomial tree algorithm for both short and long messages. At nodes
 * other than leaf nodes we need to allocate a temporary buffer to store the
 * incoming message. If the root is not rank 0, for very small messages, we
 * pack it into a temporary contiguous buffer and reorder it to be placed in
 * the right order. For small (but not very small) messages, we use a derived
 * datatype to unpack the incoming data into non-contiguous buffers in the
 * right order.
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta where n is the total size of the data
 * gathered at the root.
 */
int MPIR_Gather_intra_binomial(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                               void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                               MPIR_Comm * comm_ptr, int coll_attr)
{
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    int relative_rank;
    int mask, src, dst, relative_src;
    MPI_Aint curr_cnt = 0, nbytes, sendtype_size, recvtype_size;
    int recvblks;
    int missing;
    MPI_Aint tmp_buf_size;
    void *tmp_buf = NULL;
    MPI_Status status;
    MPI_Aint extent = 0;        /* Datatype extent */
    MPI_Datatype types[2], tmp_type;
    int copy_offset = 0, copy_blks = 0;
    MPIR_CHKLMEM_DECL();


    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /* Use binomial tree algorithm. */

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    if (rank == root) {
        MPIR_Datatype_get_extent_macro(recvtype, extent);
    }

    if (rank == root) {
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount;
    } else {
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount;
    }

    /* Find the number of missing nodes in my sub-tree compared to
     * a balanced tree */
    for (mask = 1; mask < comm_size; mask <<= 1);
    --mask;
    while (relative_rank & mask)
        mask >>= 1;
    missing = (relative_rank | mask) - comm_size + 1;
    if (missing < 0)
        missing = 0;
    tmp_buf_size = (mask - missing);

    /* If the message is smaller than the threshold, we will copy
     * our message in there too */
    if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE)
        tmp_buf_size++;

    tmp_buf_size *= nbytes;

    /* For zero-ranked root, we don't need any temporary buffer */
    if ((rank == root) && (!root || (nbytes >= MPIR_CVAR_GATHER_VSMALL_MSG_SIZE)))
        tmp_buf_size = 0;

    if (tmp_buf_size) {
        MPIR_CHKLMEM_MALLOC(tmp_buf, tmp_buf_size);
    }

    if (rank == root) {
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                       ((char *) recvbuf + extent * recvcount * rank),
                                       recvcount, recvtype);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else if (tmp_buf_size && (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE)) {
        /* copy from sendbuf into tmp_buf */
        mpi_errno =
            MPIR_Localcopy(sendbuf, sendcount, sendtype, tmp_buf, nbytes, MPIR_BYTE_INTERNAL);
        MPIR_ERR_CHECK(mpi_errno);
    }
    curr_cnt = nbytes;

    mask = 0x1;
    while (mask < comm_size) {
        if ((mask & relative_rank) == 0) {
            src = relative_rank | mask;
            if (src < comm_size) {
                src = (src + root) % comm_size;

                if (rank == root) {
                    recvblks = mask;
                    if ((2 * recvblks) > comm_size)
                        recvblks = comm_size - recvblks;

                    if ((rank + mask + recvblks == comm_size) ||
                        (((rank + mask) % comm_size) < ((rank + mask + recvblks) % comm_size))) {
                        /* If the data contiguously fits into the
                         * receive buffer, place it directly. This
                         * should cover the case where the root is
                         * rank 0. */
                        mpi_errno = MPIC_Recv(((char *) recvbuf +
                                               (((rank +
                                                  mask) % comm_size) * (MPI_Aint) recvcount *
                                                extent)), (MPI_Aint) recvblks * recvcount,
                                              recvtype, src, MPIR_GATHER_TAG, comm_ptr, &status);
                        MPIR_ERR_CHECK(mpi_errno);
                    } else if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) {
                        /* small transfer size case. cast ok */
                        MPIR_Assert(recvblks * nbytes == (int) (recvblks * nbytes));
                        mpi_errno = MPIC_Recv(tmp_buf, (int) (recvblks * nbytes),
                                              MPIR_BYTE_INTERNAL, src, MPIR_GATHER_TAG, comm_ptr,
                                              &status);
                        MPIR_ERR_CHECK(mpi_errno);
                        copy_offset = rank + mask;
                        copy_blks = recvblks;
                    } else {
                        MPI_Aint blocks[2];
                        MPI_Aint displs[2];
                        blocks[0] = recvcount * (comm_size - root - mask);
                        displs[0] = recvcount * (root + mask);
                        blocks[1] = (recvcount * recvblks) - blocks[0];
                        displs[1] = 0;

                        mpi_errno =
                            MPIR_Type_indexed_large_impl(2, blocks, displs, recvtype, &tmp_type);
                        MPIR_ERR_CHECK(mpi_errno);

                        mpi_errno = MPIR_Type_commit_impl(&tmp_type);
                        MPIR_ERR_CHECK(mpi_errno);

                        mpi_errno = MPIC_Recv(recvbuf, 1, tmp_type, src,
                                              MPIR_GATHER_TAG, comm_ptr, &status);
                        MPIR_ERR_CHECK(mpi_errno);

                        MPIR_Type_free_impl(&tmp_type);
                    }
                } else {        /* Intermediate nodes store in temporary buffer */

                    MPI_Aint offset;

                    /* Estimate the amount of data that is going to come in */
                    recvblks = mask;
                    relative_src = ((src - root) < 0) ? (src - root + comm_size) : (src - root);
                    if (relative_src + mask > comm_size)
                        recvblks -= (relative_src + mask - comm_size);

                    if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE)
                        offset = mask * nbytes;
                    else
                        offset = (mask - 1) * nbytes;
                    mpi_errno = MPIC_Recv(((char *) tmp_buf + offset),
                                          recvblks * nbytes, MPIR_BYTE_INTERNAL, src,
                                          MPIR_GATHER_TAG, comm_ptr, &status);
                    MPIR_ERR_CHECK(mpi_errno);
                    curr_cnt += (recvblks * nbytes);
                }
            }
        } else {
            dst = relative_rank ^ mask;
            dst = (dst + root) % comm_size;

            if (!tmp_buf_size) {
                /* leaf nodes send directly from sendbuf */
                mpi_errno = MPIC_Send(sendbuf, sendcount, sendtype, dst,
                                      MPIR_GATHER_TAG, comm_ptr, coll_attr);
                MPIR_ERR_CHECK(mpi_errno);
            } else if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) {
                mpi_errno = MPIC_Send(tmp_buf, curr_cnt, MPIR_BYTE_INTERNAL, dst,
                                      MPIR_GATHER_TAG, comm_ptr, coll_attr);
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                MPI_Aint blocks[2];
                MPI_Aint struct_displs[2];
                blocks[0] = sendcount;
                struct_displs[0] = (MPI_Aint) sendbuf;
                types[0] = sendtype;
                /* check for overflow.  work around int limits if needed */
                if (curr_cnt - nbytes != (int) (curr_cnt - nbytes)) {
                    blocks[1] = 1;
                    MPIR_Type_contiguous_x_impl(curr_cnt - nbytes, MPIR_BYTE_INTERNAL, &(types[1]));
                } else {
                    MPIR_Assign_trunc(blocks[1], curr_cnt - nbytes, int);
                    types[1] = MPIR_BYTE_INTERNAL;
                }
                struct_displs[1] = (MPI_Aint) tmp_buf;
                mpi_errno =
                    MPIR_Type_create_struct_large_impl(2, blocks, struct_displs, types, &tmp_type);
                MPIR_ERR_CHECK(mpi_errno);

                mpi_errno = MPIR_Type_commit_impl(&tmp_type);
                MPIR_ERR_CHECK(mpi_errno);

                mpi_errno = MPIC_Send(MPI_BOTTOM, 1, tmp_type, dst,
                                      MPIR_GATHER_TAG, comm_ptr, coll_attr);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_Type_free_impl(&tmp_type);
                if (types[1] != MPIR_BYTE_INTERNAL)
                    MPIR_Type_free_impl(&types[1]);
            }

            break;
        }
        mask <<= 1;
    }

    if ((rank == root) && root && (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) && copy_blks) {
        /* reorder and copy from tmp_buf into recvbuf */
        mpi_errno = MPIR_Localcopy(tmp_buf,
                                   nbytes * (comm_size - copy_offset), MPIR_BYTE_INTERNAL,
                                   ((char *) recvbuf + extent * recvcount * copy_offset),
                                   recvcount * (comm_size - copy_offset), recvtype);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Localcopy((char *) tmp_buf + nbytes * (comm_size - copy_offset),
                                   nbytes * (copy_blks - comm_size + copy_offset),
                                   MPIR_BYTE_INTERNAL, recvbuf,
                                   recvcount * (copy_blks - comm_size + copy_offset), recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
