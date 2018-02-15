/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Remote reduce local scatterv
 *
 * We first do an intercommunicator reduce to rank 0 on left group,
 * then an intercommunicator reduce to rank 0 on right group, followed
 * by local intracommunicator scattervs in each group.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_sched_inter_remote_reduce_local_scatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_sched_inter_remote_reduce_local_scatterv(const void *sendbuf,
                                                                  void *recvbuf,
                                                                  const int recvcounts[],
                                                                  MPI_Datatype datatype, MPI_Op op,
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, root, local_size, total_count, i;
    MPI_Aint true_extent, true_lb = 0, extent;
    void *tmp_buf = NULL;
    int *disps = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_SCHED_CHKPMEM_DECL(2);

    rank = comm_ptr->rank;
    local_size = comm_ptr->local_size;

    total_count = 0;
    for (i = 0; i < local_size; i++)
        total_count += recvcounts[i];

    if (rank == 0) {
        /* In each group, rank 0 allocates a temp. buffer for the
         * reduce */

        MPIR_SCHED_CHKPMEM_MALLOC(disps, int *, local_size * sizeof(int), mpi_errno, "disps",
                                  MPL_MEM_BUFFER);

        total_count = 0;
        for (i = 0; i < local_size; i++) {
            disps[i] = total_count;
            total_count += recvcounts[i];
        }

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, total_count * (MPL_MAX(extent, true_extent)),
                                  mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* first do a reduce from right group to rank 0 in left group,
     * then from left group to rank 0 in right group */
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Ireduce_sched(sendbuf, tmp_buf, total_count,
                                       datatype, op, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        /* sched barrier intentionally omitted here to allow both reductions to
         * proceed in parallel */

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPIR_Ireduce_sched(sendbuf, tmp_buf, total_count,
                                       datatype, op, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPIR_Ireduce_sched(sendbuf, tmp_buf, total_count,
                                       datatype, op, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        /* sched barrier intentionally omitted here to allow both reductions to
         * proceed in parallel */

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Ireduce_sched(sendbuf, tmp_buf, total_count,
                                       datatype, op, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    MPIR_SCHED_BARRIER(s);

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;

    mpi_errno = MPIR_Iscatterv_sched(tmp_buf, recvcounts, disps, datatype,
                                     recvbuf, recvcounts[rank], datatype, 0, newcomm_ptr, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
