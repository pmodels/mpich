/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* FIXME This function uses some heuristsics based off of some testing on a
 * cluster at Argonne.  We need a better system for detrmining and controlling
 * the cutoff points for these algorithms.  If I've done this right, you should
 * be able to make changes along these lines almost exclusively in this function
 * and some new functions. [goodell@ 2008/01/07] */
int MPIR_Bcast_intra_smp(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                         MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag)
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

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) ||
        (comm_ptr->local_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        /* send to intranode-rank 0 on the root's node */
        if (comm_ptr->node_comm != NULL && MPIR_Get_intranode_rank(comm_ptr, root) > 0) {       /* is not the node root (0) and is on our node (!-1) */
            if (root == comm_ptr->rank) {
                mpi_errno = MPIC_Send(buffer, count, datatype, 0,
                                      MPIR_BCAST_TAG, comm_ptr->node_comm, errflag);
                MPIR_ERR_CHECK(mpi_errno);
            } else if (0 == comm_ptr->node_comm->rank) {
                mpi_errno =
                    MPIC_Recv(buffer, count, datatype, MPIR_Get_intranode_rank(comm_ptr, root),
                              MPIR_BCAST_TAG, comm_ptr->node_comm, status_p);
                MPIR_ERR_CHECK(mpi_errno);
#ifdef HAVE_ERROR_CHECKING
                /* check that we received as much as we expected */
                MPIR_Get_count_impl(status_p, MPI_BYTE, &recvd_size);
                MPIR_ERR_CHKANDJUMP2(recvd_size != nbytes, mpi_errno, MPI_ERR_OTHER,
                                     "**collective_size_mismatch",
                                     "**collective_size_mismatch %d %d",
                                     (int) recvd_size, (int) nbytes);
#endif
            }

        }

        /* perform the internode broadcast */
        if (comm_ptr->node_roots_comm != NULL) {
            mpi_errno = MPIR_Bcast(buffer, count, datatype,
                                   MPIR_Get_internode_rank(comm_ptr, root),
                                   comm_ptr->node_roots_comm, errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* perform the intranode broadcast on all except for the root's node */
        if (comm_ptr->node_comm != NULL) {
            mpi_errno = MPIR_Bcast(buffer, count, datatype, 0, comm_ptr->node_comm, errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {    /* (nbytes > MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_ptr->size >= MPIR_CVAR_BCAST_MIN_PROCS) */

        /* supposedly...
         * smp+doubling good for pof2
         * reg+ring better for non-pof2 */
        if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPL_is_pof2(comm_ptr->local_size)) {
            /* medium-sized msg and pof2 np */

            /* perform the intranode broadcast on the root's node */
            if (comm_ptr->node_comm != NULL && MPIR_Get_intranode_rank(comm_ptr, root) > 0) {   /* is not the node root (0) and is on our node (!-1) */
                /* FIXME binomial may not be the best algorithm for on-node
                 * bcast.  We need a more comprehensive system for selecting the
                 * right algorithms here. */
                mpi_errno = MPIR_Bcast(buffer, count, datatype,
                                       MPIR_Get_intranode_rank(comm_ptr, root),
                                       comm_ptr->node_comm, errflag);
                MPIR_ERR_CHECK(mpi_errno);
            }

            /* perform the internode broadcast */
            if (comm_ptr->node_roots_comm != NULL) {
                mpi_errno = MPIR_Bcast(buffer, count, datatype,
                                       MPIR_Get_internode_rank(comm_ptr, root),
                                       comm_ptr->node_roots_comm, errflag);
                MPIR_ERR_CHECK(mpi_errno);
            }

            /* perform the intranode broadcast on all except for the root's node */
            if (comm_ptr->node_comm != NULL && MPIR_Get_intranode_rank(comm_ptr, root) <= 0) {  /* 0 if root was local root too, -1 if different node than root */
                /* FIXME binomial may not be the best algorithm for on-node
                 * bcast.  We need a more comprehensive system for selecting the
                 * right algorithms here. */
                mpi_errno = MPIR_Bcast(buffer, count, datatype, 0, comm_ptr->node_comm, errflag);
                MPIR_ERR_CHECK(mpi_errno);
            }
        } else {        /* large msg or non-pof2 */

            /* FIXME It would be good to have an SMP-aware version of this
             * algorithm that (at least approximately) minimized internode
             * communication. */
            mpi_errno =
                MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr,
                                                        errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
