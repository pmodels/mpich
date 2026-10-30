/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Coll_greq_create(MPIR_Coll_greq_t ** greq_out)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Coll_greq_t *greq = MPL_malloc(sizeof(MPIR_Coll_greq_t), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!greq, mpi_errno, MPI_ERR_OTHER, "**nomem");

    greq->state = NULL;
    greq->start_fn = NULL;
    greq->free_fn = NULL;
    MPIR_Coll_owned_init(&greq->owned);

    *greq_out = greq;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIR_Coll_greq_free(MPIR_Coll_greq_t * greq)
{
    if (greq->free_fn) {
        greq->free_fn(greq->state);
    }
    MPIR_Coll_owned_free(&greq->owned);
    MPL_free(greq);
}

/* the request of a nonblocking run frees the operation when the user frees the request */
static int greq_free_fn(void *extra_state)
{
    MPIR_Coll_greq_free(extra_state);
    return MPI_SUCCESS;
}

int MPIR_Coll_greq_start(MPIR_Coll_greq_t * greq, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req = NULL;

    mpi_errno = greq->start_fn(greq->state, &req);
    MPIR_ERR_CHECK(mpi_errno);

    if (req == NULL) {
        /* nothing to do */
        MPIR_Coll_greq_free(greq);
        req = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
        MPIR_ERR_CHKANDJUMP(!req, mpi_errno, MPIX_ERR_NOREQ, "**nomemreq");
    } else {
        MPIR_Assert(req->kind == MPIR_REQUEST_KIND__GREQUEST);
        MPIR_Assert(req->u.ureq.greq_fns->U.C.free_fn == NULL);
        req->u.ureq.greq_fns->U.C.free_fn = greq_free_fn;
        req->u.ureq.greq_fns->grequest_extra_state = greq;
    }
    *request = req;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
