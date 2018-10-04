/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* An implementation of Rabenseifner's reduce algorithm (see
   http://www.hlrs.de/mpi/myreduce.html).

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
#define FUNCNAME MPIR_Reduce_intra_reduce_scatter_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_intra_reduce_scatter_gather(const void *sendbuf,
                                            void *recvbuf,
                                            int count,
                                            MPI_Datatype datatype,
                                            MPI_Op op,
                                            int root,
                                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int comm_size, rank, type_size ATTRIBUTE((unused)), pof2, rem, newrank;
    int mask, *cnts, *disps, i, j, send_idx = 0;
    int recv_idx, last_idx = 0, newdst;
    int dst, send_cnt, recv_cnt, newroot, newdst_tree_root, newroot_tree_root;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf;

    MPIR_CHKLMEM_DECL(4);

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

    /* for the reduce-scatter, calculate the count that
     * each process receives and the displacement within
     * the buffer */

    /* We allocate these arrays on all processes, even if newrank=-1,
     * because if root is one of the excluded processes, we will
     * need them on the root later on below. */
    MPIR_CHKLMEM_MALLOC(cnts, int *, pof2 * sizeof(int), mpi_errno, "counts", MPL_MEM_BUFFER);
    MPIR_CHKLMEM_MALLOC(disps, int *, pof2 * sizeof(int), mpi_errno, "displacements",
                        MPL_MEM_BUFFER);

    if (newrank != -1) {
        for (i = 0; i < pof2; i++)
            cnts[i] = count / pof2;
        if ((count % pof2) > 0) {
            for (i = 0; i < (count % pof2); i++)
                cnts[i] += 1;
        }

        if (pof2)
            disps[0] = 0;
        for (i = 1; i < pof2; i++)
            disps[i] = disps[i - 1] + cnts[i - 1];

        mask = 0x1;
        send_idx = recv_idx = 0;
        last_idx = pof2;
        while (mask < pof2) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst * 2 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                send_idx = recv_idx + pof2 / (mask * 2);
                for (i = send_idx; i < last_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < send_idx; i++)
                    recv_cnt += cnts[i];
            } else {
                recv_idx = send_idx + pof2 / (mask * 2);
                for (i = send_idx; i < recv_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < last_idx; i++)
                    recv_cnt += cnts[i];
            }

/*                    printf("Rank %d, send_idx %d, recv_idx %d, send_cnt %d, recv_cnt %d, last_idx %d\n", newrank, send_idx, recv_idx,
                  send_cnt, recv_cnt, last_idx);
*/
            /* Send data from recvbuf. Recv into tmp_buf */
            mpi_errno = MPIC_Sendrecv((char *) recvbuf +
                                      disps[send_idx] * extent,
                                      send_cnt, datatype,
                                      dst, MPIR_REDUCE_TAG,
                                      (char *) tmp_buf +
                                      disps[recv_idx] * extent,
                                      recv_cnt, datatype, dst,
                                      MPIR_REDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* tmp_buf contains data received in this step.
             * recvbuf contains data accumulated so far */

            /* This algorithm is used only for predefined ops
             * and predefined ops are always commutative. */
            mpi_errno = MPIR_Reduce_local((char *) tmp_buf + disps[recv_idx] * extent,
                                          (char *) recvbuf + disps[recv_idx] * extent,
                                          recv_cnt, datatype, op);
            /* update send_idx for next iteration */
            send_idx = recv_idx;
            mask <<= 1;

            /* update last_idx, but not in last iteration
             * because the value is needed in the gather
             * step below. */
            if (mask < pof2)
                last_idx = recv_idx + pof2 / mask;
        }
    }

    /* now do the gather to root */

    /* Is root one of the processes that was excluded from the
     * computation above? If so, send data from newrank=0 to
     * the root and have root take on the role of newrank = 0 */

    if (root < 2 * rem) {
        if (root % 2 != 0) {
            if (rank == root) { /* recv */
                /* initialize the arrays that weren't initialized */
                for (i = 0; i < pof2; i++)
                    cnts[i] = count / pof2;
                if ((count % pof2) > 0) {
                    for (i = 0; i < (count % pof2); i++)
                        cnts[i] += 1;
                }

                if (pof2)
                    disps[0] = 0;
                for (i = 1; i < pof2; i++)
                    disps[i] = disps[i - 1] + cnts[i - 1];

                mpi_errno = MPIC_Recv(recvbuf, cnts[0], datatype,
                                      0, MPIR_REDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                newrank = 0;
                send_idx = 0;
                last_idx = 2;
            } else if (newrank == 0) {  /* send */
                mpi_errno = MPIC_Send(recvbuf, cnts[0], datatype,
                                      root, MPIR_REDUCE_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
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

    if (newrank != -1) {
        j = 0;
        mask = 0x1;
        while (mask < pof2) {
            mask <<= 1;
            j++;
        }
        mask >>= 1;
        j--;
        while (mask > 0) {
            newdst = newrank ^ mask;

            /* find real rank of dest */
            dst = (newdst < rem) ? newdst * 2 : newdst + rem;
            /* if root is playing the role of newdst=0, adjust for
             * it */
            if ((newdst == 0) && (root < 2 * rem) && (root % 2 != 0))
                dst = root;

            /* if the root of newdst's half of the tree is the
             * same as the root of newroot's half of the tree, send to
             * newdst and exit, else receive from newdst. */

            newdst_tree_root = newdst >> j;
            newdst_tree_root <<= j;

            newroot_tree_root = newroot >> j;
            newroot_tree_root <<= j;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                /* update last_idx except on first iteration */
                if (mask != pof2 / 2)
                    last_idx = last_idx + pof2 / (mask * 2);

                recv_idx = send_idx + pof2 / (mask * 2);
                for (i = send_idx; i < recv_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < last_idx; i++)
                    recv_cnt += cnts[i];
            } else {
                recv_idx = send_idx - pof2 / (mask * 2);
                for (i = send_idx; i < last_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < send_idx; i++)
                    recv_cnt += cnts[i];
            }

            if (newdst_tree_root == newroot_tree_root) {
                /* send and exit */
                /* printf("Rank %d, send_idx %d, send_cnt %d, last_idx %d\n", newrank, send_idx, send_cnt, last_idx);
                 * fflush(stdout); */
                /* Send data from recvbuf. Recv into tmp_buf */
                mpi_errno = MPIC_Send((char *) recvbuf +
                                      disps[send_idx] * extent,
                                      send_cnt, datatype, dst, MPIR_REDUCE_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                break;
            } else {
                /* recv and continue */
                /* printf("Rank %d, recv_idx %d, recv_cnt %d, last_idx %d\n", newrank, recv_idx, recv_cnt, last_idx);
                 * fflush(stdout); */
                mpi_errno = MPIC_Recv((char *) recvbuf +
                                      disps[recv_idx] * extent,
                                      recv_cnt, datatype, dst,
                                      MPIR_REDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }

            if (newrank > newdst)
                send_idx = recv_idx;

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
