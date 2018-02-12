/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_intra_smp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_intra_smp(const void *sendbuf, void *recvbuf, int count,
                          MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                          MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    void *tmp_buf = NULL;
    MPI_Aint true_lb, true_extent, extent;
    MPIR_CHKLMEM_DECL(1);

#ifdef HAVE_ERROR_CHECKING
    {
        int is_commutative;
        is_commutative = MPIR_Op_is_commutative(op);
        MPIR_Assert(is_commutative);
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Create a temporary buffer on local roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* do the intranode reduce on all nodes other than the root's node */
    if (comm_ptr->node_comm != NULL && MPIR_Get_intranode_rank(comm_ptr, root) == -1) {
        mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, count, datatype,
                                op, 0, comm_ptr->node_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* do the internode reduce to the root's node */
    if (comm_ptr->node_roots_comm != NULL) {
        if (comm_ptr->node_roots_comm->rank != MPIR_Get_internode_rank(comm_ptr, root)) {
            /* I am not on root's node.  Use tmp_buf if we
             * participated in the first reduce, otherwise use sendbuf */
            const void *buf = (comm_ptr->node_comm == NULL ? sendbuf : tmp_buf);
            mpi_errno = MPIR_Reduce(buf, NULL, count, datatype,
                                    op, MPIR_Get_internode_rank(comm_ptr, root),
                                    comm_ptr->node_roots_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        } else {        /* I am on root's node. I have not participated in the earlier reduce. */
            if (comm_ptr->rank != root) {
                /* I am not the root though. I don't have a valid recvbuf.
                 * Use tmp_buf as recvbuf. */

                mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, count, datatype,
                                        op, MPIR_Get_internode_rank(comm_ptr, root),
                                        comm_ptr->node_roots_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                /* point sendbuf at tmp_buf to make final intranode reduce easy */
                sendbuf = tmp_buf;
            } else {
                /* I am the root. in_place is automatically handled. */

                mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype,
                                        op, MPIR_Get_internode_rank(comm_ptr, root),
                                        comm_ptr->node_roots_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                /* set sendbuf to MPI_IN_PLACE to make final intranode reduce easy. */
                sendbuf = MPI_IN_PLACE;
            }
        }

    }

    /* do the intranode reduce on the root's node */
    if (comm_ptr->node_comm != NULL && MPIR_Get_intranode_rank(comm_ptr, root) != -1) {
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype,
                                op, MPIR_Get_intranode_rank(comm_ptr, root),
                                comm_ptr->node_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
