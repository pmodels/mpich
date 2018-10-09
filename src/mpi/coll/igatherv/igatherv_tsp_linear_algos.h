/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Header protection (i.e., IGATHERV_TSP_LINEAR_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"

/* Routine to schedule a linear algorithm for gatherv */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Igatherv_sched_allcomm_linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Igatherv_sched_allcomm_linear(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int recvcounts[], const int displs[],
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                           MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int comm_size, rank;
    MPI_Aint extent;
    int min_procs;
    int tag;

    rank = comm_ptr->rank;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* If rank == root, then I recv lots, otherwise I send */
    if (((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (root == rank)) ||
        ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) && (root == MPI_ROOT))) {
        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
            comm_size = comm_ptr->local_size;
        else
            comm_size = comm_ptr->remote_size;

        MPIR_Datatype_get_extent_macro(recvtype, extent);
        /* each node can make sure it is not going to overflow aint */
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                         displs[rank] * extent);

        for (i = 0; i < comm_size; i++) {
            if (recvcounts[i]) {
                if ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (i == rank)) {
                    if (sendbuf != MPI_IN_PLACE) {
                        MPIR_TSP_sched_localcopy(sendbuf, sendcount, sendtype,
                                                 ((char *) recvbuf + displs[rank] * extent),
                                                 recvcounts[rank], recvtype, sched, 0, NULL);
                    }
                } else {
                    MPIR_TSP_sched_irecv(((char *) recvbuf + displs[i] * extent),
                                         recvcounts[i], recvtype, i, tag, comm_ptr, sched, 0, NULL);
                }
            }
        }
    } else if (root != MPI_PROC_NULL) {
        /* non-root nodes, and in the intercomm. case, non-root nodes on remote side */
        if (sendcount) {
            /* we want local size in both the intracomm and intercomm cases
             * because the size of the root's group (group A in the standard) is
             * irrelevant here. */
            comm_size = comm_ptr->local_size;

            min_procs = MPIR_CVAR_GATHERV_INTER_SSEND_MIN_PROCS;
            if (min_procs == -1)
                min_procs = comm_size + 1;      /* Disable ssend */
            else if (min_procs == 0)    /* backwards compatibility, use default value */
                MPIR_CVAR_GET_DEFAULT_INT(GATHERV_INTER_SSEND_MIN_PROCS, &min_procs);

            if (comm_size >= min_procs)
                MPIR_TSP_sched_issend(sendbuf, sendcount, sendtype, root, tag, comm_ptr, sched, 0,
                                      NULL);
            else
                MPIR_TSP_sched_isend(sendbuf, sendcount, sendtype, root, tag, comm_ptr, sched, 0,
                                     NULL);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking linear algorithm for gatherv */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Igatherv_allcomm_linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Igatherv_allcomm_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, const int recvcounts[], const int displs[],
                                     MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                     MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IGATHERV_ALLCOMM_LINEAR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_TSP_IGATHERV_ALLCOMM_LINEAR);

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_TSP_sched_create(sched);

    /* schedule linear algo */
    mpi_errno =
        MPIR_TSP_Igatherv_sched_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf,
                                               recvcounts, displs, recvtype, root, comm, sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IGATHERV_ALLCOMM_LINEAR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
