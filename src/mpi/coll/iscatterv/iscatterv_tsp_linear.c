/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

/* Routine to schedule a linear algorithm for scatterv */
int MPIR_TSP_Iscatterv_sched_allcomm_linear(const void *sendbuf, const MPI_Aint sendcounts[],
                                            const MPI_Aint displs[], MPI_Datatype sendtype,
                                            void *recvbuf, MPI_Aint recvcount,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                            MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int rank, comm_size;
    MPI_Aint extent;
    int i;
    int tag, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    rank = comm_ptr->rank;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    /* If I'm the root, then scatter */
    if (((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (root == rank)) ||
        ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) && (root == MPI_ROOT))) {
        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
            comm_size = comm_ptr->local_size;
        else
            comm_size = comm_ptr->remote_size;

        MPIR_Datatype_get_extent_macro(sendtype, extent);
        /* We need a check to ensure extent will fit in a
         * pointer. That needs extent * (max count) but we can't get
         * that without looping over the input data. This is at least
         * a minimal sanity check. Maybe add a global var since we do
         * loop over sendcount[] in MPI_Scatterv before calling
         * this? */

        for (i = 0; i < comm_size; i++) {
            if (sendcounts[i]) {
                if ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (i == rank)) {
                    if (recvbuf != MPI_IN_PLACE) {
                        mpi_errno =
                            MPIR_TSP_sched_localcopy(((char *) sendbuf + displs[rank] * extent),
                                                     sendcounts[rank], sendtype, recvbuf, recvcount,
                                                     recvtype, sched, 0, NULL, &vtx_id);
                    }
                } else {
                    mpi_errno = MPIR_TSP_sched_isend(((char *) sendbuf + displs[i] * extent),
                                                     sendcounts[i], sendtype, i, tag, comm_ptr,
                                                     sched, 0, NULL, &vtx_id);
                }
            }
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

    else if (root != MPI_PROC_NULL) {
        /* non-root nodes, and in the intercomm. case, non-root nodes on remote side */
        if (recvcount) {
            mpi_errno =
                MPIR_TSP_sched_irecv(recvbuf, recvcount, recvtype, root, tag, comm_ptr, sched, 0,
                                     NULL, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
