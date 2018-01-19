/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* FIXME This function uses some heuristsics based off of some testing on a
 * cluster at Argonne.  We need a better system for detrmining and controlling
 * the cutoff points for these algorithms.  If I've done this right, you should
 * be able to make changes along these lines almost exclusively in this function
 * and some new functions. [goodell@ 2008/01/07] */
#undef FUNCNAME
#define FUNCNAME MPIR_Bcast__intra__smp
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bcast__intra__smp( void *buffer, int count, MPI_Datatype datatype, int root,
        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int is_homogeneous;
    MPI_Aint type_size, nbytes=0;
    MPI_Status status;
    int recvd_size;

#ifdef HAVE_ERROR_CHECKING
    if (!MPIR_CVAR_ENABLE_SMP_COLLECTIVES || !MPIR_CVAR_ENABLE_SMP_BCAST) {
        MPIR_Assert(0);
    }
    MPIR_Assert(MPIR_Comm_is_node_aware(comm_ptr));
#endif

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous) {
        MPIR_Datatype_get_size_macro(datatype, type_size);
    } else {
        /* --BEGIN HETEROGENEOUS-- */
        MPIR_Pack_size_impl(1, datatype, &type_size);
        /* --END HETEROGENEOUS-- */
    }

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit; /* nothing to do */

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_ptr->local_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        /* send to intranode-rank 0 on the root's node */
        if (comm_ptr->node_comm != NULL &&
                MPIR_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */
        {                                                /* and is on our node (!-1) */
            if (root == comm_ptr->rank) {
                mpi_errno = MPIC_Send(buffer,count,datatype,0,
                        MPIR_BCAST_TAG,comm_ptr->node_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            } else if (0 == comm_ptr->node_comm->rank) {
                mpi_errno = MPIC_Recv(buffer,count,datatype,MPIR_Get_intranode_rank(comm_ptr, root),
                        MPIR_BCAST_TAG,comm_ptr->node_comm, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                /* check that we received as much as we expected */
                MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
                /* recvd_size may not be accurate for packed heterogeneous data */
                if (is_homogeneous && recvd_size != nbytes) {
                    if (*errflag == MPIR_ERR_NONE) *errflag = MPIR_ERR_OTHER;
                    MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
                            "**collective_size_mismatch",
                            "**collective_size_mismatch %d %d",
                            recvd_size, nbytes );
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }

        }

        /* perform the internode broadcast */
        if (comm_ptr->node_roots_comm != NULL)
        {
            mpi_errno = MPIR_Bcast(buffer, count, datatype,
                    MPIR_Get_internode_rank(comm_ptr, root),
                    comm_ptr->node_roots_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

        /* perform the intranode broadcast on all except for the root's node */
        if (comm_ptr->node_comm != NULL)
        {
            mpi_errno = MPIR_Bcast(buffer, count, datatype, 0,
                    comm_ptr->node_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    } else /* (nbytes > MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_ptr->size >= MPIR_CVAR_BCAST_MIN_PROCS) */ {
        /* supposedly...
           smp+doubling good for pof2
           reg+ring better for non-pof2 */
        if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPL_is_pof2(comm_ptr->local_size, NULL))
        {
            /* medium-sized msg and pof2 np */

            /* perform the intranode broadcast on the root's node */
            if (comm_ptr->node_comm != NULL &&
                    MPIR_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */
            {                                                /* and is on our node (!-1) */
                /* FIXME binomial may not be the best algorithm for on-node
                   bcast.  We need a more comprehensive system for selecting the
                   right algorithms here. */
                mpi_errno = MPIR_Bcast(buffer, count, datatype,
                        MPIR_Get_intranode_rank(comm_ptr, root),
                        comm_ptr->node_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }

            /* perform the internode broadcast */
            if (comm_ptr->node_roots_comm != NULL)
            {
                mpi_errno = MPIR_Bcast(buffer, count, datatype,
                        MPIR_Get_internode_rank(comm_ptr, root),
                        comm_ptr->node_roots_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }

            /* perform the intranode broadcast on all except for the root's node */
            if (comm_ptr->node_comm != NULL &&
                    MPIR_Get_intranode_rank(comm_ptr, root) <= 0) /* 0 if root was local root too */
            {                                                 /* -1 if different node than root */
                /* FIXME binomial may not be the best algorithm for on-node
                   bcast.  We need a more comprehensive system for selecting the
                   right algorithms here. */
                mpi_errno = MPIR_Bcast(buffer, count, datatype, 0,
                        comm_ptr->node_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        } else /* large msg or non-pof2 */ {
            /* FIXME It would be good to have an SMP-aware version of this
               algorithm that (at least approximately) minimized internode
               communication. */
            mpi_errno = MPIR_Bcast__intra__scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

fn_exit:
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

