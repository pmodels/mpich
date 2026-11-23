/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"
#include "coll_impl.h"
#include "mpir_async_things.h"

/* Local copies around a schedule, for the adapters of nonblocking and persistent collectives
 * (in-place copy, GPU swap). The adapter lets the selection build the algorithm's schedule, of
 * any type, and wraps it in a greq operation: each run makes the copies that go before, starts
 * the schedule, and an async thing waits for it to complete and makes the copies that go after.
 * The adapter thus adds no steps to the schedule itself.
 *
 * The copies run when the operation starts, so an inner call of a composition reads its input
 * only when the parent reaches it, and a persistent operation copies at every start. */

#define MAX_WRAP_COPIES 4

struct wrap_copy {
    const void *src;
    MPI_Aint src_count;
    MPI_Datatype src_type;
    void *dst;
    MPI_Aint dst_count;
    MPI_Datatype dst_type;
};

struct MPIR_Coll_wrap {
    MPIR_Coll_greq_t *greq;
    struct MPIR_Coll_inner inner;
    int n_before;
    int n_after;
    struct wrap_copy before[MAX_WRAP_COPIES];
    struct wrap_copy after[MAX_WRAP_COPIES];
    /* per run */
    MPIR_Request *req;
};

static int wrap_start(void *state, MPIR_Request ** req);
static void wrap_free(void *state);

int MPIR_Coll_wrap_sched(MPIR_Csel_coll_sig_s * coll_sig, struct MPIR_Coll_wrap **wrap_out)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(coll_sig->sched);
    struct MPIR_Coll_wrap *wrap = MPL_malloc(sizeof(struct MPIR_Coll_wrap), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!wrap, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIR_Coll_inner_init(&wrap->inner, coll_sig);
    wrap->n_before = 0;
    wrap->n_after = 0;
    wrap->req = NULL;

    coll_sig->sched = NULL;
    MPII_CSEL_CREATE_GREQ(coll_sig);
    wrap->greq = coll_sig->sched;
    wrap->greq->state = wrap;
    wrap->greq->start_fn = wrap_start;
    wrap->greq->free_fn = wrap_free;

    *wrap_out = wrap;

  fn_exit:
    return mpi_errno;
  fn_fail:
    if (wrap) {
        coll_sig->sched = wrap->inner.sched;
        MPL_free(wrap);
    }
    goto fn_exit;
}

void MPIR_Coll_wrap_add_copy(struct MPIR_Coll_wrap *wrap, bool after,
                             const void *src, MPI_Aint src_count, MPI_Datatype src_type,
                             void *dst, MPI_Aint dst_count, MPI_Datatype dst_type)
{
    struct wrap_copy *c;
    if (after) {
        MPIR_Assert(wrap->n_after < MAX_WRAP_COPIES);
        c = &wrap->after[wrap->n_after++];
    } else {
        MPIR_Assert(wrap->n_before < MAX_WRAP_COPIES);
        c = &wrap->before[wrap->n_before++];
    }
    c->src = src;
    c->src_count = src_count;
    c->src_type = src_type;
    c->dst = dst;
    c->dst_count = dst_count;
    c->dst_type = dst_type;
}

MPIR_Coll_owned_t *MPIR_Coll_wrap_get_owned(struct MPIR_Coll_wrap *wrap)
{
    return &wrap->greq->owned;
}

static int do_copies(struct wrap_copy *copies, int n);
static int wrap_poll(MPIX_Async_thing thing);

static int wrap_start(void *state, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIR_Coll_wrap *wrap = state;

    mpi_errno = do_copies(wrap->before, wrap->n_before);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Coll_inner_start(&wrap->inner);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Grequest_start_impl(NULL, NULL, NULL, NULL, req);
    MPIR_ERR_CHECK(mpi_errno);
    wrap->req = *req;

    mpi_errno = MPIR_Async_things_add(wrap_poll, wrap, NULL);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Completes the run once the schedule completes, with the error of the schedule or the copies */
static int wrap_poll(MPIX_Async_thing thing)
{
    struct MPIR_Coll_wrap *wrap = MPIR_Async_thing_get_state(thing);

    bool done = false;
    int mpi_errno = MPIR_Coll_inner_test(&wrap->inner, &done);
    if (!done) {
        return MPIX_ASYNC_NOPROGRESS;
    }

    int copy_errno = do_copies(wrap->after, wrap->n_after);
    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = copy_errno;
    }

    wrap->req->status.MPI_ERROR = mpi_errno;
    /* the last time the run touches wrap: the operation may be freed right after */
    MPIR_Grequest_complete_impl(wrap->req);
    return MPIX_ASYNC_DONE;
}

static int do_copies(struct wrap_copy *copies, int n)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 0; i < n; i++) {
        struct wrap_copy *c = &copies[i];
        mpi_errno = MPIR_Localcopy(c->src, c->src_count, c->src_type,
                                   c->dst, c->dst_count, c->dst_type);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void wrap_free(void *state)
{
    struct MPIR_Coll_wrap *wrap = state;

    MPIR_Coll_inner_free(&wrap->inner);
    MPL_free(wrap);
}
