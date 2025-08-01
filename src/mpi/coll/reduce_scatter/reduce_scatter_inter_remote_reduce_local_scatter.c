/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Intercommunicator Reduce_scatter
 *
 * We first do an intercommunicator reduce to rank 0 on left group,
 * then an intercommunicator reduce to rank 0 on right group, followed
 * by local intracommunicator scattervs in each group.
 */

int MPIR_Reduce_scatter_inter_remote_reduce_local_scatter(const void *sendbuf, void *recvbuf,
                                                          const MPI_Aint recvcounts[],
                                                          MPI_Datatype datatype, MPI_Op op,
                                                          MPIR_Comm * comm_ptr, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, root, local_size, total_count, i;
    MPI_Aint true_extent, true_lb = 0, extent;
    void *tmp_buf = NULL;
    MPI_Aint *disps = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL();

    rank = comm_ptr->rank;
    local_size = comm_ptr->local_size;

    total_count = 0;
    for (i = 0; i < local_size; i++)
        total_count += recvcounts[i];

    if (rank == 0) {
        /* In each group, rank 0 allocates a temp. buffer for the
         * reduce */

        MPIR_CHKLMEM_MALLOC(disps, local_size * sizeof(MPI_Aint));

        total_count = 0;
        for (i = 0; i < local_size; i++) {
            disps[i] = total_count;
            total_count += recvcounts[i];
        }

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_CHKLMEM_MALLOC(tmp_buf, total_count * (MPL_MAX(extent, true_extent)));

        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* first do a reduce from right group to rank 0 in left group,
     * then from left group to rank 0 in right group */
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Reduce_allcomm_auto(sendbuf, tmp_buf, total_count, datatype, op,
                                             root, comm_ptr, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPIR_Reduce_allcomm_auto(sendbuf, tmp_buf, total_count, datatype, op,
                                             root, comm_ptr, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* reduce to rank 0 of left group */
        root = 0;
        mpi_errno = MPIR_Reduce_allcomm_auto(sendbuf, tmp_buf, total_count, datatype, op,
                                             root, comm_ptr, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Reduce_allcomm_auto(sendbuf, tmp_buf, total_count, datatype, op,
                                             root, comm_ptr, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;

    mpi_errno = MPIR_Scatterv(tmp_buf, recvcounts, disps, datatype, recvbuf,
                              recvcounts[rank], datatype, 0, newcomm_ptr, coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
