/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_BCAST_SMP_NONROOT_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select SMP algorithms when root is not a local root.
        auto - Internal auto selection
        send - root first send data to local root
        bcast - root first bcast within the local node

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Bcast_intra_smp(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                         MPIR_Comm * comm_ptr, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size, nbytes = 0;
    MPI_Status *status_p;
#ifdef HAVE_ERROR_CHECKING
    MPI_Status status;
    status_p = &status;
    MPI_Aint recvd_size;
#else
    status_p = MPI_STATUS_IGNORE;
#endif

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr));
#endif

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    /* if root is on my local node but not the local root */
    bool need_pre_local_step = (MPIR_Get_intranode_rank(comm_ptr, root) > 0);
    /* the pre_local_step may done the local bcast already */
    bool need_post_local_bcast = true;

    if (need_pre_local_step) {
        int nonroot_algo = MPIR_CVAR_BCAST_SMP_NONROOT_ALGORITHM;
        if (nonroot_algo == MPIR_CVAR_BCAST_SMP_NONROOT_ALGORITHM_auto) {
            /* overwrite "auto" */
            nonroot_algo = MPIR_CVAR_BCAST_SMP_NONROOT_ALGORITHM_send;
        }

        if (nonroot_algo == MPIR_CVAR_BCAST_SMP_NONROOT_ALGORITHM_send) {
            /* send to intranode-rank 0 on the root's node */
            if (root == comm_ptr->rank) {
                mpi_errno = MPIC_Send(buffer, count, datatype, 0,
                                      MPIR_BCAST_TAG, comm_ptr->node_comm, coll_attr);
                MPIR_ERR_CHECK(mpi_errno);
            } else if (0 == comm_ptr->node_comm->rank) {
                mpi_errno = MPIC_Recv(buffer, count, datatype,
                                      MPIR_Get_intranode_rank(comm_ptr, root),
                                      MPIR_BCAST_TAG, comm_ptr->node_comm, status_p);
                MPIR_ERR_CHECK(mpi_errno);
#ifdef HAVE_ERROR_CHECKING
                /* check that we received as much as we expected */
                MPIR_Get_count_impl(status_p, MPIR_BYTE_INTERNAL, &recvd_size);
                MPIR_ERR_CHKANDJUMP2(recvd_size != nbytes, mpi_errno, MPI_ERR_OTHER,
                                     "**collective_size_mismatch",
                                     "**collective_size_mismatch %d %d",
                                     (int) recvd_size, (int) nbytes);
#endif
            }
            /* TODO: set need_post_local_bcast = false if node_comm->local_size is 2 */
        } else if (nonroot_algo == MPIR_CVAR_BCAST_SMP_NONROOT_ALGORITHM_bcast) {
            mpi_errno = MPIR_Bcast(buffer, count, datatype, MPIR_Get_intranode_rank(comm_ptr, root),
                                   comm_ptr->node_comm, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);

            need_post_local_bcast = false;
        }
    }

    /* perform the internode broadcast */
    if (comm_ptr->node_roots_comm != NULL) {
        mpi_errno = MPIR_Bcast(buffer, count, datatype,
                               MPIR_Get_internode_rank(comm_ptr, root),
                               comm_ptr->node_roots_comm, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* perform the intranode broadcast unless it's done in pre-step */
    if (need_post_local_bcast) {
        mpi_errno = MPIR_Bcast(buffer, count, datatype, 0, comm_ptr->node_comm, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
