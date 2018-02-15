/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
 * Linear
 *
 * Root receives from all processes, everyone else sends to root.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Igatherv_sched_allcomm_linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igatherv_sched_allcomm_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, const int recvcounts[], const int displs[],
                                       MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                       MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int comm_size, rank;
    MPI_Aint extent;
    int min_procs;

    rank = comm_ptr->rank;

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
                        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                                    ((char *) recvbuf + displs[rank] * extent),
                                                    recvcounts[rank], recvtype, s);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);
                    }
                } else {
                    mpi_errno = MPIR_Sched_recv(((char *) recvbuf + displs[i] * extent),
                                                recvcounts[i], recvtype, i, comm_ptr, s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
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
                mpi_errno = MPIR_Sched_ssend(sendbuf, sendcount, sendtype, root, comm_ptr, s);
            else
                mpi_errno = MPIR_Sched_send(sendbuf, sendcount, sendtype, root, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
