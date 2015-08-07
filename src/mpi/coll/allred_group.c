/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

int MPIR_Allreduce_group_intra(void *sendbuf, void *recvbuf, int count,
                               MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr,
                               MPID_Group *group_ptr, int tag, MPIR_Errflag_t *errflag);

/* Local utility macro: takes an two args and sets lvalue cr_ equal to the rank
 * in comm_ptr corresponding to rvalue gr_ */
#define to_comm_rank(cr_, gr_)                                                                                \
    do {                                                                                                      \
        int gr_tmp_ = (gr_);                                                                                  \
        mpi_errno = MPIR_Group_translate_ranks_impl(group_ptr, 1, &(gr_tmp_), comm_ptr->local_group, &(cr_)); \
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);                                                               \
        MPIU_Assert((cr_) != MPI_UNDEFINED);                                                                  \
    } while (0)

#undef FUNCNAME
#define FUNCNAME MPIR_Allreduce_group_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce_group_intra(void *sendbuf, void *recvbuf, int count,
                               MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr,
                               MPID_Group *group_ptr, int tag, MPIR_Errflag_t *errflag)
{
    MPI_Aint type_size;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    /* newrank is a rank in group_ptr */
    int mask, dst, is_commutative, pof2, newrank, rem, newdst, i,
        send_idx, recv_idx, last_idx, send_cnt, recv_cnt, *cnts, *disps;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf;
    int group_rank, group_size;
    int cdst, csrc;
    MPIU_CHKLMEM_DECL(3);

#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        MPIU_Assert_fmt_msg(FALSE,("heterogeneous support for Allreduce_group_intra not yet implemented"));
#endif

    /* homogeneous case */

    group_rank = group_ptr->rank;
    group_size = group_ptr->size;
    MPIR_ERR_CHKANDJUMP(group_rank == MPI_UNDEFINED, mpi_errno, MPI_ERR_OTHER, "**rank");

    is_commutative = MPIR_Op_is_commutative(op);

    /* need to allocate temporary buffer to store incoming data*/
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPID_Datatype_get_extent_macro(datatype, extent);

    MPIU_Ensure_Aint_fits_in_pointer(count * MPIR_MAX(extent, true_extent));
    MPIU_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPIR_MAX(extent,true_extent)), mpi_errno, "temporary buffer");

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                   count, datatype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPID_Datatype_get_size_macro(datatype, type_size);

    /* find nearest power-of-two less than or equal to comm_size */
    pof2 = 1;
    while (pof2 <= group_size) pof2 <<= 1;
    pof2 >>=1;

    rem = group_size - pof2;

    /* In the non-power-of-two case, all even-numbered
       processes of rank < 2*rem send their data to
       (rank+1). These even-numbered processes no longer
       participate in the algorithm until the very end. The
       remaining processes form a nice power-of-two. */

    if (group_rank < 2*rem) {
        if (group_rank % 2 == 0) { /* even */
            to_comm_rank(cdst, group_rank+1);
            mpi_errno = MPIC_Send(recvbuf, count,
                                     datatype, cdst,
                                     tag, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* temporarily set the rank to -1 so that this
               process does not pariticipate in recursive
               doubling */
            newrank = -1;
        }
        else { /* odd */
            to_comm_rank(csrc, group_rank-1);
            mpi_errno = MPIC_Recv(tmp_buf, count,
                                     datatype, csrc,
                                     tag, comm_ptr,
                                     MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* do the reduction on received data. since the
               ordering is right, it doesn't matter whether
               the operation is commutative or not. */
            mpi_errno = MPIR_Reduce_local_impl(tmp_buf, recvbuf, count, datatype, op);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            /* change the rank */
            newrank = group_rank / 2;
        }
    }
    else  /* rank >= 2*rem */
        newrank = group_rank - rem;

    /* If op is user-defined or count is less than pof2, use
       recursive doubling algorithm. Otherwise do a reduce-scatter
       followed by allgather. (If op is user-defined,
       derived datatypes are allowed and the user could pass basic
       datatypes on one process and derived on another as long as
       the type maps are the same. Breaking up derived
       datatypes to do the reduce-scatter is tricky, therefore
       using recursive doubling in that case.) */

    if (newrank != -1) {
        if ((count*type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
            (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) ||
            (count < pof2))
        {
            /* use recursive doubling */
            mask = 0x1;
            while (mask < pof2) {
                newdst = newrank ^ mask;
                /* find real rank of dest */
                dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;
                to_comm_rank(cdst, dst);

                /* Send the most current data, which is in recvbuf. Recv
                   into tmp_buf */
                mpi_errno = MPIC_Sendrecv(recvbuf, count, datatype,
                                             cdst, tag, tmp_buf,
                                             count, datatype, cdst,
                                             tag, comm_ptr,
                                             MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                } else {

                    /* tmp_buf contains data received in this step.
                       recvbuf contains data accumulated so far */

                    if (is_commutative  || (dst < group_rank)) {
                        /* op is commutative OR the order is already right */
                        mpi_errno = MPIR_Reduce_local_impl(tmp_buf, recvbuf, count, datatype, op);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    }
                    else {
                        /* op is noncommutative and the order is not right */
                        mpi_errno = MPIR_Reduce_local_impl(recvbuf, tmp_buf, count, datatype, op);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                        /* copy result back into recvbuf */
                        mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                recvbuf, count, datatype);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    }
                }
                mask <<= 1;
            }
        }

        else {

            /* do a reduce-scatter followed by allgather */

            /* for the reduce-scatter, calculate the count that
               each process receives and the displacement within
               the buffer */

            MPIU_CHKLMEM_MALLOC(cnts, int *, pof2*sizeof(int), mpi_errno, "counts");
            MPIU_CHKLMEM_MALLOC(disps, int *, pof2*sizeof(int), mpi_errno, "displacements");

            for (i=0; i<(pof2-1); i++)
                cnts[i] = count/pof2;
            cnts[pof2-1] = count - (count/pof2)*(pof2-1);

            disps[0] = 0;
            for (i=1; i<pof2; i++)
                disps[i] = disps[i-1] + cnts[i-1];

            mask = 0x1;
            send_idx = recv_idx = 0;
            last_idx = pof2;
            while (mask < pof2) {
                newdst = newrank ^ mask;
                /* find real rank of dest */
                dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;
                to_comm_rank(cdst, dst);

                send_cnt = recv_cnt = 0;
                if (newrank < newdst) {
                    send_idx = recv_idx + pof2/(mask*2);
                    for (i=send_idx; i<last_idx; i++)
                        send_cnt += cnts[i];
                    for (i=recv_idx; i<send_idx; i++)
                        recv_cnt += cnts[i];
                }
                else {
                    recv_idx = send_idx + pof2/(mask*2);
                    for (i=send_idx; i<recv_idx; i++)
                        send_cnt += cnts[i];
                    for (i=recv_idx; i<last_idx; i++)
                        recv_cnt += cnts[i];
                }

                /* Send data from recvbuf. Recv into tmp_buf */
                mpi_errno = MPIC_Sendrecv((char *) recvbuf +
                                             disps[send_idx]*extent,
                                             send_cnt, datatype,
                                             cdst, tag,
                                             (char *) tmp_buf +
                                             disps[recv_idx]*extent,
                                             recv_cnt, datatype, cdst,
                                             tag, comm_ptr,
                                             MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                /* tmp_buf contains data received in this step.
                   recvbuf contains data accumulated so far */

                /* This algorithm is used only for predefined ops
                   and predefined ops are always commutative. */
                mpi_errno = MPIR_Reduce_local_impl(((char *) tmp_buf + disps[recv_idx]*extent),
                                                   ((char *) recvbuf + disps[recv_idx]*extent),
                                                   recv_cnt, datatype, op);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                /* update send_idx for next iteration */
                send_idx = recv_idx;
                mask <<= 1;

                /* update last_idx, but not in last iteration
                   because the value is needed in the allgather
                   step below. */
                if (mask < pof2)
                    last_idx = recv_idx + pof2/mask;
            }

            /* now do the allgather */

            mask >>= 1;
            while (mask > 0) {
                newdst = newrank ^ mask;
                /* find real rank of dest */
                dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;
                to_comm_rank(cdst, dst);

                send_cnt = recv_cnt = 0;
                if (newrank < newdst) {
                    /* update last_idx except on first iteration */
                    if (mask != pof2/2)
                        last_idx = last_idx + pof2/(mask*2);

                    recv_idx = send_idx + pof2/(mask*2);
                    for (i=send_idx; i<recv_idx; i++)
                        send_cnt += cnts[i];
                    for (i=recv_idx; i<last_idx; i++)
                        recv_cnt += cnts[i];
                }
                else {
                    recv_idx = send_idx - pof2/(mask*2);
                    for (i=send_idx; i<last_idx; i++)
                        send_cnt += cnts[i];
                    for (i=recv_idx; i<send_idx; i++)
                        recv_cnt += cnts[i];
                }

                mpi_errno = MPIC_Sendrecv((char *) recvbuf +
                                             disps[send_idx]*extent,
                                             send_cnt, datatype,
                                             cdst, tag,
                                             (char *) recvbuf +
                                             disps[recv_idx]*extent,
                                             recv_cnt, datatype, cdst,
                                             tag, comm_ptr,
                                             MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                if (newrank > newdst) send_idx = recv_idx;

                mask >>= 1;
            }
        }
    }

    /* In the non-power-of-two case, all odd-numbered
       processes of rank < 2*rem send the result to
       (rank-1), the ranks who didn't participate above. */
    if (group_rank < 2*rem) {
        if (group_rank % 2) { /* odd */
            to_comm_rank(cdst, group_rank-1);
            mpi_errno = MPIC_Send(recvbuf, count,
                                     datatype, cdst,
                                     tag, comm_ptr, errflag);
        }
        else { /* even */
            to_comm_rank(csrc, group_rank+1);
            mpi_errno = MPIC_Recv(recvbuf, count,
                                     datatype, csrc,
                                     tag, comm_ptr,
                                     MPI_STATUS_IGNORE, errflag);
        }
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return (mpi_errno);

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Allreduce_group
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce_group(void *sendbuf, void *recvbuf, int count,
                         MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr,
                         MPID_Group *group_ptr, int tag, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_CHKANDJUMP(comm_ptr->comm_kind != MPID_INTRACOMM, mpi_errno, MPI_ERR_OTHER, "**commnotintra");

    mpi_errno = MPIR_Allreduce_group_intra(sendbuf, recvbuf, count, datatype,
                                           op, comm_ptr, group_ptr, tag, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    fn_exit:
        return mpi_errno;
    fn_fail:
        goto fn_exit;
}
