/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* This is the machine-independent implementation of scatter. The algorithm is:

   Algorithm: Binomial

   We use a binomial tree algorithm for both short and
   long messages. At nodes other than leaf nodes we need to allocate
   a temporary buffer to store the incoming message. If the root is
   not rank 0, we reorder the sendbuf in order of relative ranks by
   copying it into a temporary buffer, so that all the sends from the
   root are contiguous and in the right order.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is the total size of the data to be scattered from the root.

   Possible improvements:

   End Algorithm: MPI_Scatter
*/


/* not declared static because a machine-specific function may call this one in some cases */
int MPIR_Scatter_intra_binomial(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                MPIR_Comm * comm_ptr, int coll_attr)
{
    MPI_Status status;
    MPI_Aint extent = 0;
    int rank, comm_size;
    int relative_rank;
    MPI_Aint curr_cnt, send_subtree_cnt;
    int mask, src, dst;
    MPI_Aint tmp_buf_size = 0;
    void *tmp_buf = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    if (rank == root)
        MPIR_Datatype_get_extent_macro(sendtype, extent);

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    MPI_Aint nbytes;
    if (rank == root) {
        /* We separate the two cases (root and non-root) because
         * in the event of recvbuf=MPI_IN_PLACE on the root,
         * recvcount and recvtype are not valid */
        MPI_Aint sendtype_size;
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount;
    } else {
        MPI_Aint recvtype_size;
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount;
    }

    curr_cnt = 0;

    /* all even nodes other than root need a temporary buffer to
     * receive data of max size (nbytes*comm_size)/2 */
    if (relative_rank && !(relative_rank % 2)) {
        tmp_buf_size = (nbytes * comm_size) / 2;
        MPIR_CHKLMEM_MALLOC(tmp_buf, tmp_buf_size);
    }

    /* if the root is not rank 0, we reorder the sendbuf in order of
     * relative ranks and copy it into a temporary buffer, so that
     * all the sends from the root are contiguous and in the right
     * order. */
    if (rank == root) {
        if (root != 0) {
            tmp_buf_size = nbytes * comm_size;
            MPIR_CHKLMEM_MALLOC(tmp_buf, tmp_buf_size);

            if (recvbuf != MPI_IN_PLACE)
                mpi_errno = MPIR_Localcopy(((char *) sendbuf + extent * sendcount * rank),
                                           sendcount * (comm_size - rank), sendtype, tmp_buf,
                                           nbytes * (comm_size - rank), MPIR_BYTE_INTERNAL);
            else
                mpi_errno = MPIR_Localcopy(((char *) sendbuf + extent * sendcount * (rank + 1)),
                                           sendcount * (comm_size - rank - 1),
                                           sendtype, (char *) tmp_buf + nbytes,
                                           nbytes * (comm_size - rank - 1), MPIR_BYTE_INTERNAL);
            MPIR_ERR_CHECK(mpi_errno);

            mpi_errno = MPIR_Localcopy(sendbuf, sendcount * rank, sendtype,
                                       ((char *) tmp_buf + nbytes * (comm_size - rank)),
                                       nbytes * rank, MPIR_BYTE_INTERNAL);
            MPIR_ERR_CHECK(mpi_errno);

            curr_cnt = nbytes * comm_size;
        } else
            curr_cnt = sendcount * comm_size;
    }

    /* root has all the data; others have zero so far */

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask;
            if (src < 0)
                src += comm_size;

            /* The leaf nodes receive directly into recvbuf because
             * they don't have to forward data to anyone. Others
             * receive data into a temporary buffer. */
            if (relative_rank % 2) {
                mpi_errno = MPIC_Recv(recvbuf, recvcount, recvtype,
                                      src, MPIR_SCATTER_TAG, comm_ptr, &status);
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                mpi_errno = MPIC_Recv(tmp_buf, tmp_buf_size, MPIR_BYTE_INTERNAL, src,
                                      MPIR_SCATTER_TAG, comm_ptr, &status);
                MPIR_ERR_CHECK(mpi_errno);
                if (mpi_errno) {
                    curr_cnt = 0;
                } else
                    /* the recv size is larger than what may be sent in
                     * some cases. query amount of data actually received */
                    MPIR_Get_count_impl(&status, MPIR_BYTE_INTERNAL, &curr_cnt);
            }
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
     * set from the LSB up to (but not including) mask.  Because of
     * the "not including", we start by shifting mask back down
     * one. */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            dst = rank + mask;
            if (dst >= comm_size)
                dst -= comm_size;

            if ((rank == root) && (root == 0)) {
                send_subtree_cnt = curr_cnt - sendcount * mask;
                /* mask is also the size of this process's subtree */
                mpi_errno = MPIC_Send(((char *) sendbuf +
                                       extent * sendcount * mask),
                                      send_subtree_cnt,
                                      sendtype, dst, MPIR_SCATTER_TAG, comm_ptr, coll_attr);
            } else {
                /* non-zero root and others */
                send_subtree_cnt = curr_cnt - nbytes * mask;
                /* mask is also the size of this process's subtree */
                mpi_errno = MPIC_Send(((char *) tmp_buf + nbytes * mask),
                                      send_subtree_cnt,
                                      MPIR_BYTE_INTERNAL, dst, MPIR_SCATTER_TAG, comm_ptr,
                                      coll_attr);
            }
            MPIR_ERR_CHECK(mpi_errno);
            curr_cnt -= send_subtree_cnt;
        }
        mask >>= 1;
    }

    if ((rank == root) && (root == 0) && (recvbuf != MPI_IN_PLACE)) {
        /* for root=0, put root's data in recvbuf if not MPI_IN_PLACE */
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (!(relative_rank % 2) && (recvbuf != MPI_IN_PLACE)) {
        /* for non-zero root and non-leaf nodes, copy from tmp_buf
         * into recvbuf */
        mpi_errno =
            MPIR_Localcopy(tmp_buf, nbytes, MPIR_BYTE_INTERNAL, recvbuf, recvcount, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
