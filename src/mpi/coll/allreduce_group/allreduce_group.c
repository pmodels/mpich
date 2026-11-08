/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Local utility macro: takes an two args and sets lvalue cr_ equal to the rank
 * in comm_ptr corresponding to rvalue gr_ */
#define to_comm_rank(cr_, gr_)                                                                                \
    do {                                                                                                      \
        int gr_tmp_ = (gr_);                                                                                  \
        mpi_errno = MPIR_Group_translate_ranks_impl(group_ptr, 1, &(gr_tmp_), comm_ptr->local_group, &(cr_)); \
        MPIR_ERR_CHECK(mpi_errno);                                                                            \
        MPIR_Assert((cr_) != MPI_UNDEFINED);                                                                  \
    } while (0)

int MPII_Allreduce_group_intra(void *sendbuf, void *recvbuf, MPI_Aint count,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                               MPIR_Group * group_ptr, int tag, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    /* newrank is a rank in group_ptr */
    int mask, dst, is_commutative, pof2, newrank, rem, newdst;
    void *tmp_buf;
    int group_rank, group_size;
    int cdst, csrc;
    MPIR_CHKLMEM_DECL();

    group_rank = group_ptr->rank;
    group_size = group_ptr->size;
    MPIR_ERR_CHKANDJUMP(group_rank == MPI_UNDEFINED, mpi_errno, MPI_ERR_OTHER, "**rank");

    is_commutative = MPIR_Op_is_commutative(op);

    /* need to allocate temporary buffer to store incoming data */
    MPI_Aint buf_shift, buf_size;
    MPIR_Datatype_get_buffer_span(datatype, count, &buf_shift, &buf_size);
    MPIR_CHKLMEM_MALLOC(tmp_buf, buf_size);

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - buf_shift);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* get nearest power-of-two less than or equal to comm_size */
    pof2 = MPL_pof2(group_size);

    rem = group_size - pof2;

    /* In the non-power-of-two case, all even-numbered
     * processes of rank < 2*rem send their data to
     * (rank+1). These even-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two. */

    if (group_rank < 2 * rem) {
        if (group_rank % 2 == 0) {      /* even */
            to_comm_rank(cdst, group_rank + 1);
            mpi_errno = MPIC_Send(recvbuf, count, datatype, cdst, tag, comm_ptr, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);

            /* temporarily set the rank to -1 so that this
             * process does not participate in recursive
             * doubling */
            newrank = -1;
        } else {        /* odd */
            to_comm_rank(csrc, group_rank - 1);
            mpi_errno = MPIC_Recv(tmp_buf, count, datatype, csrc, tag, comm_ptr, MPI_STATUS_IGNORE);
            MPIR_ERR_CHECK(mpi_errno);

            /* do the reduction on received data. since the
             * ordering is right, it doesn't matter whether
             * the operation is commutative or not. */
            mpi_errno = MPIR_Reduce_local(tmp_buf, recvbuf, count, datatype, op);
            MPIR_ERR_CHECK(mpi_errno);

            /* change the rank */
            newrank = group_rank / 2;
        }
    } else      /* rank >= 2*rem */
        newrank = group_rank - rem;

    if (newrank != -1) {
        mask = 0x1;
        while (mask < pof2) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;
            to_comm_rank(cdst, dst);

            /* Send the most current data, which is in recvbuf. Recv
             * into tmp_buf */
            mpi_errno = MPIC_Sendrecv(recvbuf, count, datatype,
                                      cdst, tag, tmp_buf,
                                      count, datatype, cdst,
                                      tag, comm_ptr, MPI_STATUS_IGNORE, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
            if (!mpi_errno) {
                /* tmp_buf contains data received in this step.
                 * recvbuf contains data accumulated so far */

                if (is_commutative || (dst < group_rank)) {
                    /* op is commutative OR the order is already right */
                    mpi_errno = MPIR_Reduce_local(tmp_buf, recvbuf, count, datatype, op);
                    MPIR_ERR_CHECK(mpi_errno);
                } else {
                    /* op is noncommutative and the order is not right */
                    mpi_errno = MPIR_Reduce_local(recvbuf, tmp_buf, count, datatype, op);
                    MPIR_ERR_CHECK(mpi_errno);

                    /* copy result back into recvbuf */
                    mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype, recvbuf, count, datatype);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            }
            mask <<= 1;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
     * processes of rank < 2*rem send the result to
     * (rank-1), the ranks who didn't participate above. */
    if (group_rank < 2 * rem) {
        if (group_rank % 2) {   /* odd */
            to_comm_rank(cdst, group_rank - 1);
            mpi_errno = MPIC_Send(recvbuf, count, datatype, cdst, tag, comm_ptr, coll_attr);
        } else {        /* even */
            to_comm_rank(csrc, group_rank + 1);
            mpi_errno = MPIC_Recv(recvbuf, count, datatype, csrc, tag, comm_ptr, MPI_STATUS_IGNORE);
        }
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_Allreduce_group(void *sendbuf, void *recvbuf, MPI_Aint count,
                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                         MPIR_Group * group_ptr, int tag, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_ERR_CHKANDJUMP(comm_ptr->comm_kind != MPIR_COMM_KIND__INTRACOMM, mpi_errno, MPI_ERR_OTHER,
                        "**commnotintra");

    mpi_errno = MPII_Allreduce_group_intra(sendbuf, recvbuf, count, datatype,
                                           op, comm_ptr, group_ptr, tag, coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
