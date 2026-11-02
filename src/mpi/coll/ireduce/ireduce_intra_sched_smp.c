/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Ireduce_intra_sched_smp(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    const void *sendbuf = coll_sig->u.reduce.sendbuf;
    void *recvbuf = coll_sig->u.reduce.recvbuf;
    MPI_Aint count = coll_sig->u.reduce.count;
    MPI_Datatype datatype = coll_sig->u.reduce.datatype;
    MPI_Op op = coll_sig->u.reduce.op;
    int root = coll_sig->u.reduce.root;
    MPIR_Sched_t s = coll_sig->sched;
    unsigned saved_flags = coll_sig->flags;
    void *tmp_buf = NULL;
    MPIR_Comm *nc;
    MPIR_Comm *nrc;

    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr));
    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    nc = comm_ptr->node_comm;
    nrc = comm_ptr->node_roots_comm;

    /* Create a temporary buffer on local roots of all nodes. It is host memory, so the ranks of
     * each inner call can mix it with the user's buffers, and no inner call has gpu_coll. The
     * flags are restored on exit. */
    coll_sig->flags &= ~MPIR_COLL_SIG_FLAG__GPU_COLL;
    if (nrc != NULL) {
        MPI_Aint buf_shift, buf_size;
        MPIR_Datatype_get_buffer_span(datatype, count, &buf_shift, &buf_size);
        tmp_buf = MPIR_Sched_alloc_state(s, buf_size);
        MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - buf_shift);
    }

    /* do the intranode reduce on all nodes other than the root's node */
    if (nc != NULL && MPIR_Get_intranode_rank(comm_ptr, root) == -1) {
        MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, sendbuf, tmp_buf, count, datatype, op, 0, nc);
        MPIR_SCHED_BARRIER(s);
    }

    /* do the internode reduce to the root's node */
    if (nrc != NULL) {
        int nrc_root = MPIR_Get_internode_rank(comm_ptr, root);
        if (nrc->rank != nrc_root) {
            /* I am not on root's node.  Use tmp_buf if we
             * participated in the first reduce, otherwise use sendbuf */
            const void *buf = (nc == NULL ? sendbuf : tmp_buf);
            MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, buf, NULL, count, datatype, op, nrc_root, nrc);
        } else if (comm_ptr->rank != root) {
            /* I am on root's node and not the root. I don't have a valid recvbuf.
             * Use tmp_buf as recvbuf. */
            MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, sendbuf, tmp_buf, count, datatype, op,
                                  nrc_root, nrc);

            /* point sendbuf at tmp_buf to make final intranode reduce easy */
            sendbuf = tmp_buf;
        } else {
            /* I am the root. in_place is automatically handled. */
            MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, sendbuf, recvbuf, count, datatype, op,
                                  nrc_root, nrc);

            /* set sendbuf to MPI_IN_PLACE to make final intranode reduce easy. */
            sendbuf = MPI_IN_PLACE;
        }
        MPIR_SCHED_BARRIER(s);
    }

    /* do the intranode reduce on the root's node */
    if (nc != NULL && MPIR_Get_intranode_rank(comm_ptr, root) != -1) {
        MPIR_COLL_SCHED_INNER(Ireduce, coll_sig, sendbuf, recvbuf, count, datatype, op,
                              MPIR_Get_intranode_rank(comm_ptr, root), nc);
        MPIR_SCHED_BARRIER(s);
    }

  fn_exit:
    coll_sig->flags = saved_flags;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
