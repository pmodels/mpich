/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"

/* the root, not a node root, sends to the root of its node */
static int send_to_node_root(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                             MPI_Aint nbytes, MPIR_Comm * comm_ptr);

/* A root that is not a node root first sends to the root of its node. Then a bcast among the node
 * roots, and one on every node. */
int MPIR_Bcast_intra_smp(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    void *buffer = coll_sig->u.bcast.buffer;
    MPI_Aint count = coll_sig->u.bcast.count;
    MPI_Datatype datatype = coll_sig->u.bcast.datatype;
    int root = coll_sig->u.bcast.root;
    MPI_Aint type_size;

    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr));

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPI_Aint nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    if (MPIR_Get_intranode_rank(comm_ptr, root) > 0) {
        mpi_errno = send_to_node_root(buffer, count, datatype, root, nbytes, comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (comm_ptr->node_roots_comm != NULL) {
        MPIR_COLL_INNER(Bcast, coll_sig, buffer, count, datatype,
                        MPIR_Get_internode_rank(comm_ptr, root), comm_ptr->node_roots_comm);
    }

    if (comm_ptr->node_comm != NULL) {
        MPIR_COLL_INNER(Bcast, coll_sig, buffer, count, datatype, 0, comm_ptr->node_comm);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* The root's node does its bcast first, from the root. Then a bcast among the node roots, and one
 * on every other node. This avoids a potential double data movement when the node root is not the
 * local lead. */
int MPIR_Bcast_intra_smp_node_first(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    void *buffer = coll_sig->u.bcast.buffer;
    MPI_Aint count = coll_sig->u.bcast.count;
    MPI_Datatype datatype = coll_sig->u.bcast.datatype;
    int root = coll_sig->u.bcast.root;
    MPI_Aint type_size;

    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr));

    MPIR_Datatype_get_size_macro(datatype, type_size);
    if (type_size * count == 0)
        goto fn_exit;   /* nothing to do */

    int node_root = MPIR_Get_intranode_rank(comm_ptr, root);

    if (node_root > 0) {
        MPIR_COLL_INNER(Bcast, coll_sig, buffer, count, datatype, node_root, comm_ptr->node_comm);
    }

    if (comm_ptr->node_roots_comm != NULL) {
        MPIR_COLL_INNER(Bcast, coll_sig, buffer, count, datatype,
                        MPIR_Get_internode_rank(comm_ptr, root), comm_ptr->node_roots_comm);
    }

    if (node_root <= 0 && comm_ptr->node_comm != NULL) {
        MPIR_COLL_INNER(Bcast, coll_sig, buffer, count, datatype, 0, comm_ptr->node_comm);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int send_to_node_root(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                             MPI_Aint nbytes, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_attr = 0;
    MPI_Status *status_p;
#ifdef HAVE_ERROR_CHECKING
    MPI_Status status;
    status_p = &status;
    MPI_Aint recvd_size;
#else
    status_p = MPI_STATUS_IGNORE;
#endif

    if (root == comm_ptr->rank) {
        mpi_errno = MPIC_Send(buffer, count, datatype, 0,
                              MPIR_BCAST_TAG, comm_ptr->node_comm, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (0 == comm_ptr->node_comm->rank) {
        mpi_errno = MPIC_Recv(buffer, count, datatype, MPIR_Get_intranode_rank(comm_ptr, root),
                              MPIR_BCAST_TAG, comm_ptr->node_comm, status_p);
        MPIR_ERR_CHECK(mpi_errno);
#ifdef HAVE_ERROR_CHECKING
        /* check that we received as much as we expected */
        MPIR_Get_count_impl(status_p, MPIR_BYTE_INTERNAL, &recvd_size);
        MPIR_ERR_CHKANDJUMP2(recvd_size != nbytes, mpi_errno, MPI_ERR_OTHER,
                             "**collective_size_mismatch",
                             "**collective_size_mismatch %d %d", (int) recvd_size, (int) nbytes);
#endif
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
