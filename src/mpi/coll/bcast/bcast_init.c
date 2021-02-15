/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpiimpl.h"
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../ibcast/ibcast_tsp_tree_algos_prototypes.h"
#include "../ibcast/ibcast_tsp_scatterv_allgatherv_algos_prototypes.h"

static int MPIR_Bcast_sched_intra_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                       int root, MPIR_Comm * comm_ptr, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                 MPIR_Ibcast_tree_type, MPIR_CVAR_IBCAST_TREE_KVAL,
                                                 MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE, sched);

    return mpi_errno;
}

int MPIR_Bcast_init_impl(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                         MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;

    /* create a new request */
    MPIR_Request *req = MPIR_Request_create(MPIR_REQUEST_KIND__PREQUEST_COLL);
    MPIR_ERR_CHKANDJUMP(!req, mpi_errno, MPI_ERR_OTHER, "**nomem");

    req->u.persist.real_request = NULL;

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!sched, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_TSP_sched_create(sched, true);

    switch (MPIR_CVAR_IBCAST_INTRA_ALGORITHM) {
        case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_tree:
            mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                         MPIR_Ibcast_tree_type,
                                                         MPIR_CVAR_IBCAST_TREE_KVAL,
                                                         MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE,
                                                         sched);
            break;
        case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_scatterv_recexch_allgatherv:
            mpi_errno =
                MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype, root,
                                                                comm_ptr,
                                                                MPIR_CVAR_IBCAST_SCATTERV_KVAL,
                                                                MPIR_CVAR_IBCAST_ALLGATHERV_RECEXCH_KVAL,
                                                                sched);
            break;
        case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_ring:
            mpi_errno =
                MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                 MPIR_TREE_TYPE_KARY, 1,
                                                 MPIR_CVAR_IBCAST_RING_CHUNK_SIZE, sched);
            break;
        default:
            mpi_errno = MPIR_Bcast_sched_intra_auto(buffer, count, datatype, root, comm_ptr, sched);
            break;
    }

    MPIR_ERR_CHECK(mpi_errno);
    req->u.persist.sched = sched;

    *request = req;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Bcast_init(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                    MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IBCAST_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Bcast_init(buffer, count, datatype, root, comm_ptr, info_ptr, request);
    } else {
        mpi_errno =
            MPIR_Bcast_init_impl(buffer, count, datatype, root, comm_ptr, info_ptr, request);
    }

    return mpi_errno;
}
