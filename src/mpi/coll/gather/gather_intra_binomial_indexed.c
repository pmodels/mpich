
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


#undef FUNCNAME
#define FUNCNAME MPIR_Gather_intra_binomial_indexed
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Gather_intra_binomial_indexed(const void *sendbuf,
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

    /* communicator is homogeneous. no need to pack buffer. */

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
    if (missing < 0) {
        missing = 0;
    }
    tmp_buf_size = (mask - missing);

    tmp_buf_size *= nbytes;

    /* For zero-ranked root, we don't need any temporary buffer */
    if (rank == root) {
        tmp_buf_size = 0;
    }

    if (tmp_buf_size) {
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);
    }

    if (rank == root) {
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                       ((char *) recvbuf + extent * recvcount * rank), recvcount,
                                       recvtype);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
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
                    if ((2 * recvblks) > comm_size) {
                        recvblks = comm_size - recvblks;
                    }

                    if ((rank + mask + recvblks == comm_size) ||
                        (((rank + mask) % comm_size) < ((rank + mask + recvblks) % comm_size))) {
                        /* If the data contiguously fits into the
                         * receive buffer, place it directly. This
                         * should cover the case where the root is
                         * rank 0. */
                        mpi_errno = MPIC_Recv(((char *) recvbuf +
                                              (((rank + mask) % comm_size) * (MPI_Aint) recvcount *
                                              extent)), (MPI_Aint) recvblks * recvcount, recvtype,
                                              src, MPIR_GATHER_TAG, comm_ptr, &status, errflag);
                        if (mpi_errno) {
                            /* for communication errors, just record the error but continue */
                            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        }
                    } else {
                        blocks[0] = recvcount * (comm_size - root - mask);
                        displs[0] = recvcount * (root + mask);
                        blocks[1] = (recvcount * recvblks) - blocks[0];
                        displs[1] = 0;

                        mpi_errno = MPIR_Type_indexed_impl(2, blocks, displs, recvtype, &tmp_type);
                        if (mpi_errno) {
                            MPIR_ERR_POP(mpi_errno);
                        }

                        mpi_errno = MPIR_Type_commit_impl(&tmp_type);
                        if (mpi_errno) {
                            MPIR_ERR_POP(mpi_errno);
                        }

                        mpi_errno = MPIC_Recv(recvbuf, 1, tmp_type, src,
                                              MPIR_GATHER_TAG, comm_ptr, &status, errflag);
                        if (mpi_errno) {
                            /* for communication errors, just record the error but continue */
                            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        }

                        MPIR_Type_free_impl(&tmp_type);
                    }
                } else { /* Intermediate nodes store in temporary buffer */
                    MPI_Aint offset;

                    /* Estimate the amount of data that is going to come in */
                    recvblks = mask;
                    relative_src = ((src - root) < 0) ? (src - root + comm_size) : (src - root);
                    if (relative_src + mask > comm_size) {
                        recvblks -= (relative_src + mask - comm_size);
                    }

                    offset = (mask - 1) * nbytes;

                    mpi_errno = MPIC_Recv(((char *) tmp_buf + offset),
                                          recvblks * nbytes, MPI_BYTE, src,
                                          MPIR_GATHER_TAG, comm_ptr, &status, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                    curr_cnt += (recvblks * nbytes);
                }
            }
        } else {
            dst = relative_rank ^ mask;
            dst = (dst + root) % comm_size;

            if (!tmp_buf_size) {
                /* leaf nodes send directly from sendbuf */
                mpi_errno = MPIC_Send(sendbuf, sendcount, sendtype, dst,
                                      MPIR_GATHER_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            } else {
                blocks[0] = sendcount;
                struct_displs[0] = MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf;
                types[0] = sendtype;
                /* check for overflow.  work around int limits if needed */
                if (curr_cnt - nbytes != (int) (curr_cnt - nbytes)) {
                    blocks[1] = 1;
                    MPIR_Type_contiguous_x_impl(curr_cnt - nbytes, MPI_BYTE, &(types[1]));
                } else {
                    MPIR_Assign_trunc(blocks[1], curr_cnt - nbytes, int);
                    types[1] = MPI_BYTE;
                }
                struct_displs[1] = MPIR_VOID_PTR_CAST_TO_MPI_AINT tmp_buf;
                mpi_errno =
                    MPIR_Type_create_struct_impl(2, blocks, struct_displs, types, &tmp_type);
                if (mpi_errno) {
                    MPIR_ERR_POP(mpi_errno);
                }

                mpi_errno = MPIR_Type_commit_impl(&tmp_type);
                if (mpi_errno) {
                    MPIR_ERR_POP(mpi_errno);
                }

                mpi_errno = MPIC_Send(MPI_BOTTOM, 1, tmp_type, dst,
                                      MPIR_GATHER_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                MPIR_Type_free_impl(&tmp_type);
                if (types[1] != MPI_BYTE) {
                    MPIR_Type_free_impl(&types[1]);
                }
            }

            break;
        }
        mask <<= 1;
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
