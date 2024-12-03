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
int MPIR_Bcast_intra_scatter_ring_allgather_group(void *buffer,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            int root, MPIR_Comm * comm_ptr, int* group, int group_size, MPIR_Errflag_t errflag)
{
    int rank;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint scatter_size;
    int j, i, is_contig;
    MPI_Aint nbytes, type_size;
    int left, right, jnext;
    void *tmp_buf;
    MPI_Aint recvd_size, curr_size = 0;
    MPI_Status status;
    MPI_Aint true_extent, true_lb;
    MPIR_CHKLMEM_DECL(1);
    
    rank = comm_ptr->rank;
    int group_rank, group_root;
    
    bool found_rank_in_group = find_local_rank_linear(group, group_size, rank, root, &group_rank, &group_root);
    if (!found_rank_in_group) go fn_exit;
    
    /* Uncomment the below code snippet for isolated testing */

    // if (!found_rank_in_group) {
    //     return mpi_errno_ret;
    // }

    MPIR_Assert(found_rank_in_group);
    

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
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

        tmp_buf = MPIR_get_contig_ptr(buffer, true_lb);
    } else {
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

        if (rank == root) {
            mpi_errno = MPIR_Localcopy(buffer, count, datatype, tmp_buf, nbytes, MPI_BYTE);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    scatter_size = (nbytes + group_size - 1) / group_size;        

    //MPII_Scatter_for_bcast needs to be modified to work with groups

    mpi_errno = MPII_Scatter_for_bcast_group(buffer, count, datatype, root, comm_ptr, group, group_size,
                                       nbytes, tmp_buf, is_contig, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);

    curr_size = MPL_MIN(scatter_size,
                        nbytes - ((group_rank - group_root + group_size) % group_size) * scatter_size);
    if (curr_size < 0)
        curr_size = 0;

    left = (group_size + group_rank - 1) % group_size;
    right = (group_rank + 1) % group_size;
    j = group_rank;
    jnext = left;
    for (i = 1; i < group_size; i++) {
        MPI_Aint left_count, right_count, left_disp, right_disp;
        int rel_j, rel_jnext;

        rel_j = (j - group_root + group_size) % group_size;
        rel_jnext = (jnext - group_root + group_size) % group_size;
        left_count = MPL_MIN(scatter_size, (nbytes - rel_jnext * scatter_size));
        if (left_count < 0)
            left_count = 0;
        left_disp = rel_jnext * scatter_size;
        right_count = MPL_MIN(scatter_size, (nbytes - rel_j * scatter_size));
        if (right_count < 0)
            right_count = 0;
        right_disp = rel_j * scatter_size;

        mpi_errno = MPIC_Sendrecv((char *) tmp_buf + right_disp, right_count,
                                  MPI_BYTE, group[right], MPIR_BCAST_TAG,
                                  (char *) tmp_buf + left_disp, left_count,
                                  MPI_BYTE, group[left], MPIR_BCAST_TAG, comm_ptr, &status, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
        MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
        curr_size += recvd_size;
        j = jnext;
        jnext = (group_size + jnext - 1) % group_size;
    }

#ifdef HAVE_ERROR_CHECKING
    MPIR_ERR_COLL_CHECK_SIZE(curr_size, nbytes, errflag, mpi_errno_ret);
#endif

    if (!is_contig) {
        if (rank != root) {
            mpi_errno = MPIR_Localcopy(tmp_buf, nbytes, MPI_BYTE, buffer, count, datatype);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno_ret;
  fn_fail:
    mpi_errno_ret = mpi_errno;
    goto fn_exit;
}
