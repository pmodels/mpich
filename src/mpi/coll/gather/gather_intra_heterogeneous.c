
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: MPI_Gather
 *
 * We use a binomial tree algorithm for both short and long messages. At nodes
 * other than leaf nodes we need to allocate a temporary buffer to store the
 * incoming message. If the root is not rank 0, for very small messages, we
 * pack it into a temporary contiguous buffer and reorder it to be placed in
 * the right order. For small (but not very small) messages, we use a derived
 * datatype to unpack the incoming data into non-contiguous buffers in the
 * right order. In the heterogeneous case we first pack the buffers by using
 * MPI_Pack and then do the gather.
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta where n is the total size of the data
 * gathered at the root.
 */

#ifdef MPID_HAS_HETERO
#undef FUNCNAME
#define FUNCNAME MPIR_Gather_intra_heterogeneous
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Gather_intra_heterogeneous(const void *sendbuf,
                                    int sendcount,
                                    MPI_Datatype sendtype,
                                    void *recvbuf,
                                    int recvcount,
                                    MPI_Datatype recvtype,
                                    int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size = 0;
    int rank = -1;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int relative_rank = -1;
    int mask = 0;
    int src = -1;
    int dst = -1;
    int relative_src = -1;
    MPI_Aint curr_cnt = 0;
    MPI_Aint nbytes = 0;
    MPI_Aint sendtype_size = 0;
    MPI_Aint recvtype_size = 0;
    int recvblks = 0;
    int missing = 0;
    MPI_Aint tmp_buf_size = 0;
    void *tmp_buf = NULL;
    MPI_Status status;
    MPI_Aint extent = 0; /* Datatype extent */
    int blocks[2];
    int displs[2];
    MPI_Aint struct_displs[2];
    MPI_Datatype types[2];
    MPI_Datatype tmp_type = recvtype;
    int copy_offset = 0;
    int copy_blks = 0;
    int position = -1;
    int recv_size = 0;

    MPIR_CHKLMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if (((rank == root) && (recvcount == 0)) || ((rank != root) && (sendcount == 0))) {
        return MPI_SUCCESS;
    }

    /* Use binomial tree algorithm. */

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    if (rank == root) {
        MPIR_Datatype_get_extent_macro(recvtype, extent);
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                         (extent * recvcount * comm_size));
    }

    /* communicator is heterogeneous. pack data into tmp_buf. */
    if (rank == root) {
        MPIR_Pack_size_impl(recvcount * comm_size, recvtype, &tmp_buf_size);
    } else {
        MPIR_Pack_size_impl(sendcount * (comm_size / 2), sendtype, &tmp_buf_size);
    }

    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

    position = 0;
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Pack_impl(sendbuf, sendcount, sendtype, tmp_buf, tmp_buf_size, &position);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
        nbytes = position;
    } else {
        /* do a dummy pack just to calculate nbytes */
        mpi_errno = MPIR_Pack_impl(recvbuf, 1, recvtype, tmp_buf, tmp_buf_size, &position);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
        nbytes = position * recvcount;
    }

    curr_cnt = nbytes;

    mask = 0x1;
    while (mask < comm_size) {
        if ((mask & relative_rank) == 0) {
            src = relative_rank | mask;
            if (src < comm_size) {
                src = (src + root) % comm_size;
                mpi_errno = MPIC_Recv(((char *) tmp_buf + curr_cnt),
                                      tmp_buf_size - curr_cnt, MPI_BYTE, src,
                                      MPIR_GATHER_TAG, comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    recv_size = 0;
                } else {
                    /* the recv size is larger than what may be sent in
                     * some cases. query amount of data actually received */
                    MPIR_Get_count_impl(&status, MPI_BYTE, &recv_size);
                    curr_cnt += recv_size;
                }
            }
        } else {
            dst = relative_rank ^ mask;
            dst = (dst + root) % comm_size;
            mpi_errno = MPIC_Send(tmp_buf, curr_cnt, MPI_BYTE, dst,
                                  MPIR_GATHER_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            break;
        }
        mask <<= 1;
    }

    if (rank == root) {
        /* reorder and copy from tmp_buf into recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            position = 0;
            mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position,
                                         ((char *) recvbuf + extent * recvcount * rank),
                                         recvcount * (comm_size - rank), recvtype);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
        } else {
            position = nbytes;
            mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position,
                                         ((char *) recvbuf + extent * recvcount * (rank + 1)),
                                         recvcount * (comm_size - rank - 1), recvtype);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
        if (root != 0) {
            mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position, recvbuf,
                                         recvcount * rank, recvtype);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
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
#endif /* MPID_HAS_HETERO */
