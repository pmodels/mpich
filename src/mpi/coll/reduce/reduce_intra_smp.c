/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Reduce_intra_smp(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.reduce.sendbuf;
    void *recvbuf = coll_sig->u.reduce.recvbuf;
    MPI_Aint count = coll_sig->u.reduce.count;
    MPI_Datatype datatype = coll_sig->u.reduce.datatype;
    MPI_Op op = coll_sig->u.reduce.op;
    int root = coll_sig->u.reduce.root;
    int mpi_errno = MPI_SUCCESS;
    void *tmp_buf = NULL;
    unsigned saved_flags = coll_sig->flags;
    MPIR_CHKLMEM_DECL();

#ifdef HAVE_ERROR_CHECKING
    {
        int is_commutative;
        is_commutative = MPIR_Op_is_commutative(op);
        MPIR_Assertp(is_commutative);
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Create a temporary buffer on local roots of all nodes. It is host memory, so the ranks of
     * each inner call can mix it with the user's buffers, and no inner call has gpu_coll. The
     * flags are restored on exit. */
    coll_sig->flags &= ~MPIR_COLL_SIG_FLAG__GPU_COLL;
    if (comm_ptr->node_roots_comm != NULL) {

        MPI_Aint buf_shift, buf_size;
        MPIR_Datatype_get_buffer_span(datatype, count, &buf_shift, &buf_size);
        MPIR_CHKLMEM_MALLOC(tmp_buf, buf_size);
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - buf_shift);
    }

    /* do the intranode reduce on all nodes other than the root's node */
    if (comm_ptr->node_comm != NULL && MPIR_Get_intranode_rank(comm_ptr, root) == -1) {
        MPIR_COLL_INNER(Reduce, coll_sig, sendbuf, tmp_buf, count, datatype, op, 0,
                        comm_ptr->node_comm);
    }

    /* do the internode reduce to the root's node */
    if (comm_ptr->node_roots_comm != NULL) {
        if (comm_ptr->node_roots_comm->rank != MPIR_Get_internode_rank(comm_ptr, root)) {
            /* I am not on root's node.  Use tmp_buf if we
             * participated in the first reduce, otherwise use sendbuf */
            const void *buf = (comm_ptr->node_comm == NULL ? sendbuf : tmp_buf);
            MPIR_COLL_INNER(Reduce, coll_sig, buf, NULL, count, datatype, op,
                            MPIR_Get_internode_rank(comm_ptr, root), comm_ptr->node_roots_comm);
        } else {        /* I am on root's node. I have not participated in the earlier reduce. */
            if (comm_ptr->rank != root) {
                /* I am not the root though. I don't have a valid recvbuf.
                 * Use tmp_buf as recvbuf. */

                MPIR_COLL_INNER(Reduce, coll_sig, sendbuf, tmp_buf, count, datatype, op,
                                MPIR_Get_internode_rank(comm_ptr, root), comm_ptr->node_roots_comm);

                /* point sendbuf at tmp_buf to make final intranode reduce easy */
                sendbuf = tmp_buf;
            } else {
                /* I am the root. in_place is automatically handled. */

                MPIR_COLL_INNER(Reduce, coll_sig, sendbuf, recvbuf, count, datatype, op,
                                MPIR_Get_internode_rank(comm_ptr, root), comm_ptr->node_roots_comm);

                /* set sendbuf to MPI_IN_PLACE to make final intranode reduce easy. */
                sendbuf = MPI_IN_PLACE;
            }
        }

    }

    /* do the intranode reduce on the root's node */
    if (comm_ptr->node_comm != NULL && MPIR_Get_intranode_rank(comm_ptr, root) != -1) {
        MPIR_COLL_INNER(Reduce, coll_sig, sendbuf, recvbuf, count, datatype, op,
                        MPIR_Get_intranode_rank(comm_ptr, root), comm_ptr->node_comm);
    }

  fn_exit:
    coll_sig->flags = saved_flags;
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
