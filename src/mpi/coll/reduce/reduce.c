/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
*/


/* This is the machine-independent implementation of reduce. The algorithm is:

   Algorithm: MPI_Reduce

   For long messages and for builtin ops and if count >= pof2 (where
   pof2 is the nearest power-of-two less than or equal to the number
   of processes), we use Rabenseifner's algorithm (see
   http://www.hlrs.de/organization/par/services/models/mpi/myreduce.html).
   This algorithm implements the reduce in two steps: first a
   reduce-scatter, followed by a gather to the root. A
   recursive-halving algorithm (beginning with processes that are
   distance 1 apart) is used for the reduce-scatter, and a binomial tree
   algorithm is used for the gather. The non-power-of-two case is
   handled by dropping to the nearest lower power-of-two: the first
   few odd-numbered processes send their data to their left neighbors
   (rank-1), and the reduce-scatter happens among the remaining
   power-of-two processes. If the root is one of the excluded
   processes, then after the reduce-scatter, rank 0 sends its result to
   the root and exits; the root now acts as rank 0 in the binomial tree
   algorithm for gather.

   For the power-of-two case, the cost for the reduce-scatter is
   lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma. The cost for the
   gather to root is lgp.alpha + n.((p-1)/p).beta. Therefore, the
   total cost is:
   Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma

   For the non-power-of-two case, assuming the root is not one of the
   odd-numbered processes that get excluded in the reduce-scatter,
   Cost = (2.floor(lgp)+1).alpha + (2.((p-1)/p) + 1).n.beta +
           n.(1+(p-1)/p).gamma


   For short messages, user-defined ops, and count < pof2, we use a
   binomial tree algorithm for both short and long messages.

   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma


   We use the binomial tree algorithm in the case of user-defined ops
   because in this case derived datatypes are allowed, and the user
   could pass basic datatypes on one process and derived on another as
   long as the type maps are the same. Breaking up derived datatypes
   to do the reduce-scatter is tricky.

   FIXME: Per the MPI-2.1 standard this case is not possible.  We
   should be able to use the reduce-scatter/gather approach as long as
   count >= pof2.  [goodell@ 2009-01-21]

   Possible improvements:

   End Algorithm: MPI_Reduce
*/



int MPIR_Reduce_allcomm_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                             MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__REDUCE,
        .comm_ptr = comm_ptr,

        .u.reduce.sendbuf = sendbuf,
        .u.reduce.recvbuf = recvbuf,
        .u.reduce.count = count,
        .u.reduce.datatype = datatype,
        .u.reduce.op = op,
        .u.reduce.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_binomial:
            mpi_errno =
                MPIR_Reduce_intra_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                           errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_reduce_scatter_gather:
            mpi_errno =
                MPIR_Reduce_intra_reduce_scatter_gather(sendbuf, recvbuf, count, datatype, op, root,
                                                        comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_intra_smp:
            mpi_errno =
                MPIR_Reduce_intra_smp(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                      errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_inter_local_reduce_remote_send:
            mpi_errno =
                MPIR_Reduce_inter_local_reduce_remote_send(sendbuf, recvbuf, count, datatype, op,
                                                           root, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_allcomm_nb:
            mpi_errno =
                MPIR_Reduce_allcomm_nb(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                       errflag);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Reduce_impl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                     MPI_Datatype datatype, MPI_Op op, int root,
                     MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_REDUCE_INTRA_ALGORITHM) {
            case MPIR_CVAR_REDUCE_INTRA_ALGORITHM_binomial:
                mpi_errno = MPIR_Reduce_intra_binomial(sendbuf, recvbuf,
                                                       count, datatype, op, root, comm_ptr,
                                                       errflag);
                break;
            case MPIR_CVAR_REDUCE_INTRA_ALGORITHM_reduce_scatter_gather:
                mpi_errno = MPIR_Reduce_intra_reduce_scatter_gather(sendbuf, recvbuf,
                                                                    count, datatype, op, root,
                                                                    comm_ptr, errflag);
                break;
            case MPIR_CVAR_REDUCE_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Reduce_allcomm_nb(sendbuf, recvbuf,
                                                   count, datatype, op, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_REDUCE_INTRA_ALGORITHM_smp:
                mpi_errno =
                    MPIR_Reduce_intra_smp(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                          errflag);
                break;
            case MPIR_CVAR_REDUCE_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Reduce_allcomm_auto(sendbuf, recvbuf,
                                                     count, datatype, op, root, comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_REDUCE_INTER_ALGORITHM) {
            case MPIR_CVAR_REDUCE_INTER_ALGORITHM_local_reduce_remote_send:
                mpi_errno =
                    MPIR_Reduce_inter_local_reduce_remote_send(sendbuf, recvbuf, count, datatype,
                                                               op, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_REDUCE_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Reduce_allcomm_nb(sendbuf, recvbuf,
                                                   count, datatype, op, root, comm_ptr, errflag);
                break;
            case MPIR_CVAR_REDUCE_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Reduce_allcomm_auto(sendbuf, recvbuf, count, datatype,
                                                     op, root, comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Reduce(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    void *in_recvbuf = recvbuf;
    void *host_sendbuf;
    void *host_recvbuf;

    MPIR_Coll_host_buffer_alloc(sendbuf, recvbuf, count, datatype, &host_sendbuf, &host_recvbuf);
    if (host_sendbuf)
        sendbuf = host_sendbuf;
    if (host_recvbuf)
        recvbuf = host_recvbuf;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_REDUCE_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                     errflag);
    }

    /* Copy out data from host recv buffer to GPU buffer */
    if (host_recvbuf) {
        recvbuf = in_recvbuf;
        MPIR_Localcopy(host_recvbuf, count, datatype, recvbuf, count, datatype);
    }

    MPIR_Coll_host_buffer_free(host_sendbuf, host_recvbuf);

    return mpi_errno;
}
