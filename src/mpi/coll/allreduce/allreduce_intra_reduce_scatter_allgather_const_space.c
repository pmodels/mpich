/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: Rabenseifner's Algorithm
 * An implementation of Rabenseifner's reduce algorithm with constant space
 *   implementation (see http://www.hlrs.de/mpi/myreduce.html).
 *
 * Restrictions: Built-in ops only
 *
 * In this implementation of Rabenseifner's reduce algorithm memory usage
 * is optimized reducing to constant space complexity compared to the
 * existing implementation. The communication pattern and the algorithm
 * remains the same in both the implementations.
 *
 * This algorithm implements the allreduce in two steps: first a
 * reduce-scatter, followed by an allgather. A recursive-halving algorithm
 * (beginning with processes that are distance 1 apart) is used for the
 * reduce-scatter, and a recursive doubling algorithm is used for the
 * allgather. The non-power-of-two case is handled by dropping to the nearest
 * lower power-of-two: the first few even-numbered processes send their data to
 * their right neighbors (rank+1), and the reduce-scatter and allgather happen
 * among the remaining power-of-two processes. At the end, the first few
 * even-numbered processes get the result from their right neighbors.
 *
 * For the power-of-two case, the cost for the reduce-scatter is:
 *
 * lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma.
 *
 * The cost for the allgather:
 *
 * lgp.alpha +.n.((p-1)/p).beta
 *
 * Therefore, the total cost is:
 *
 * Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma
 *
 * For the non-power-of-two case:
 *
 * Cost = (2.floor(lgp)+2).alpha + (2.((p-1)/p) + 2).n.beta + n.(1+(p-1)/p).gamma
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Allreduce_intra_reduce_scatter_allgather_const_space
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce_intra_reduce_scatter_allgather_const_space(const void *sendbuf,
                                                              void *recvbuf,
                                                              int count,
                                                              MPI_Datatype datatype,
                                                              MPI_Op op,
                                                              MPIR_Comm * comm_ptr,
                                                              MPIR_Errflag_t * errflag)
{
    MPIR_CHKLMEM_DECL(2);
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int mask, dst, pof2, newrank, rem, newdst, i, niter, newcount, scnt, rcnt, sdisp, rdisp;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* need to allocate temporary buffer to store incoming data */
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));
    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)), mpi_errno,
                        "temporary buffer", MPL_MEM_BUFFER);

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    /* get nearest power-of-two less than or equal to comm_size */
    pof2 = comm_ptr->pof2;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN);
    MPIR_Assert(count >= pof2);
#endif /* HAVE_ERROR_CHECKING */

    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
     * processes of rank < 2*rem send their data to
     * (rank+1). These even-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two. */

    if (rank < 2 * rem) {
        if (rank % 2 == 0) {    /* even */
            mpi_errno = MPIC_Send(recvbuf, count,
                                  datatype, rank + 1, MPIR_ALLREDUCE_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* temporarily set the rank to -1 so that this
             * process does not pariticipate in recursive
             * doubling */
            newrank = -1;
        } else {        /* odd */
            mpi_errno = MPIC_Recv(tmp_buf, count,
                                  datatype, rank - 1,
                                  MPIR_ALLREDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* do the reduction on received data. since the
             * ordering is right, it doesn't matter whether
             * the operation is commutative or not. */
            mpi_errno = MPIR_Reduce_local(tmp_buf, recvbuf, count, datatype, op);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            /* change the rank */
            newrank = rank / 2;
        }
    } else      /* rank >= 2*rem */
        newrank = rank - rem;

    /* If op is user-defined or count is less than pof2, use
     * recursive doubling algorithm. Otherwise do a reduce-scatter
     * followed by allgather. (If op is user-defined,
     * derived datatypes are allowed and the user could pass basic
     * datatypes on one process and derived on another as long as
     * the type maps are the same. Breaking up derived
     * datatypes to do the reduce-scatter is tricky, therefore
     * using recursive doubling in that case.) */

    if (newrank != -1) {
        newcount = count;
        sdisp = rdisp = 0;

        for (mask = 1; mask < pof2; mask <<= 1) {
            newdst = newrank ^ mask;
            /* Find real rank of the dest */
            dst = (newdst < rem) ? newdst * 2 : newdst + rem;
            if ((rank < dst)) {
                /* Recv: into the left half of the current window
                 * Send: the right half of the window to the peer
                 * (perform reduce on the left half of the current window) */
                rcnt = newcount / 2;
                scnt = newcount - rcnt;
                sdisp = rdisp + rcnt;
            } else {
                /* Recv into the right half of the current window
                 * Send the left half of the window to the peer
                 * (perform reduce on the right half of the current window) */
                scnt = newcount / 2;
                rcnt = newcount - scnt;
                rdisp = sdisp + scnt;
            }
            mpi_errno = MPIC_Sendrecv((char *) recvbuf + sdisp * extent,
                                      scnt, datatype, dst,
                                      MPIR_ALLREDUCE_TAG,
                                      (char *) tmp_buf + rdisp * extent,
                                      rcnt, datatype, dst,
                                      MPIR_ALLREDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            /* Local-reduce */
            mpi_errno = MPIR_Reduce_local((char *) tmp_buf +
                                          rdisp * extent,
                                          (char *) recvbuf + rdisp * extent, rcnt, datatype, op);

            sdisp = rdisp;
            newcount = rcnt;
        }
    }

    /* Gather */
    if (newrank != -1) {
        mask = pof2 >> 1;

        while (mask > 0) {
            newcount = count;
            pof2 = 1;
            sdisp = rdisp = rcnt = 0;
            /* Computing the disp and cnt for Gather */
            for (i = 1; i <= mask; i <<= 1) {
                newdst = newrank ^ pof2;
                dst = (newdst < rem) ? newdst * 2 : newdst + rem;
                sdisp = rdisp;

                if ((rank < dst)) {
                    rcnt = newcount / 2;
                    scnt = newcount - rcnt;
                    sdisp = rdisp + rcnt;
                } else {
                    scnt = newcount / 2;
                    rcnt = newcount - scnt;
                    rdisp = sdisp + scnt;
                }
                newcount = rcnt;
                pof2 <<= 1;
            }
            mpi_errno = MPIC_Sendrecv((char *) recvbuf +
                                      rdisp * extent,
                                      rcnt, datatype,
                                      dst, MPIR_ALLREDUCE_TAG,
                                      (char *) recvbuf +
                                      sdisp * extent,
                                      scnt, datatype, dst,
                                      MPIR_ALLREDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            mask >>= 1;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
     * processes of rank < 2*rem send the result to
     * (rank-1), the ranks who didn't participate above. */
    if (rank < 2 * rem) {
        if (rank % 2)   /* odd */
            mpi_errno = MPIC_Send(recvbuf, count,
                                  datatype, rank - 1, MPIR_ALLREDUCE_TAG, comm_ptr, errflag);
        else    /* even */
            mpi_errno = MPIC_Recv(recvbuf, count,
                                  datatype, rank + 1,
                                  MPIR_ALLREDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
