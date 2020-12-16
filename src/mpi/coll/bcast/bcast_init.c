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

static int MPIR_Bcast_sched_intra_auto(void *buffer, int count, MPI_Datatype datatype,
                                       int root, MPIR_Comm * comm_ptr, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                 MPIR_Ibcast_tree_type, MPIR_CVAR_IBCAST_TREE_KVAL,
                                                 MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE, sched);

    return mpi_errno;
}

/* -- Begin Profiling Symbol Block for routine MPIX_Bcast_init */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Bcast_init = PMPIX_Bcast_init
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Bcast_init  MPIX_Bcast_init
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Bcast_init as PMPIX_Bcast_init
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root,
                    MPI_Comm comm, MPI_Info info, MPI_Request * request)
    __attribute__ ((weak, alias("PMPIX_Bcast_init")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Bcast_init
#define MPIX_Bcast_init PMPIX_Bcast_init

int MPIR_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root,
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

#endif

/*@
MPIX_Bcast_init - Creates a nonblocking, persistent collective communication request for the broadcast operation.

Input/Output Parameters:
. buffer - starting address of buffer (choice)

Input Parameters:
+ count - number of entries in buffer (integer)
. datatype - data type of buffer (handle)
. root - rank of broadcast root (integer)
. comm - communicator (handle)
. info - info argument (handle)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_ROOT

.seealso: MPI_Start, MPI_Startall, MPI_Request_free
@*/
int MPIX_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root,
                    MPI_Comm comm, MPI_Info info, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Info *info_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIX_BCAST_INIT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER(MPID_STATE_MPIX_BCAST_INIT);

    /* Validate handle parameters needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);
    MPIR_Info_get_ptr(info, info_ptr);

    /* Validate parameters if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            MPIR_Info_valid_ptr(info_ptr, mpi_errno);

            if (mpi_errno)
                goto fn_fail;

            MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);

            /* Validate datatype handle */
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
            } else {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);
            }

            /* Validate datatype object */
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *datatype_ptr = NULL;

                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
            }

            MPIR_ERRTEST_BUF_INPLACE(buffer, count, mpi_errno);
            /* Validate buffer */
            MPIR_ERRTEST_USERBUFFER(buffer, count, datatype, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (MPIR_CVAR_IBCAST_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES)
        mpi_errno =
            MPID_Bcast_init(buffer, count, datatype, root, comm_ptr, info_ptr, &request_ptr);
    else
        mpi_errno =
            MPIR_Bcast_init(buffer, count, datatype, root, comm_ptr, info_ptr, &request_ptr);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* return the handle of the request to the user */
    MPIR_OBJ_PUBLISH_HANDLE(*request, request_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_PT2PT_EXIT(MPID_STATE_MPIX_BCAST_INIT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpix_bcast_init", "**mpix_bcast_init %p %d %D %d %C %I %p",
                                 buffer, count, datatype, root, comm, info, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
