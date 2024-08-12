/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* The sticky point in the old smp bcast is when root is not a local root, resulting
 * an extra send/recv. With the new MPIR_Subgroup, we don't have to, in principle.
 */
int MPIR_Bcast_intra_smp_new(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                             MPIR_Comm * comm_ptr, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint type_size, nbytes = 0;
    MPI_Status *status_p;
#ifdef HAVE_ERROR_CHECKING
    MPI_Status status;
    status_p = &status;
    MPI_Aint recvd_size;
#else
    status_p = MPI_STATUS_IGNORE;
#endif

    MPIR_Assert(comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT);
    MPIR_Assert(MPIR_COLL_ATTR_GET_SUBGROUP(coll_attr) == 0);

    int node_group = -1, node_cross_group = -1;
    for (int i = 0; i < comm_ptr->num_subgroups; i++) {
        if (comm_ptr->subgroups[i].kind == MPIR_SUBGROUP_NODE) {
            node_group = i;
        }
        if (comm_ptr->subgroups[i].kind == MPIR_SUBGROUP_NODE_CROSS) {
            node_cross_group = i;
        }
    }
    MPIR_Assert(node_group > 0 && node_cross_group > 0);

#define NODEGROUP(field) comm_ptr->subgroups[node_group].field
    int local_rank, local_size;
    local_rank = NODEGROUP(rank);
    local_size = NODEGROUP(size);

    int local_root_rank = MPIR_Get_intranode_rank(comm_ptr, root);
    int inter_root_rank = MPIR_Get_internode_rank(comm_ptr, root);
    int node_attr = coll_attr | MPIR_COLL_ATTR_SUBGROUP(node_group);
    int node_cross_attr = coll_attr | MPIR_COLL_ATTR_SUBGROUP(node_cross_group);

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (local_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        /* send to intranode-rank 0 on the root's node */
        if (local_size > 1 && local_root_rank > 0) {
            if (root == comm_ptr->rank) {
                int node_root = NODEGROUP(proc_table)[0];
                mpi_errno = MPIC_Send(buffer, count, datatype, node_root, MPIR_BCAST_TAG, comm_ptr,
                                      coll_attr);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, coll_attr, mpi_errno_ret);
            } else if (local_rank == 0) {
                mpi_errno = MPIC_Recv(buffer, count, datatype, root, MPIR_BCAST_TAG, comm_ptr,
                                      coll_attr, status_p);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, coll_attr, mpi_errno_ret);
#ifdef HAVE_ERROR_CHECKING
                /* check that we received as much as we expected */
                MPIR_Get_count_impl(status_p, MPI_BYTE, &recvd_size);
                MPIR_ERR_COLL_CHECK_SIZE(recvd_size, nbytes, coll_attr, mpi_errno_ret);
#endif
            }

        }

        /* perform the internode broadcast */
        if (local_rank == 0) {
            mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, inter_root_rank,
                                                  comm_ptr, node_cross_attr);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, coll_attr, mpi_errno_ret);
        }

        /* perform the intranode broadcast */
        if (local_size > 1) {
            mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, 0, comm_ptr, node_attr);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, coll_attr, mpi_errno_ret);
        }
    } else {    /* (nbytes > MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_ptr->size >= MPIR_CVAR_BCAST_MIN_PROCS) */

        /* supposedly...
         * smp+doubling good for pof2
         * reg+ring better for non-pof2 */
        if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPL_is_pof2(local_size)) {
            /* medium-sized msg and pof2 np */

            /* perform the intranode broadcast on the root's node */
            if (local_size > 1 && local_root_rank > 0) {        /* is not the node root (0) and is on our node (!-1) */
                /* FIXME binomial may not be the best algorithm for on-node
                 * bcast.  We need a more comprehensive system for selecting the
                 * right algorithms here. */
                mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, local_root_rank,
                                                      comm_ptr, node_attr);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, coll_attr, mpi_errno_ret);
            }

            /* perform the internode broadcast */
            if (local_rank == 0) {
                mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, inter_root_rank,
                                                      comm_ptr, node_cross_attr);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, coll_attr, mpi_errno_ret);
            }

            /* perform the intranode broadcast on all except for the root's node */
            if (local_size > 1 && local_root_rank <= 0) {       /* 0 if root was local root too, -1 if different node than root */
                /* FIXME binomial may not be the best algorithm for on-node
                 * bcast.  We need a more comprehensive system for selecting the
                 * right algorithms here. */
                mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, 0,
                                                      comm_ptr, node_attr);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, coll_attr, mpi_errno_ret);
            }
        } else {        /* large msg or non-pof2 */

            /* FIXME It would be good to have an SMP-aware version of this
             * algorithm that (at least approximately) minimized internode
             * communication. */
            mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root,
                                                  comm_ptr, coll_attr);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, coll_attr, mpi_errno_ret);
        }
    }

  fn_exit:
    return mpi_errno_ret;
}
