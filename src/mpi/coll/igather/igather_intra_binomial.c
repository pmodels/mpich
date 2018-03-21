/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* This is the machine-independent implementation of igather. The algorithm is:

   Algorithm: MPI_Igather

   We use a binomial tree algorithm for both short and long
   messages. At nodes other than leaf nodes we need to allocate a
   temporary buffer to store the incoming message. If the root is not
   rank 0, for very small messages, we pack it into a temporary
   contiguous buffer and reorder it to be placed in the right
   order. For small (but not very small) messages, we use a derived
   datatype to unpack the incoming data into non-contiguous buffers in
   the right order.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is the total size of the data gathered at the root.

   Possible improvements:

   End Algorithm: MPI_Gather
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Igather_sched_intra_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igather_sched_intra_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                      MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, rank;
    int relative_rank;
    int mask, src, dst, relative_src;
    MPI_Aint recvtype_size, sendtype_size, curr_cnt = 0, nbytes;
    int recvblks;
    int tmp_buf_size, missing;
    void *tmp_buf = NULL;
    int blocks[2];
    int displs[2];
    MPI_Aint struct_displs[2];
    MPI_Aint extent = 0;
    int copy_offset = 0, copy_blks = 0;
    MPI_Datatype types[2], tmp_type;
    MPIR_SCHED_CHKPMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if (((rank == root) && (recvcount == 0)) || ((rank != root) && (sendcount == 0)))
        goto fn_exit;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    /* Use binomial tree algorithm. */

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    if (rank == root) {
        MPIR_Datatype_get_extent_macro(recvtype, extent);
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                         (extent * recvcount * comm_size));
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
        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf",
                                  MPL_MEM_BUFFER);
    }

    if (rank == root) {
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                       ((char *) recvbuf + extent * recvcount * rank),
                                       recvcount, recvtype);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    } else if (tmp_buf_size && (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE)) {
        /* copy from sendbuf into tmp_buf */
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype, tmp_buf, nbytes, MPI_BYTE);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
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
                        char *rp =
                            (char *) recvbuf + (((rank + mask) % comm_size) * recvcount * extent);
                        mpi_errno =
                            MPIR_Sched_recv(rp, (recvblks * recvcount), recvtype, src, comm_ptr, s);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);
                        mpi_errno = MPIR_Sched_barrier(s);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);
                    } else if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) {
                        mpi_errno =
                            MPIR_Sched_recv(tmp_buf, (recvblks * nbytes), MPI_BYTE, src,
                                            comm_ptr, s);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);
                        mpi_errno = MPIR_Sched_barrier(s);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);
                        copy_offset = rank + mask;
                        copy_blks = recvblks;
                    } else {
                        blocks[0] = recvcount * (comm_size - root - mask);
                        displs[0] = recvcount * (root + mask);
                        blocks[1] = (recvcount * recvblks) - blocks[0];
                        displs[1] = 0;

                        mpi_errno = MPIR_Type_indexed_impl(2, blocks, displs, recvtype, &tmp_type);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);
                        mpi_errno = MPIR_Type_commit_impl(&tmp_type);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);

                        mpi_errno = MPIR_Sched_recv(recvbuf, 1, tmp_type, src, comm_ptr, s);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);
                        mpi_errno = MPIR_Sched_barrier(s);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);

                        /* this "premature" free is safe b/c the sched holds an actual ref to keep it alive */
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
                    mpi_errno =
                        MPIR_Sched_recv(((char *) tmp_buf + offset), (recvblks * nbytes),
                                        MPI_BYTE, src, comm_ptr, s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    mpi_errno = MPIR_Sched_barrier(s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    curr_cnt += (recvblks * nbytes);
                }
            }
        } else {
            dst = relative_rank ^ mask;
            dst = (dst + root) % comm_size;

            if (!tmp_buf_size) {
                /* leaf nodes send directly from sendbuf */
                mpi_errno = MPIR_Sched_send(sendbuf, sendcount, sendtype, dst, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                mpi_errno = MPIR_Sched_barrier(s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            } else if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) {
                mpi_errno = MPIR_Sched_send(tmp_buf, curr_cnt, MPI_BYTE, dst, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                mpi_errno = MPIR_Sched_barrier(s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
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
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                mpi_errno = MPIR_Type_commit_impl(&tmp_type);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);

                mpi_errno = MPIR_Sched_send(MPI_BOTTOM, 1, tmp_type, dst, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* this "premature" free is safe b/c the sched holds an actual ref to keep it alive */
                MPIR_Type_free_impl(&tmp_type);
            }

            break;
        }
        mask <<= 1;
    }

    if ((rank == root) && root && (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) && copy_blks) {
        /* reorder and copy from tmp_buf into recvbuf */
        /* FIXME why are there two copies here? */
        mpi_errno = MPIR_Sched_copy(tmp_buf, nbytes * (comm_size - copy_offset), MPI_BYTE,
                                    ((char *) recvbuf + extent * recvcount * copy_offset),
                                    recvcount * (comm_size - copy_offset), recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_copy((char *) tmp_buf + nbytes * (comm_size - copy_offset),
                                    nbytes * (copy_blks - comm_size + copy_offset), MPI_BYTE,
                                    recvbuf, recvcount * (copy_blks - comm_size + copy_offset),
                                    recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
