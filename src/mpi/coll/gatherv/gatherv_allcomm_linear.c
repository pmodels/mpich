/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_GATHERV_INTER_SSEND_MIN_PROCS
      category    : COLLECTIVE
      type        : int
      default     : 32
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use Ssend (synchronous send) for intercommunicator MPI_Gatherv if the
        "group B" size is >= this value.  Specifying "-1" always avoids using
        Ssend.  For backwards compatibility, specifying "0" uses the default
        value.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Algorithm: MPI_Gatherv
 *
 * Since the array of recvcounts is valid only on the root, we cannot do a tree
 * algorithm without first communicating the recvcounts to other processes.
 * Therefore, we simply use a linear algorithm for the gather, which takes
 * (p-1) steps versus lgp steps for the tree algorithm. The bandwidth
 * requirement is the same for both algorithms.
 *
 * Cost = (p-1).alpha + n.((p-1)/p).beta
*/
int MPIR_Gatherv_allcomm_linear(const void *sendbuf,
                                MPI_Aint sendcount,
                                MPI_Datatype sendtype,
                                void *recvbuf,
                                const MPI_Aint * recvcounts,
                                const MPI_Aint * displs,
                                MPI_Datatype recvtype,
                                int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag)
{
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint extent;
    int i, reqs;
    int min_procs;
    MPIR_Request **reqarray;
    MPI_Status *starray;
    MPIR_CHKLMEM_DECL(2);

    MPIR_THREADCOMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /* If rank == root, then I recv lots, otherwise I send */
    if (((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (root == rank)) ||
        ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) && (root == MPI_ROOT))) {
        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
            comm_size = comm_ptr->remote_size;

        MPIR_Datatype_get_extent_macro(recvtype, extent);

        MPIR_CHKLMEM_MALLOC(reqarray, MPIR_Request **, comm_size * sizeof(MPIR_Request *),
                            mpi_errno, "reqarray", MPL_MEM_BUFFER);
        MPIR_CHKLMEM_MALLOC(starray, MPI_Status *, comm_size * sizeof(MPI_Status), mpi_errno,
                            "starray", MPL_MEM_BUFFER);

        reqs = 0;
        for (i = 0; i < comm_size; i++) {
            if (recvcounts[i]) {
                if ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (i == rank)) {
                    if (sendbuf != MPI_IN_PLACE) {
                        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                                   ((char *) recvbuf + displs[rank] * extent),
                                                   recvcounts[rank], recvtype);
                        MPIR_ERR_CHECK(mpi_errno);
                    }
                } else {
                    mpi_errno = MPIC_Irecv(((char *) recvbuf + displs[i] * extent),
                                           recvcounts[i], recvtype, i,
                                           MPIR_GATHERV_TAG, comm_ptr, &reqarray[reqs++]);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            }
        }
        /* ... then wait for *all* of them to finish: */
        mpi_errno = MPIC_Waitall(reqs, reqarray, starray);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
    }

    else if (root != MPI_PROC_NULL) {   /* non-root nodes, and in the intercomm. case, non-root nodes on remote side */
        if (sendcount) {
            /* we want local size in both the intracomm and intercomm cases
             * because the size of the root's group (group A in the standard) is
             * irrelevant here. */
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                comm_size = comm_ptr->local_size;

            min_procs = MPIR_CVAR_GATHERV_INTER_SSEND_MIN_PROCS;
            if (min_procs == -1)
                min_procs = comm_size + 1;      /* Disable ssend */
            else if (min_procs == 0)    /* backwards compatibility, use default value */
                MPIR_CVAR_GET_DEFAULT_INT(MPIR_CVAR_GATHERV_INTER_SSEND_MIN_PROCS, &min_procs);

            if (comm_size >= min_procs) {
                mpi_errno = MPIC_Ssend(sendbuf, sendcount, sendtype, root,
                                       MPIR_GATHERV_TAG, comm_ptr, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
            } else {
                mpi_errno = MPIC_Send(sendbuf, sendcount, sendtype, root,
                                      MPIR_GATHERV_TAG, comm_ptr, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
            }
        }
    }


  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno_ret;
  fn_fail:
    mpi_errno_ret = mpi_errno;
    goto fn_exit;
}
