/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../ireduce/ireduce_tsp_tree_algos_prototypes.h"

/*
*/

int MPIR_Ireduce_allcomm_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                    MPI_Datatype datatype, MPI_Op op, int root,
                                    MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                    enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IREDUCE,
        .comm_ptr = comm_ptr,

        .u.ireduce.sendbuf = sendbuf,
        .u.ireduce.recvbuf = recvbuf,
        .u.ireduce.count = count,
        .u.ireduce.datatype = datatype,
        .u.ireduce.op = op,
        .u.ireduce.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_gentran_tree:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op, root,
                                                  comm_ptr,
                                                  cnt->u.ireduce.intra_gentran_tree.tree_type,
                                                  cnt->u.ireduce.intra_gentran_tree.k,
                                                  cnt->u.ireduce.intra_gentran_tree.chunk_size,
                                                  cnt->u.ireduce.
                                                  intra_gentran_tree.buffer_per_child, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_gentran_ring:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op, root,
                                                  comm_ptr, MPIR_TREE_TYPE_KARY, 1,
                                                  cnt->u.ireduce.intra_gentran_ring.chunk_size,
                                                  cnt->u.ireduce.
                                                  intra_gentran_ring.buffer_per_child, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_binomial:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ireduce_intra_sched_binomial(sendbuf, recvbuf, count, datatype, op,
                                                          root, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_reduce_scatter_gather:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ireduce_intra_sched_reduce_scatter_gather(sendbuf, recvbuf, count,
                                                                       datatype, op, root, comm_ptr,
                                                                       *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_sched_smp:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ireduce_intra_sched_smp(sendbuf, recvbuf, count, datatype, op, root,
                                                     comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_inter_sched_local_reduce_remote_send:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno =
                MPIR_Ireduce_inter_sched_local_reduce_remote_send(sendbuf, recvbuf, count, datatype,
                                                                  op, root, comm_ptr, *sched_p);
            break;

        default:
            MPIR_Assert(0);
        /* *INDENT-ON* */
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_sched_impl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                            MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                            bool is_persistent, void **sched_p, enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_IREDUCE_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_gentran_tree:
                /*Only knomial_1 tree supports non-commutative operations */
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, MPIR_Op_is_commutative(op) ||
                                               MPIR_Ireduce_tree_type == MPIR_TREE_TYPE_KNOMIAL_1,
                                               mpi_errno,
                                               "Ireduce gentran_tree cannot be applied.\n");
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op, root,
                                                      comm_ptr, MPIR_Ireduce_tree_type,
                                                      MPIR_CVAR_IREDUCE_TREE_KVAL,
                                                      MPIR_CVAR_IREDUCE_TREE_PIPELINE_CHUNK_SIZE,
                                                      MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_CHILD,
                                                      *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_gentran_ring:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ireduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op, root,
                                                      comm_ptr, MPIR_TREE_TYPE_KARY, 1,
                                                      MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE,
                                                      MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_CHILD,
                                                      *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_sched_binomial:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ireduce_intra_sched_binomial(sendbuf, recvbuf, count, datatype, op,
                                                              root, comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_sched_smp:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ireduce_intra_sched_smp(sendbuf, recvbuf, count, datatype, op,
                                                         root, comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_sched_reduce_scatter_gather:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ireduce_intra_sched_reduce_scatter_gather(sendbuf, recvbuf, count,
                                                                           datatype, op, root,
                                                                           comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ireduce_allcomm_sched_auto(sendbuf, recvbuf, count, datatype, op, root,
                                                    comm_ptr, is_persistent, sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_IREDUCE_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IREDUCE_INTER_ALGORITHM_sched_local_reduce_remote_send:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ireduce_inter_sched_local_reduce_remote_send(sendbuf, recvbuf,
                                                                              count, datatype, op,
                                                                              root, comm_ptr,
                                                                              *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ireduce_allcomm_sched_auto(sendbuf, recvbuf, count, datatype, op, root,
                                                    comm_ptr, is_persistent, sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    mpi_errno =
        MPIR_Ireduce_allcomm_sched_auto(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                        is_persistent, sched_p, sched_type_p);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_impl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                      MPI_Datatype datatype, MPI_Op op, int root,
                      MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Ireduce_sched_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                        false, &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce(const void *sendbuf, void *recvbuf, MPI_Aint count,
                 MPI_Datatype datatype, MPI_Op op, int root,
                 MPIR_Comm * comm_ptr, MPIR_Request ** request)
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
         MPIR_CVAR_IREDUCE_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ireduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                      request);
    }

    MPIR_Coll_host_buffer_swap_back(host_sendbuf, host_recvbuf, in_recvbuf, count, datatype,
                                    *request);

    return mpi_errno;
}
