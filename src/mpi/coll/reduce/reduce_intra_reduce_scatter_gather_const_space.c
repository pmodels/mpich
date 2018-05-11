/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* An implementation of Rabenseifner's reduce algorithm with constant space
   implementation (see http://www.hlrs.de/mpi/myreduce.html).

   In this implementation of Rabenseifner's reduce algorithm memory usage
   is optimized reducing to constant space complexity compared to the
   existing implementation. The communication pattern and the algorithm
   remains the same in both the implementations.

   This algorithm implements the reduce in two steps: first a
   reduce-scatter, followed by a gather to the root. A
   recursive-halving algorithm (beginning with processes that are
   distance 1 apart) is used for the reduce-scatter, and a binomial tree
   algorithm is used for the gather. The non-power-of-two case is
   handled by dropping to the nearest lower power-of-two: the first
   few odd-numbered processes send their data to their left neighbors
   (rank-1), and the reduce-scatter happens among the remaining
   power-of-two processes. If the root is one of the excluded
   processes, then after the reduce-scatter, rank 0 sends its result to
   the root and exits; the root now acts as rank 0 in the binomial tree
   algorithm for gather.

   For the power-of-two case, the cost for the reduce-scatter is
   lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma. The cost for the
   gather to root is lgp.alpha + n.((p-1)/p).beta. Therefore, the
   total cost is:
   Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma

   For the non-power-of-two case, assuming the root is not one of the
   odd-numbered processes that get excluded in the reduce-scatter,
   Cost = (2.floor(lgp)+1).alpha + (2.((p-1)/p) + 1).n.beta +
           n.(1+(p-1)/p).gamma
*/

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_intra_reduce_scatter_gather_const_space
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_intra_reduce_scatter_gather_const_space(const void *sendbuf,
                                                        void *recvbuf,
                                                        int count,
                                                        MPI_Datatype datatype,
                                                        MPI_Op op,
                                                        int root,
                                                        MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int comm_size, rank, type_size ATTRIBUTE((unused)), pof2, rem, newrank, rank_gather;
    int mask, i, j, newdst;
    int dst, newroot, newdst_tree_root, newroot_tree_root;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf;
    int lsize, rsize, sdisp, rdisp, rcnt, scnt, iter, niter, newcount;

    MPIR_CHKLMEM_DECL(2);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* set op_errno to 0. stored in perthread structure */
    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        per_thread->op_errno = 0;
    }

    /* Create a temporary buffer */

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    /* I think this is the worse case, so we can avoid an assert()
     * inside the for loop */
    /* should be buf+{this}? */
    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)),
                        mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    /* If I'm not the root, then my recvbuf may not be valid, therefore
     * I have to allocate a temporary one */
    if (rank != root) {
        MPIR_CHKLMEM_MALLOC(recvbuf, void *,
                            count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "receive buffer", MPL_MEM_BUFFER);
        recvbuf = (void *) ((char *) recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* find the number of steps in reduce scatter */
    pof2 = 1;
    niter = -1;
    while (pof2 <= comm_size) {
        niter++;
        pof2 <<= 1;
    }

    /* get nearest power-of-two less than or equal to comm_size */
    pof2 = comm_ptr->pof2;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN);
    MPIR_Assert(count >= pof2);
#endif /* HAVE_ERROR_CHECKING */

    rem = comm_size - pof2;

    /* In the non-power-of-two case, all odd-numbered
     * processes of rank < 2*rem send their data to
     * (rank-1). These odd-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two.
     *
     * Note that in MPI_Allreduce we have the even-numbered processes
     * send data to odd-numbered processes. That is better for
     * non-commutative operations because it doesn't require a
     * buffer copy. However, for MPI_Reduce, the most common case
     * is commutative operations with root=0. Therefore we want
     * even-numbered processes to participate the computation for
     * the root=0 case, in order to avoid an extra send-to-root
     * communication after the reduce-scatter. In MPI_Allreduce it
     * doesn't matter because all processes must get the result. */

    if (rank < 2 * rem) {
        if (rank % 2 != 0) {    /* odd */
            mpi_errno = MPIC_Send(recvbuf, count,
                                  datatype, rank - 1, MPIR_REDUCE_TAG, comm_ptr, errflag);
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
        } else {        /* even */
            mpi_errno = MPIC_Recv(tmp_buf, count,
                                  datatype, rank + 1,
                                  MPIR_REDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* do the reduction on received data. */
            /* This algorithm is used only for predefined ops
             * and predefined ops are always commutative. */
            mpi_errno = MPIR_Reduce_local(tmp_buf, recvbuf, count, datatype, op);
            /* change the rank */
            newrank = rank / 2;
        }
    } else      /* rank >= 2*rem */
        newrank = rank - rem;

    if (newrank != -1) {
        newcount = count;
        sdisp = rdisp = 0;
        iter = 1;

        for (mask = 1; mask < pof2; mask <<= 1) {
            newdst = newrank ^ mask;
            /* Find real rank of the dest */
            dst = (newdst < rem) ? newdst * 2 : newdst + rem;
            if ((rank < dst)) {
                /* Recv: into the left half of the current window
                 * Send: the right half of the window to the pee
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
                                      MPIR_REDUCE_TAG,
                                      (char *) tmp_buf + rdisp * extent,
                                      rcnt, datatype, dst,
                                      MPIR_REDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
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

            if (iter < niter) {
                sdisp = rdisp;
                newcount = rcnt;
                iter++;
            }
        }
    }

    /* Set root process to gather at root */
    if (root < 2 * rem) {
        if (root % 2 != 0) {
            if (rank == root) {
                rdisp = 0;
                newcount = count;

                for (mask = 1; mask < pof2; mask *= 2) {
                    rcnt = newcount / 2;
                    scnt = newcount - rcnt;
                    sdisp = rcnt;
                    newcount = newcount / 2;
                }
                mpi_errno = MPIC_Recv(recvbuf, sdisp, datatype,
                                      0, MPIR_REDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                newrank = 0;
            } else if (newrank == 0) {  /* send */
                mpi_errno = MPIC_Send(recvbuf, sdisp, datatype,
                                      root, MPIR_REDUCE_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                newrank = -1;
            }
            newroot = 0;
        } else
            newroot = root / 2;
    } else
        newroot = root - rem;

    /* Gather */
    if (newrank != -1) {
        mask = pof2 >> 1;
        j = niter - 1;

        while (mask > 0) {
            newcount = count;
            pof2 = 1;
            sdisp = rdisp = rcnt = 0;
            /* Computing the disp and cnt for Gather */
            for (i = 1; i <= mask; i <<= 1) {
                newdst = newrank ^ pof2;
                dst = (newdst < rem) ? newdst * 2 : newdst + rem;
                sdisp = rdisp;

                if ((rank == root) && (newrank == 0))
                    rank_gather = newrank;
                else
                    rank_gather = rank;
                if ((rank_gather < dst)) {
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

            newdst = newrank ^ mask;

            dst = (newdst < rem) ? newdst * 2 : newdst + rem;
            if ((newdst == 0) && (root < 2 * rem) && (root % 2 != 0))
                dst = root;

            /* if the root of newdst's half of the tree is the
             * same as the root of newroot's half of the tree, send to
             * newdst and exit, else receive from newdst. */
            newdst_tree_root = newdst >> j;
            newdst_tree_root <<= j;

            newroot_tree_root = newroot >> j;
            newroot_tree_root <<= j;

            if (newdst_tree_root == newroot_tree_root) {
                /* Send data from recvbuf */
                mpi_errno = MPIC_Send((char *) recvbuf +
                                      rdisp * extent,
                                      rcnt, datatype, dst, MPIR_REDUCE_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                break;
            } else {
                /* Recv and continue */
                mpi_errno = MPIC_Recv((char *) recvbuf +
                                      sdisp * extent,
                                      scnt, datatype, dst,
                                      MPIR_REDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            mask >>= 1;
            j--;

        }
    }

    /* FIXME does this need to be checked after each uop invocation for
     * predefined operators? */
    /* --BEGIN ERROR HANDLING-- */
    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        if (per_thread->op_errno) {
            mpi_errno = per_thread->op_errno;
            goto fn_fail;
        }
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
