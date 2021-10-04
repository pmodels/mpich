/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Remote reduce local scatterv
 *
 * We first do an intercommunicator reduce to rank 0 on left group,
 * then an intercommunicator reduce to rank 0 on right group, followed
 * by local intracommunicator scattervs in each group.
 */

int MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv(const void *sendbuf,
                                                                  void *recvbuf,
                                                                  const MPI_Aint recvcounts[],
                                                                  MPI_Datatype datatype, MPI_Op op,
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, root, local_size, total_count, i;
    MPI_Aint true_extent, true_lb = 0, extent;
    void *tmp_buf = NULL;
    MPI_Aint *disps = NULL;
    MPIR_Comm *newcomm_ptr = NULL;

    rank = comm_ptr->rank;
    local_size = comm_ptr->local_size;

    total_count = 0;
    for (i = 0; i < local_size; i++)
        total_count += recvcounts[i];

    if (rank == 0) {
        /* In each group, rank 0 allocates a temp. buffer for the
         * reduce */

        disps = MPIR_Sched_alloc_state(s, local_size * sizeof(MPI_Aint));
        MPIR_ERR_CHKANDJUMP(!disps, mpi_errno, MPI_ERR_OTHER, "**nomem");

        total_count = 0;
        for (i = 0; i < local_size; i++) {
            disps[i] = total_count;
            total_count += recvcounts[i];
        }

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        tmp_buf = MPIR_Sched_alloc_state(s, total_count * (MPL_MAX(extent, true_extent)));
        MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* first do a reduce from right group to rank 0 in left group,
     * then from left group to rank 0 in right group */
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Ireduce_inter_sched_auto(sendbuf, tmp_buf, total_count,
                                                  datatype, op, root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);

        /* sched barrier intentionally omitted here to allow both reductions to
         * proceed in parallel */

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPIR_Ireduce_inter_sched_auto(sendbuf, tmp_buf, total_count,
                                                  datatype, op, root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPIR_Ireduce_inter_sched_auto(sendbuf, tmp_buf, total_count,
                                                  datatype, op, root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);

        /* sched barrier intentionally omitted here to allow both reductions to
         * proceed in parallel */

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Ireduce_inter_sched_auto(sendbuf, tmp_buf, total_count,
                                                  datatype, op, root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    }
    MPIR_SCHED_BARRIER(s);

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;

    mpi_errno = MPIR_Iscatterv_intra_sched_auto(tmp_buf, recvcounts, disps, datatype,
                                                recvbuf, recvcounts[rank], datatype, 0, newcomm_ptr,
                                                s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
