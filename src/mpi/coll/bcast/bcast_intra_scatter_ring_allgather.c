/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "bcast.h"

/* Algorithm: Broadcast based on a scatter followed by an allgather.
 *
 * We first scatter the buffer using a binomial tree algorithm. This costs
 * lgp.alpha + n.((p-1)/p).beta
 * If the datatype is contiguous, we treat the data as bytes and
 * divide (scatter) it among processes by using ceiling division.
 * For the noncontiguous cases, we first pack the data into a
 * temporary buffer by using MPI_Pack, scatter it as bytes, and
 * unpack it after the allgather.
 *
 * We use a ring algorithm for the allgather, which takes p-1 steps.
 * This may perform better than recursive doubling for long messages and
 * medium-sized non-power-of-two messages.
 * Total Cost = (lgp+p-1).alpha + 2.n.((p-1)/p).beta
 */
int MPIR_Bcast_intra_scatter_ring_allgather(void *buffer,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            int root, MPIR_Comm * comm_ptr, int coll_group,
                                            MPIR_Errflag_t errflag)
{
    int rank, comm_size;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint scatter_size;
    int j, i, is_contig;
    MPI_Aint nbytes, type_size;
    int left, right, jnext;
    void *tmp_buf;
    MPI_Aint recvd_size, curr_size = 0;
    MPI_Status status;
    MPI_Aint true_extent, true_lb;
    MPIR_CHKLMEM_DECL(1);

    MPIR_COLL_RANK_SIZE(comm_ptr, coll_group, rank, comm_size);

    if (HANDLE_IS_BUILTIN(datatype))
        is_contig = 1;
    else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    if (is_contig) {
        /* contiguous. no need to pack. */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

        tmp_buf = MPIR_get_contig_ptr(buffer, true_lb);
    } else {
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

        if (rank == root) {
            mpi_errno = MPIR_Localcopy(buffer, count, datatype, tmp_buf, nbytes, MPI_BYTE);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    scatter_size = (nbytes + comm_size - 1) / comm_size;        /* ceiling division */

    mpi_errno = MPII_Scatter_for_bcast(buffer, count, datatype, root, comm_ptr, coll_group,
                                       nbytes, tmp_buf, is_contig, errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* long-message allgather or medium-size but non-power-of-two. use ring algorithm. */

    /* Calculate how much data we already have */
    curr_size = MPL_MIN(scatter_size,
                        nbytes - ((rank - root + comm_size) % comm_size) * scatter_size);
    if (curr_size < 0)
        curr_size = 0;

    left = (comm_size + rank - 1) % comm_size;
    right = (rank + 1) % comm_size;
    j = rank;
    jnext = left;
    for (i = 1; i < comm_size; i++) {
        MPI_Aint left_count, right_count, left_disp, right_disp;
        int rel_j, rel_jnext;

        rel_j = (j - root + comm_size) % comm_size;
        rel_jnext = (jnext - root + comm_size) % comm_size;
        left_count = MPL_MIN(scatter_size, (nbytes - rel_jnext * scatter_size));
        if (left_count < 0)
            left_count = 0;
        left_disp = rel_jnext * scatter_size;
        right_count = MPL_MIN(scatter_size, (nbytes - rel_j * scatter_size));
        if (right_count < 0)
            right_count = 0;
        right_disp = rel_j * scatter_size;

        mpi_errno = MPIC_Sendrecv((char *) tmp_buf + right_disp, right_count,
                                  MPI_BYTE, right, MPIR_BCAST_TAG,
                                  (char *) tmp_buf + left_disp, left_count,
                                  MPI_BYTE, left, MPIR_BCAST_TAG, comm_ptr, coll_group, &status,
                                  errflag);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
        curr_size += recvd_size;
        j = jnext;
        jnext = (comm_size + jnext - 1) % comm_size;
    }

#ifdef HAVE_ERROR_CHECKING
    /* check that we received as much as we expected */
    MPIR_ERR_CHKANDJUMP2(curr_size != nbytes, mpi_errno, MPI_ERR_OTHER,
                         "**collective_size_mismatch",
                         "**collective_size_mismatch %d %d", (int) curr_size, (int) nbytes);
#endif

    if (!is_contig) {
        if (rank != root) {
            mpi_errno = MPIR_Localcopy(tmp_buf, nbytes, MPI_BYTE, buffer, count, datatype);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
