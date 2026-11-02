/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "tsp_impl.h"

/* start a persistent operation, or start it again */
static int persist_sched_start(enum MPIR_sched_type sched_type, void *sched, MPIR_Comm * comm,
                               MPIR_Request ** request);
static void sched_free(enum MPIR_sched_type sched_type, void *sched);

int MPIR_Sched_cb_free_buf(MPIR_Comm * comm, int tag, void *state)
{
    MPL_free(state);
    return MPI_SUCCESS;
}

int MPIR_Persist_coll_start(MPIR_Request * preq)
{
    int mpi_errno = MPI_SUCCESS;

    preq->u.persist_coll.real_request = NULL;
    mpi_errno = persist_sched_start(preq->u.persist_coll.sched_type, preq->u.persist_coll.sched,
                                    preq->comm, &preq->u.persist_coll.real_request);
    MPIR_ERR_CHECK(mpi_errno);

    preq->status.MPI_ERROR = MPI_SUCCESS;
    preq->cc_ptr = &preq->u.persist_coll.real_request->cc;

  fn_exit:
    return mpi_errno;
  fn_fail:
    /* If a failure occurs attempting to start the request, then we
     * assume that partner request was not created, and stuff
     * the error code in the persistent request.  The wait and test
     * routines will look at the error code in the persistent
     * request if a partner request is not present. */
    preq->u.persist_coll.real_request = NULL;
    preq->status.MPI_ERROR = mpi_errno;
    preq->cc_ptr = &preq->cc;
    MPIR_cc_set(&preq->cc, 0);
    goto fn_exit;
}

void MPIR_Persist_coll_free_cb(MPIR_Request * request)
{
    if (request->u.persist_coll.sched) {
        sched_free(request->u.persist_coll.sched_type, request->u.persist_coll.sched);
    }
}

/* An inner call of a nonblocking compositional algorithm builds a schedule of its own, of whatever
 * type the selection gives. A COND entry in the parent's schedule starts it, waits for it to
 * complete, passes on its error, and frees it with the parent. */
static int add_inner_entry(MPIR_Sched_t s, MPIR_Csel_coll_sig_s * sub);

/* Selects the inner call described by sub, and adds to the schedule s an entry that runs it. The
 * caller's collective holds the handles that sub uses. */
int MPIR_Coll_sched_call_inner(MPIR_Sched_t s, MPIR_Csel_coll_sig_s * sub)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Coll_run_inner(sub);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(sub->sched);

    mpi_errno = add_inner_entry(s, sub);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

struct sched_inner {
    enum MPIR_sched_type sched_type;
    void *sched;
    MPIR_Comm *comm;
    MPIR_Request *req;
    bool is_persistent;
    bool is_started;
};

/* the callbacks of the COND entry */
static int sched_inner_start(void *state);
static int sched_inner_test(void *state, bool * done);
static void sched_inner_free(void *state);

/* allocate the state of the inner call from the schedule s and add its COND entry */
static int add_inner_entry(MPIR_Sched_t s, MPIR_Csel_coll_sig_s * sub)
{
    int mpi_errno = MPI_SUCCESS;

    struct sched_inner *inner = MPIR_Sched_alloc_state(s, sizeof(struct sched_inner));
    MPIR_ERR_CHKANDJUMP(!inner, mpi_errno, MPI_ERR_OTHER, "**nomem");

    inner->sched_type = sub->sched_type;
    inner->sched = sub->sched;
    inner->comm = sub->comm_ptr;
    inner->req = NULL;
    inner->is_persistent = sub->flags & MPIR_COLL_SIG_FLAG__PERSISTENT;
    inner->is_started = false;

    mpi_errno = MPIR_Sched_cond(sched_inner_start, sched_inner_test, sched_inner_free, inner, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int sched_inner_start(void *state)
{
    int mpi_errno = MPI_SUCCESS;
    struct sched_inner *inner = state;

    if (inner->is_persistent) {
        mpi_errno = persist_sched_start(inner->sched_type, inner->sched, inner->comm, &inner->req);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPII_SCHED_START(inner->sched_type, inner->sched, inner->comm, &inner->req);
    }
    inner->is_started = true;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int sched_inner_test(void *state, bool * done)
{
    int mpi_errno = MPI_SUCCESS;
    struct sched_inner *inner = state;

    if (MPIR_Request_is_complete(inner->req)) {
        mpi_errno = inner->req->status.MPI_ERROR;
        MPIR_Request_free(inner->req);
        inner->req = NULL;
        *done = true;
    }

    return mpi_errno;
}

static void sched_inner_free(void *state)
{
    struct sched_inner *inner = state;

    /* a regular schedule that started frees itself when it completes */
    if (inner->is_persistent || !inner->is_started) {
        sched_free(inner->sched_type, inner->sched);
    }
}


static int persist_sched_start(enum MPIR_sched_type sched_type, void *sched, MPIR_Comm * comm,
                               MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if (sched_type == MPIR_SCHED_NORMAL) {
        mpi_errno = MPIR_Sched_reset(sched);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Sched_start(sched, comm, request);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (sched_type == MPIR_SCHED_GENTRAN) {
        MPIR_TSP_sched_reset(sched);
        mpi_errno = MPIR_TSP_sched_start(sched, comm, request);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (sched_type == MPIR_SCHED_GREQ) {
        MPIR_Coll_greq_t *greq = sched;
        mpi_errno = greq->start_fn(greq->state, request);
        MPIR_ERR_CHECK(mpi_errno);
        if (*request == NULL) {
            /* nothing to do */
            *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
            MPIR_ERR_CHKANDJUMP(!*request, mpi_errno, MPIX_ERR_NOREQ, "**nomemreq");
        }
    } else {
        /* TODO: proper error return */
        MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void sched_free(enum MPIR_sched_type sched_type, void *sched)
{
    if (sched_type == MPIR_SCHED_NORMAL) {
        MPIR_Sched_free(sched);
    } else if (sched_type == MPIR_SCHED_GENTRAN) {
        MPIR_TSP_sched_free(sched);
    } else if (sched_type == MPIR_SCHED_GREQ) {
        MPIR_Coll_greq_free(sched);
    } else {
        /* TODO: proper error return */
        MPIR_Assert(0);
    }
}
