/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "coll_impl.h"
#include "coll_csel.h"

/* Persistent collectives with a request-based algorithm.
 *
 * A request-based algorithm (a nonblocking algorithm that is neither sched_ nor tsp_, e.g.
 * ibcast circ_graph) sets up and starts the whole operation in one call and returns its own
 * request, so there is no schedule to build at init and replay at start. Instead, the
 * persistent init saves the selection (the coll_sig, the algorithm container, and the
 * algorithm's _cnt wrapper) as the "schedule" of type MPIR_SCHED_REQUEST, and every start
 * runs the algorithm again.
 *
 * The saved coll_sig holds a reference on its communicator, datatypes and op, since the user
 * may free them after the init. */

static void add_ref_fn(int handle, void *arg)
{
    MPIR_Handle_add_ref(handle);
}

static void release_fn(int handle, void *arg)
{
    MPIR_Handle_release(handle);
}

struct persist_request {
    MPIR_Csel_coll_sig_s coll_sig;
    MPII_Csel_container_s cnt;
    MPIR_Coll_algo_fn fn;
};

/* Called from the generated _cnt wrapper of a request-based algorithm in place of the
 * algorithm, when the PERSISTENT flag is set. */
int MPIR_Coll_persist_request_save(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt,
                                   MPIR_Coll_algo_fn fn)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(coll_sig->sched == NULL);

    struct persist_request *p = MPL_malloc(sizeof(struct persist_request), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    p->coll_sig = *coll_sig;
    /* every start runs fn as a nonblocking collective */
    p->coll_sig.flags &= ~MPIR_COLL_SIG_FLAG__PERSISTENT;
    p->cnt = *cnt;
    p->fn = fn;
    MPIR_Coll_sig_foreach_handle(&p->coll_sig, add_ref_fn, NULL);

    coll_sig->sched_type = MPIR_SCHED_REQUEST;
    coll_sig->sched = p;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Runs the saved algorithm, and returns the request it creates in *req_out. An algorithm
 * that has nothing to do (e.g. a single process) returns no request; a completed one stands
 * in for it. */
int MPIR_Coll_persist_request_start(void *sched, MPIR_Request ** req_out)
{
    int mpi_errno = MPI_SUCCESS;
    struct persist_request *p = sched;

    /* the algorithm may modify the coll_sig; run it on a copy */
    MPIR_Csel_coll_sig_s coll_sig = p->coll_sig;
    coll_sig.sched = NULL;

    mpi_errno = p->fn(&coll_sig, &p->cnt);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Assert(coll_sig.sched_type == MPIR_SCHED_REQUEST);
    if (coll_sig.sched) {
        *req_out = coll_sig.sched;
    } else {
        *req_out = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIR_Coll_persist_request_free(void *sched)
{
    struct persist_request *p = sched;

    MPIR_Coll_sig_foreach_handle(&p->coll_sig, release_fn, NULL);
    MPL_free(p);
}
