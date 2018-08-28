/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., ISCATTERV_TSP_LINEAR_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"

/* Routine to schedule a linear algorithm for scatterv */
int MPIR_TSP_Iscatterv_sched_allcomm_linear(const void *sendbuf, const int sendcounts[],
                                            const int displs[], MPI_Datatype sendtype,
                                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                            int root, MPIR_Comm * comm_ptr,
                                            MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size;
    MPI_Aint extent;
    int i;
    int tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_ISCATTERV_SCHED_ALLCOMM_LINEAR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_ISCATTERV_SCHED_ALLCOMM_LINEAR);

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
                        MPIR_TSP_sched_localcopy(((char *) sendbuf + displs[rank] * extent),
                                                 sendcounts[rank], sendtype,
                                                 recvbuf, recvcount, recvtype, sched, 0, NULL);
                    }
                } else {
                    MPIR_TSP_sched_isend(((char *) sendbuf + displs[i] * extent),
                                         sendcounts[i], sendtype, i, tag, comm_ptr, sched, 0, NULL);
                }
            }
        }
    }

    else if (root != MPI_PROC_NULL) {
        /* non-root nodes, and in the intercomm. case, non-root nodes on remote side */
        if (recvcount) {
            MPIR_TSP_sched_irecv(recvbuf, recvcount, recvtype, root, tag, comm_ptr, sched, 0, NULL);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_ISCATTERV_SCHED_ALLCOMM_LINEAR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking linear algorithm for scatterv */
int MPIR_TSP_Iscatterv_allcomm_linear(const void *sendbuf, const int sendcounts[],
                                      const int displs[], MPI_Datatype sendtype, void *recvbuf,
                                      int recvcount, MPI_Datatype recvtype, int root,
                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_ISCATTERV_ALLCOMM_LINEAR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_ISCATTERV_ALLCOMM_LINEAR);

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!sched, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_TSP_sched_create(sched, false);

    /* schedule linear algo */
    mpi_errno =
        MPIR_TSP_Iscatterv_sched_allcomm_linear(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                                recvcount, recvtype, root, comm, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_ISCATTERV_ALLCOMM_LINEAR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
