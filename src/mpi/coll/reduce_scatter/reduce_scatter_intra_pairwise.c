/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_intra_pairwise
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/* Algorithm: Pairwise Exchange
 *
 * For commutative operations and very long messages we use This is a pairwise
 * exchange algorithm similar to the one used in MPI_Alltoall. At step i, each
 * process sends n/p amount of data to (rank+i) and receives n/p amount of data
 * from (rank-i).
 */
int MPIR_Reduce_scatter_intra_pairwise(const void *sendbuf, void *recvbuf, const int recvcounts[],
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                       MPIR_Errflag_t * errflag)
{
    int rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb;
    int *disps;
    void *tmp_recvbuf;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int total_count, src, dst;
    MPIR_CHKLMEM_DECL(5);

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

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);


#ifdef HAVE_ERROR_CHECKING
    {
        int is_commutative;
        is_commutative = MPIR_Op_is_commutative(op);
        MPIR_Assert(is_commutative);
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_CHKLMEM_MALLOC(disps, int *, comm_size * sizeof(int), mpi_errno, "disps", MPL_MEM_BUFFER);

    total_count = 0;
    for (i = 0; i < comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcounts[i];
    }

    if (total_count == 0) {
        goto fn_exit;
    }

    /* total_count*extent eventually gets malloced. it isn't added to
     * a user-passed in buffer */
    MPIR_Ensure_Aint_fits_in_pointer(total_count * MPL_MAX(true_extent, extent));


    /* commutative and long message, or noncommutative and long message.
     * use (p-1) pairwise exchanges */

    if (sendbuf != MPI_IN_PLACE) {
        /* copy local data into recvbuf */
        mpi_errno = MPIR_Localcopy(((char *) sendbuf + disps[rank] * extent),
                                   recvcounts[rank], datatype, recvbuf, recvcounts[rank], datatype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    /* allocate temporary buffer to store incoming data */
    MPIR_CHKLMEM_MALLOC(tmp_recvbuf, void *, recvcounts[rank] * (MPL_MAX(true_extent, extent)) + 1,
                        mpi_errno, "tmp_recvbuf", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *) ((char *) tmp_recvbuf - true_lb);

    for (i = 1; i < comm_size; i++) {
        src = (rank - i + comm_size) % comm_size;
        dst = (rank + i) % comm_size;

        /* send the data that dst needs. recv data that this process
         * needs from src into tmp_recvbuf */
        if (sendbuf != MPI_IN_PLACE)
            mpi_errno = MPIC_Sendrecv(((char *) sendbuf + disps[dst] * extent),
                                      recvcounts[dst], datatype, dst,
                                      MPIR_REDUCE_SCATTER_TAG, tmp_recvbuf,
                                      recvcounts[rank], datatype, src,
                                      MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                      MPI_STATUS_IGNORE, errflag);
        else
            mpi_errno = MPIC_Sendrecv(((char *) recvbuf + disps[dst] * extent),
                                      recvcounts[dst], datatype, dst,
                                      MPIR_REDUCE_SCATTER_TAG, tmp_recvbuf,
                                      recvcounts[rank], datatype, src,
                                      MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                      MPI_STATUS_IGNORE, errflag);

        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Reduce_local(tmp_recvbuf, recvbuf, recvcounts[rank], datatype, op);
        } else {
            mpi_errno = MPIR_Reduce_local(tmp_recvbuf, ((char *) recvbuf + disps[rank] * extent),
                                          recvcounts[rank], datatype, op);
            /* we can't store the result at the beginning of
             * recvbuf right here because there is useful data
             * there that other process/processes need. at the
             * end, we will copy back the result to the
             * beginning of recvbuf. */
        }
    }

    /* if MPI_IN_PLACE, move output data to the beginning of
     * recvbuf. already done for rank 0. */
    if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
        mpi_errno = MPIR_Localcopy(((char *) recvbuf +
                                    disps[rank] * extent),
                                   recvcounts[rank], datatype, recvbuf, recvcounts[rank], datatype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();

    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        if (per_thread->op_errno)
            mpi_errno = per_thread->op_errno;
    }

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
