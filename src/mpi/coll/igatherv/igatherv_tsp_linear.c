/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

/* Routine to schedule a linear algorithm for gatherv. This algorithm has
 * been ported to the tsp infrastructure from the previous MPI_Gatherv
 * implementation. */
/* Since the array of recvcounts is valid only on the root, we cannot
   do a tree algorithm without first communicating the recvcounts to
   other processes. Therefore, we simply use a linear algorithm for the
   gather, which takes (p-1) steps versus lgp steps for the tree
   algorithm. The bandwidth requirement is the same for both algorithms.

   Cost = (p-1).alpha + n.((p-1)/p).beta
*/
int MPIR_TSP_Igatherv_sched_allcomm_linear(const void *sendbuf, MPI_Aint sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                           MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int i, vtx_id;
    int comm_size, rank;
    MPI_Aint extent;
    int min_procs;
    int tag;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    rank = comm_ptr->rank;

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    /* If rank == root, then I recv lots, otherwise I send */
    if (((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (root == rank)) ||
        ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) && (root == MPI_ROOT))) {
        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
            comm_size = comm_ptr->local_size;
        else
            comm_size = comm_ptr->remote_size;

        MPIR_Datatype_get_extent_macro(recvtype, extent);

        for (i = 0; i < comm_size; i++) {
            if (recvcounts[i]) {
                if ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (i == rank)) {
                    if (sendbuf != MPI_IN_PLACE) {
                        mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, sendcount, sendtype,
                                                             ((char *) recvbuf +
                                                              displs[rank] * extent),
                                                             recvcounts[rank], recvtype, sched, 0,
                                                             NULL, &vtx_id);
                    }
                } else {
                    mpi_errno = MPIR_TSP_sched_irecv(((char *) recvbuf + displs[i] * extent),
                                                     recvcounts[i], recvtype, i, tag, comm_ptr,
                                                     sched, 0, NULL, &vtx_id);
                }
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
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
                mpi_errno =
                    MPIR_TSP_sched_issend(sendbuf, sendcount, sendtype, root, tag, comm_ptr, sched,
                                          0, NULL, &vtx_id);
            else
                mpi_errno =
                    MPIR_TSP_sched_isend(sendbuf, sendcount, sendtype, root, tag, comm_ptr, sched,
                                         0, NULL, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
