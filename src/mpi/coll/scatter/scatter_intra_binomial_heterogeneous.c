
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* This is the machine-independent implementation of scatter. The algorithm is:

   Algorithm: MPI_Scatter

   We use a binomial tree algorithm for both short and
   long messages. At nodes other than leaf nodes we need to allocate
   a temporary buffer to store the incoming message. If the root is
   not rank 0, we reorder the sendbuf in order of relative ranks by
   copying it into a temporary buffer, so that all the sends from the
   root are contiguous and in the right order. In the heterogeneous
   case, we first pack the buffer by using MPI_Pack and then do the
   scatter.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is the total size of the data to be scattered from the root.

   Possible improvements:

   End Algorithm: MPI_Scatter
*/

#ifdef MPID_HAS_HETERO
#undef FUNCNAME
#define FUNCNAME MPIR_Scatter__intra__binomial_heterogeneous
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Scatter__intra__binomial_heterogeneous(const void *sendbuf,
                                                int sendcount,
                                                MPI_Datatype sendtype,
                                                void *recvbuf,
                                                int recvcount,
                                                MPI_Datatype recvtype,
                                                int root, MPIR_Comm * comm_ptr,
                                                MPIR_Errflag_t * errflag)
{
    MPI_Status status;
    MPI_Aint extent = 0;
    int rank = 0;
    int comm_size = -1;
    int sendtype_size = 0;
    int curr_cnt = 0;
    int relative_rank = 0;
    int nbytes = 0;
    int send_subtree_cnt = 0;
    int mask = 0;
    int recvtype_size = 0;
    int src = 0;
    int dst = 0;
    int tmp_buf_size = 0;
    void *tmp_buf = NULL;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int position = 0;

    MPIR_CHKLMEM_DECL(4);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if (((rank == root) && (sendcount == 0)) || ((rank != root) && (recvcount == 0))) {
        return MPI_SUCCESS;
    }

    /* Use binomial tree algorithm */

    if (rank == root) {
        MPIR_Datatype_get_extent_macro(sendtype, extent);
    }

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    if (rank == root) {
        MPIR_Pack_size_impl(sendcount * comm_size, sendtype, &tmp_buf_size);

        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

        /* calculate the value of nbytes, the number of bytes in packed
         * representation that each process receives. We can't
         * accurately calculate that from tmp_buf_size because
         * MPI_Pack_size returns an upper bound on the amount of memory
         * required. (For example, for a single integer, MPICH-1 returns
         * pack_size=12.) Therefore, we actually pack some data into
         * tmp_buf and see by how much 'position' is incremented. */

        position = 0;
        mpi_errno = MPIR_Pack_impl(sendbuf, 1, sendtype, tmp_buf, tmp_buf_size, &position);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }

        nbytes = position * sendcount;

        curr_cnt = nbytes * comm_size;

        if (root == 0) {
            if (recvbuf != MPI_IN_PLACE) {
                position = 0;
                mpi_errno = MPIR_Pack_impl(sendbuf, sendcount * comm_size,
                                           sendtype, tmp_buf, tmp_buf_size, &position);
                if (mpi_errno) {
                    MPIR_ERR_POP(mpi_errno);
                }
            } else {
                position = nbytes;
                mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent * sendcount),
                                           sendcount * (comm_size - 1),
                                           sendtype, tmp_buf, tmp_buf_size, &position);
                if (mpi_errno) {
                    MPIR_ERR_POP(mpi_errno);
                }
            }
        } else {
            if (recvbuf != MPI_IN_PLACE) {
                position = 0;
                mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent * sendcount * rank),
                                           sendcount * (comm_size - rank),
                                           sendtype, tmp_buf, tmp_buf_size, &position);
                if (mpi_errno) {
                    MPIR_ERR_POP(mpi_errno);
                }
            } else {
                position = nbytes;
                mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent * sendcount * (rank + 1)),
                                           sendcount * (comm_size - rank - 1),
                                           sendtype, tmp_buf, tmp_buf_size, &position);
                if (mpi_errno) {
                    MPIR_ERR_POP(mpi_errno);
                }
            }
            mpi_errno = MPIR_Pack_impl(sendbuf, sendcount * rank, sendtype,
                                       tmp_buf, tmp_buf_size, &position);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
    } else {
        MPIR_Pack_size_impl(recvcount * (comm_size / 2), recvtype, &tmp_buf_size);
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

        /* calculate nbytes */
        position = 0;
        mpi_errno = MPIR_Pack_impl(recvbuf, 1, recvtype, tmp_buf, tmp_buf_size, &position);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
        nbytes = position * recvcount;

        curr_cnt = 0;
    }

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask;
            if (src < 0) {
                src += comm_size;
            }

            mpi_errno = MPIC_Recv(tmp_buf, tmp_buf_size, MPI_BYTE, src,
                                  MPIR_SCATTER_TAG, comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                curr_cnt = 0;
            } else {
                /* the recv size is larger than what may be sent in
                 * some cases. query amount of data actually received */
                MPIR_Get_count_impl(&status, MPI_BYTE, &curr_cnt);
            }
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
     * set from the LSB upto (but not including) mask.  Because of
     * the "not including", we start by shifting mask back down
     * one. */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            dst = rank + mask;
            if (dst >= comm_size) {
                dst -= comm_size;
            }

            send_subtree_cnt = curr_cnt - nbytes * mask;
            /* mask is also the size of this process's subtree */
            mpi_errno = MPIC_Send(((char *) tmp_buf + nbytes * mask),
                                  send_subtree_cnt, MPI_BYTE, dst,
                                  MPIR_SCATTER_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            curr_cnt -= send_subtree_cnt;
        }
        mask >>= 1;
    }

    /* copy local data into recvbuf */
    position = 0;
    if (recvbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position, recvbuf,
                                     recvcount, recvtype);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret) {
        mpi_errno = mpi_errno_ret;
    } else if (*errflag != MPIR_ERR_NONE) {
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif
