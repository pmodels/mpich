/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "ibcast.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../ibcast/ibcast_tsp_tree_algos_prototypes.h"
#include "../ibcast/ibcast_tsp_scatterv_allgatherv_algos_prototypes.h"

/*
*/

/* Provides a "flat" broadcast that doesn't know anything about
 * hierarchy.  It will choose between several different algorithms
 * based on the given parameters. */

int MPIR_Ibcast_allcomm_sched_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                   int root, MPIR_Comm * comm_ptr,
                                   bool is_persistent, void **sched_p,
                                   enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IBCAST,
        .comm_ptr = comm_ptr,

        .u.ibcast.buffer = buffer,
        .u.ibcast.count = count,
        .u.ibcast.datatype = datatype,
        .u.ibcast.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_gentran_tree:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                 cnt->u.ibcast.intra_gentran_tree.tree_type,
                                 cnt->u.ibcast.intra_gentran_tree.k,
                                 cnt->u.ibcast.intra_gentran_tree.chunk_size,
                                 *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_gentran_scatterv_recexch_allgatherv:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno = MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype,
                                root, comm_ptr,
                                cnt->u.ibcast.intra_gentran_scatterv_recexch_allgatherv.scatterv_k,
                                cnt->u.ibcast.intra_gentran_scatterv_recexch_allgatherv.allgatherv_k,
                                *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_gentran_ring:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype,
                                root, comm_ptr,
                                MPIR_TREE_TYPE_KARY, 1,
                                cnt->u.ibcast.intra_gentran_ring.chunk_size,
                                *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_binomial:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_intra_sched_binomial(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_scatter_ring_allgather:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_intra_sched_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_sched_smp:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_intra_sched_smp(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_inter_sched_flat:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibcast_inter_sched_flat(buffer, count, datatype, root, comm_ptr, *sched_p);
            MPIR_ERR_CHECK(mpi_errno);
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

int MPIR_Ibcast_sched_impl(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                           MPIR_Comm * comm_ptr, bool is_persistent,
                           void **sched_p, enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    /* *INDENT-OFF* */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_IBCAST_INTRA_ALGORITHM) {
            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_tree:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype,
                                                    root, comm_ptr,
                                                    MPIR_Ibcast_tree_type,
                                                    MPIR_CVAR_IBCAST_TREE_KVAL,
                                                    MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE,
                                                    *sched_p);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_scatterv_recexch_allgatherv:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno = MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype,
                                                    root, comm_ptr,
                                                    MPIR_CVAR_IBCAST_SCATTERV_KVAL,
                                                    MPIR_CVAR_IBCAST_ALLGATHERV_RECEXCH_KVAL,
                                                    *sched_p);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_ring:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype,
                                                    root, comm_ptr,
                                                    MPIR_TREE_TYPE_KARY, 1,
                                                    MPIR_CVAR_IBCAST_RING_CHUNK_SIZE,
                                                    *sched_p);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_sched_binomial:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_intra_sched_binomial(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_sched_smp:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_intra_sched_smp(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_sched_scatter_recursive_doubling_allgather:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_sched_scatter_ring_allgather:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_intra_sched_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Ibcast_allcomm_sched_auto(buffer, count, datatype, root, comm_ptr,
                                                           is_persistent, sched_p, sched_type_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_IBCAST_INTER_ALGORITHM) {
            case MPIR_CVAR_IBCAST_INTER_ALGORITHM_sched_flat:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibcast_inter_sched_flat(buffer, count, datatype, root, comm_ptr, *sched_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            case MPIR_CVAR_IBCAST_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Ibcast_allcomm_sched_auto(buffer, count, datatype, root, comm_ptr,
                                                           is_persistent, sched_p, sched_type_p);
                MPIR_ERR_CHECK(mpi_errno);
                break;

            default:
                MPIR_Assert(0);
        }
    }
    /* *INDENT-ON* */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ibcast_impl(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                     MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Ibcast_sched_impl(buffer, count, datatype, root, comm_ptr,
                                       false, &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ibcast(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
                MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IBCAST_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Ibcast(buffer, count, datatype, root, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ibcast_impl(buffer, count, datatype, root, comm_ptr, request);
    }

    return mpi_errno;
}
