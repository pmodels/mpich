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

int MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv(MPIR_Csel_coll_sig_s * coll_sig,
                                                                  MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.reduce_scatter.sendbuf;
    void *recvbuf = coll_sig->u.reduce_scatter.recvbuf;
    const MPI_Aint *recvcounts = coll_sig->u.reduce_scatter.recvcounts;
    MPI_Datatype datatype = coll_sig->u.reduce_scatter.datatype;
    MPI_Op op = coll_sig->u.reduce_scatter.op;
    MPIR_Sched_t s = coll_sig->sched;
    int mpi_errno = MPI_SUCCESS;
    int rank, root, local_size, total_count, i;
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

        MPI_Aint buf_shift, buf_size;
        MPIR_Datatype_get_buffer_span(datatype, total_count, &buf_shift, &buf_size);
        tmp_buf = MPIR_Sched_alloc_state(s, buf_size);
        MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - buf_shift);
    }

    /* first do a reduce from right group to rank 0 in left group,
     * then from left group to rank 0 in right group */
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, sendbuf, tmp_buf, total_count, datatype, op, root,
                              comm_ptr);

        /* sched barrier intentionally omitted here to allow both reductions to
         * proceed in parallel */

        /* reduce to rank 0 of right group */
        root = 0;
        MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, sendbuf, tmp_buf, total_count, datatype, op, root,
                              comm_ptr);
    } else {
        /* reduce to rank 0 of right group */
        root = 0;
        MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, sendbuf, tmp_buf, total_count, datatype, op, root,
                              comm_ptr);

        /* sched barrier intentionally omitted here to allow both reductions to
         * proceed in parallel */

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, sendbuf, tmp_buf, total_count, datatype, op, root,
                              comm_ptr);
    }
    MPIR_SCHED_BARRIER(s);

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;

    MPIR_COLL_SCHED_INNER(Iscatterv, coll_sig, tmp_buf, recvcounts, disps, datatype, recvbuf,
                          recvcounts[rank], datatype, 0, newcomm_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
