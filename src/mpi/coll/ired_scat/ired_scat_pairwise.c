/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "../collutil.h"

/* A pairwise exchange algorithm for MPI_Ireduce_scatter.  Requires a
 * commutative op and is intended for use with large messages. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_rec_pairwise_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_pairwise_sched(const void *sendbuf, void *recvbuf, const int recvcounts[],
                                  MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                                  MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int   rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb;
    int  *disps;
    void *tmp_recvbuf;
    int src, dst;
    int is_commutative;
    int total_count;
    MPIR_SCHED_CHKPMEM_DECL(2);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    is_commutative = MPIR_Op_is_commutative(op);
    MPIR_Assert(is_commutative);

    MPIR_SCHED_CHKPMEM_MALLOC(disps, int *, comm_size * sizeof(int), mpi_errno, "disps", MPL_MEM_BUFFER);

    total_count = 0;
    for (i=0; i<comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcounts[i];
    }

    if (total_count == 0) {
        goto fn_exit;
    }
    /* total_count*extent eventually gets malloced. it isn't added to
     * a user-passed in buffer */
    MPIR_Ensure_Aint_fits_in_pointer(total_count * MPL_MAX(true_extent, extent));

    if (sendbuf != MPI_IN_PLACE) {
        /* copy local data into recvbuf */
        mpi_errno = MPIR_Sched_copy(((char *)sendbuf+disps[rank]*extent),
                                    recvcounts[rank], datatype,
                                    recvbuf, recvcounts[rank], datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* allocate temporary buffer to store incoming data */
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_recvbuf, void *, recvcounts[rank]*(MPL_MAX(true_extent,extent))+1, mpi_errno, "tmp_recvbuf", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *)((char*)tmp_recvbuf - true_lb);

    for (i=1; i<comm_size; i++) {
        src = (rank - i + comm_size) % comm_size;
        dst = (rank + i) % comm_size;

        /* send the data that dst needs. recv data that this process
           needs from src into tmp_recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Sched_send(((char *)sendbuf+disps[dst]*extent),
                                        recvcounts[dst], datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else {
            mpi_errno = MPIR_Sched_send(((char *)recvbuf+disps[dst]*extent),
                                        recvcounts[dst], datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        mpi_errno = MPIR_Sched_recv(tmp_recvbuf, recvcounts[rank], datatype, src, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        /* FIXME does this algorithm actually work correctly for noncommutative ops?
         * If so, relax restriction in assert and comments... */
        if (is_commutative || (src < rank)) {
            if (sendbuf != MPI_IN_PLACE) {
                mpi_errno = MPIR_Sched_reduce(tmp_recvbuf, recvbuf, recvcounts[rank], datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            else {
                mpi_errno = MPIR_Sched_reduce(tmp_recvbuf, ((char *)recvbuf+disps[rank]*extent),
                                              recvcounts[rank], datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                /* We can't store the result at the beginning of
                   recvbuf right here because there is useful data there that
                   other process/processes need.  At the end we will copy back
                   the result to the beginning of recvbuf. */
            }
            MPIR_SCHED_BARRIER(s);
        }
        else {
            if (sendbuf != MPI_IN_PLACE) {
                mpi_errno = MPIR_Sched_reduce(recvbuf, tmp_recvbuf, recvcounts[rank], datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
                /* copy result back into recvbuf */
                mpi_errno = MPIR_Sched_copy(tmp_recvbuf, recvcounts[rank], datatype,
                                            recvbuf, recvcounts[rank], datatype, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            else {
                mpi_errno = MPIR_Sched_reduce(((char *)recvbuf+disps[rank]*extent),
                                              tmp_recvbuf, recvcounts[rank], datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
                /* copy result back into recvbuf */
                mpi_errno = MPIR_Sched_copy(tmp_recvbuf, recvcounts[rank], datatype,
                                            ((char *)recvbuf + disps[rank]*extent),
                                            recvcounts[rank], datatype, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            MPIR_SCHED_BARRIER(s);
        }
    }

    /* if MPI_IN_PLACE, move output data to the beginning of
       recvbuf. already done for rank 0. */
    if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
        mpi_errno = MPIR_Sched_copy(((char *)recvbuf + disps[rank]*extent),
                                    recvcounts[rank], datatype,
                                    recvbuf, recvcounts[rank], datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

