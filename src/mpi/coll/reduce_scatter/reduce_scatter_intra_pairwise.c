/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


/* Algorithm: Pairwise Exchange
 *
 * For commutative operations and very long messages we use This is a pairwise
 * exchange algorithm similar to the one used in MPI_Alltoall. At step i, each
 * process sends n/p amount of data to (rank+i) and receives n/p amount of data
 * from (rank-i).
 */
int MPIR_Reduce_scatter_intra_pairwise(const void *sendbuf, void *recvbuf,
                                       const MPI_Aint recvcounts[], MPI_Datatype datatype,
                                       MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag)
{
    int rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb;
    MPI_Aint *disps;
    void *tmp_recvbuf;
    int mpi_errno = MPI_SUCCESS;
    int src, dst;
    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);


#ifdef HAVE_ERROR_CHECKING
    {
        int is_commutative;
        is_commutative = MPIR_Op_is_commutative(op);
        MPIR_Assertp(is_commutative);
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_CHKLMEM_MALLOC(disps, comm_size * sizeof(MPI_Aint));

    MPI_Aint total_count;
    total_count = 0;
    for (i = 0; i < comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcounts[i];
    }

    if (total_count == 0) {
        goto fn_exit;
    }

    /* commutative and long message, or noncommutative and long message.
     * use (p-1) pairwise exchanges */

    if (sendbuf != MPI_IN_PLACE) {
        /* copy local data into recvbuf */
        mpi_errno = MPIR_Localcopy(((char *) sendbuf + disps[rank] * extent),
                                   recvcounts[rank], datatype, recvbuf, recvcounts[rank], datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* allocate temporary buffer to store incoming data */
    MPIR_CHKLMEM_MALLOC(tmp_recvbuf, recvcounts[rank] * (MPL_MAX(true_extent, extent)) + 1);
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

        MPIR_ERR_CHECK(mpi_errno);

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
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* if MPI_IN_PLACE, move output data to the beginning of
     * recvbuf. already done for rank 0. */
    if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
        mpi_errno = MPIR_Localcopy(((char *) recvbuf +
                                    disps[rank] * extent),
                                   recvcounts[rank], datatype, recvbuf, recvcounts[rank], datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
