/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "tsp_gentran_types.h"
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "utlist.h"

/* UT_icd helper structure for vtx_t utarray */
UT_icd vtx_t_icd = {
    sizeof(MPII_Genutil_vtx_t),
    NULL,
    MPII_Genutil_vtx_copy,
    MPII_Genutil_vtx_dtor
};

/* UT_icd helper structure for MPII_Genutil_vtx_type_t utarray */
UT_icd vtx_type_t_icd = {
    sizeof(MPII_Genutil_vtx_type_t),
    NULL,
    NULL,
    NULL
};

int MPII_Genutil_sched_create(MPII_Genutil_sched_t * sched, bool is_persistent)
{
    sched->total_vtcs = 0;
    sched->completed_vtcs = 0;
    sched->last_fence = -1;

    /* initialize array for storing vertices */
    utarray_new(sched->vtcs, &vtx_t_icd, MPL_MEM_COLL);

    utarray_new(sched->buffers, &ut_ptr_icd, MPL_MEM_COLL);
    utarray_init(&sched->generic_types, &vtx_type_t_icd);

    sched->issued_head = NULL;
    sched->issued_tail = NULL;

    sched->is_persistent = is_persistent;

    return MPI_SUCCESS;
}

void MPII_Genutil_sched_free(MPII_Genutil_sched_t * sched)
{
    int i;
    vtx_t *vtx;
    void **p;

    /* free up the sched resources */
    vtx = ut_type_array(sched->vtcs, vtx_t *);
    MPII_Genutil_vtx_type_t *vtype =
        ut_type_array(&sched->generic_types, MPII_Genutil_vtx_type_t *);
    for (i = 0; i < sched->total_vtcs; i++, vtx++) {
        MPIR_Assert(vtx != NULL);
        if (vtx->vtx_kind == MPII_GENUTIL_VTX_KIND__IMCAST) {
            MPL_free(vtx->u.imcast.req);
            utarray_free(vtx->u.imcast.dests);
        } else if (vtx->vtx_kind == MPII_GENUTIL_VTX_KIND__SCHED) {
            /* In normal case, sub schedule is free'ed when it is done
             * only when the sub-scheduler is persistent */
            if (vtx->u.sched.is_persistent) {
                MPII_Genutil_sched_free(vtx->u.sched.sched);
            }
        } else if (vtx->vtx_kind > MPII_GENUTIL_VTX_KIND__LAST) {
            MPII_Genutil_vtx_type_t *type = vtype + vtx->vtx_kind - MPII_GENUTIL_VTX_KIND__LAST - 1;
            MPIR_Assert(type != NULL);
            if (type->free_fn != NULL) {
                int mpi_errno = type->free_fn(vtx);
                /* TODO: change this function to return mpi_errno */
                MPIR_Assert(mpi_errno == MPI_SUCCESS);
            }
        }
    }

    /* free up the allocated buffers */
    p = NULL;
    while ((p = (void **) utarray_next(sched->buffers, p)))
        MPL_free(*p);

    utarray_free(sched->vtcs);
    utarray_free(sched->buffers);
    utarray_done(&sched->generic_types);
    MPL_free(sched);
}

int MPII_Genutil_sched_new_type(MPII_Genutil_sched_t * sched, MPII_Genutil_sched_issue_fn issue_fn,
                                MPII_Genutil_sched_complete_fn complete_fn,
                                MPII_Genutil_sched_free_fn free_fn)
{
    MPII_Genutil_vtx_type_t *newtype;
    int type_id;

    type_id = MPII_GENUTIL_VTX_KIND__LAST + utarray_len(&sched->generic_types) + 1;
    utarray_extend_back(&sched->generic_types, MPL_MEM_COLL);
    newtype = (MPII_Genutil_vtx_type_t *) utarray_back(&sched->generic_types);
    newtype->id = type_id;
    newtype->issue_fn = issue_fn;
    newtype->complete_fn = complete_fn;
    newtype->free_fn = free_fn;

    return type_id;
}

int MPII_Genutil_sched_generic(int type_id, void *data,
                               MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs,
                               int *vtx_id)
{
    int mpi_errno = MPI_SUCCESS;
    vtx_t *vtxp;

    if (type_id <= MPII_GENUTIL_VTX_KIND__LAST ||
        type_id - MPII_GENUTIL_VTX_KIND__LAST > utarray_len(&sched->generic_types)) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }
    MPIR_Assert(vtx_id);
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);
    vtxp->vtx_kind = type_id;
    vtxp->u.generic.data = data;
    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] generic [%d]", *vtx_id, type_id));

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_Genutil_sched_isend(const void *buf,
                             int count,
                             MPI_Datatype dt,
                             int dest,
                             int tag,
                             MPIR_Comm * comm_ptr,
                             MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs)
{
    vtx_t *vtxp;
    int vtx_id;

    /* assign a new vertex */
    vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);
    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__ISEND;
    MPII_Genutil_vtx_add_dependencies(sched, vtx_id, n_in_vtcs, in_vtcs);

    /* store the arguments */
    vtxp->u.isend.buf = buf;
    vtxp->u.isend.count = count;
    vtxp->u.isend.dt = dt;
    vtxp->u.isend.dest = dest;
    vtxp->u.isend.tag = tag;
    vtxp->u.isend.comm = comm_ptr;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] isend", vtx_id));

    return vtx_id;
}


int MPII_Genutil_sched_irecv(void *buf,
                             int count,
                             MPI_Datatype dt,
                             int source,
                             int tag,
                             MPIR_Comm * comm_ptr,
                             MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs)
{
    vtx_t *vtxp;
    int vtx_id;

    /* assign a new vertex */
    vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__IRECV;
    MPII_Genutil_vtx_add_dependencies(sched, vtx_id, n_in_vtcs, in_vtcs);

    /* record the arguments */
    vtxp->u.irecv.buf = buf;
    vtxp->u.irecv.count = count;
    vtxp->u.irecv.dt = dt;
    vtxp->u.irecv.src = source;
    vtxp->u.irecv.tag = tag;
    vtxp->u.irecv.comm = comm_ptr;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] irecv", vtx_id));

    return vtx_id;
}


int MPII_Genutil_sched_imcast(const void *buf,
                              int count,
                              MPI_Datatype dt,
                              UT_array * dests,
                              int num_dests,
                              int tag,
                              MPIR_Comm * comm_ptr,
                              MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs)
{
    vtx_t *vtxp;
    int vtx_id;

    /* assign a new vertex */
    vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);
    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__IMCAST;
    MPII_Genutil_vtx_add_dependencies(sched, vtx_id, n_in_vtcs, in_vtcs);

    /* store the arguments */
    vtxp->u.imcast.buf = (void *) buf;
    vtxp->u.imcast.count = count;
    vtxp->u.imcast.dt = dt;
    vtxp->u.imcast.num_dests = num_dests;
    utarray_new(vtxp->u.imcast.dests, &ut_int_icd, MPL_MEM_COLL);
    utarray_concat(vtxp->u.imcast.dests, dests, MPL_MEM_COLL);
    vtxp->u.imcast.tag = tag;
    vtxp->u.imcast.comm = comm_ptr;
    vtxp->u.imcast.req =
        (struct MPIR_Request **) MPL_malloc(sizeof(struct MPIR_Request *) * num_dests,
                                            MPL_MEM_COLL);
    vtxp->u.imcast.last_complete = -1;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] imcast", vtx_id));
    return vtx_id;
}

int MPII_Genutil_sched_issend(const void *buf,
                              int count,
                              MPI_Datatype dt,
                              int dest,
                              int tag,
                              MPIR_Comm * comm_ptr,
                              MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs)
{
    vtx_t *vtxp;
    int vtx_id;

    /* assign a new vertex */
    vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);
    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__ISSEND;
    MPII_Genutil_vtx_add_dependencies(sched, vtx_id, n_in_vtcs, in_vtcs);

    /* store the arguments */
    vtxp->u.issend.buf = buf;
    vtxp->u.issend.count = count;
    vtxp->u.issend.dt = dt;
    vtxp->u.issend.dest = dest;
    vtxp->u.issend.tag = tag;
    vtxp->u.issend.comm = comm_ptr;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] issend", vtx_id));

    return vtx_id;
}

int MPII_Genutil_sched_reduce_local(const void *inbuf, void *inoutbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, MPII_Genutil_sched_t * sched,
                                    int n_in_vtcs, int *in_vtcs)
{
    vtx_t *vtxp;
    int vtx_id;

    /* assign a new vertex */
    vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__REDUCE_LOCAL;
    MPII_Genutil_vtx_add_dependencies(sched, vtx_id, n_in_vtcs, in_vtcs);

    /* record the arguments */
    vtxp->u.reduce_local.inbuf = inbuf;
    vtxp->u.reduce_local.inoutbuf = inoutbuf;
    vtxp->u.reduce_local.count = count;
    vtxp->u.reduce_local.datatype = datatype;
    vtxp->u.reduce_local.op = op;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] reduce_local", vtx_id));

    return vtx_id;
}


int MPII_Genutil_sched_localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                 MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs)
{
    vtx_t *vtxp;
    int vtx_id;

    /* assign a new vertex */
    vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__LOCALCOPY;
    MPII_Genutil_vtx_add_dependencies(sched, vtx_id, n_in_vtcs, in_vtcs);

    /* record the arguments */
    vtxp->u.localcopy.sendbuf = sendbuf;
    vtxp->u.localcopy.sendcount = sendcount;
    vtxp->u.localcopy.sendtype = sendtype;
    vtxp->u.localcopy.recvbuf = recvbuf;
    vtxp->u.localcopy.recvcount = recvcount;
    vtxp->u.localcopy.recvtype = recvtype;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] localcopy", vtx_id));

    return vtx_id;
}


/* Transport function that adds a no op vertex in the graph that has
 * all the vertices posted before it as incoming vertices */
int MPII_Genutil_sched_sink(MPII_Genutil_sched_t * sched)
{
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: sched [sink] total=%d", sched->total_vtcs));

    vtx_t *vtxp;
    int i, n_in_vtcs = 0, vtx_id;
    int *in_vtcs;
    int mpi_errno = MPI_SUCCESS;

    MPIR_CHKLMEM_DECL(1);

    /* assign a new vertex */
    vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__SINK;

    MPIR_CHKLMEM_MALLOC(in_vtcs, int *, sizeof(int) * vtx_id,
                        mpi_errno, "in_vtcs buffer", MPL_MEM_COLL);
    /* record incoming vertices */
    for (i = vtx_id - 1; i >= 0; i--) {
        vtx_t *sched_fence = (vtx_t *) utarray_eltptr(sched->vtcs, i);
        MPIR_Assert(sched_fence != NULL);
        if (sched_fence->vtx_kind == MPII_GENUTIL_VTX_KIND__FENCE)
            /* no need to record this and any vertex before fence.
             * Dependency on the last fence call will be added by
             * the subsequent call to MPIC_Genutil_vtx_add_dependencies function */
            break;
        else {
            in_vtcs[vtx_id - 1 - i] = i;
            n_in_vtcs++;
        }
    }

    MPII_Genutil_vtx_add_dependencies(sched, vtx_id, n_in_vtcs, in_vtcs);
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return vtx_id;
  fn_fail:
    /* TODO - Replace this with real error handling */
    MPIR_Assert(MPI_SUCCESS == mpi_errno);
    goto fn_exit;
}

void MPII_Genutil_sched_fence(MPII_Genutil_sched_t * sched)
{
    int fence_id;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Gentran: scheduling a fence"));

    /* fence operation is an extension to fence, so we can resuse the fence call */
    fence_id = MPII_Genutil_sched_sink(sched);
    /* change the vertex kind from SINK to FENCE */
    vtx_t *sched_fence = (vtx_t *) utarray_eltptr(sched->vtcs, fence_id);
    MPIR_Assert(sched_fence != NULL);
    sched_fence->vtx_kind = MPII_GENUTIL_VTX_KIND__FENCE;
    sched->last_fence = fence_id;
}

int MPII_Genutil_sched_selective_sink(MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs)
{
    vtx_t *vtxp;
    int vtx_id;

    /* assign a new vertex */
    vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__SELECTIVE_SINK;
    MPII_Genutil_vtx_add_dependencies(sched, vtx_id, n_in_vtcs, in_vtcs);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] selective_sink task", vtx_id));

    return vtx_id;

}

void *MPII_Genutil_sched_malloc(size_t size, MPII_Genutil_sched_t * sched)
{
    void *addr = MPL_malloc(size, MPL_MEM_COLL);
    utarray_push_back(sched->buffers, &addr, MPL_MEM_COLL);
    return addr;
}

int MPII_Genutil_sched_start(MPII_Genutil_sched_t * sched, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    int is_complete;
    int made_progress;
    MPIR_Request *reqp;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPII_GENUTIL_SCHED_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPII_GENUTIL_SCHED_START);

    /* Create a request */
    reqp = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!reqp)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    *req = reqp;
    MPIR_Request_add_ref(reqp);

    if (unlikely(sched->total_vtcs == 0)) {
        if (!sched->is_persistent)
            MPII_Genutil_sched_free(sched);
        MPIR_Request_complete(reqp);
        goto fn_exit;
    }
    MPIR_Assert(sched->completed_vtcs == 0);
    /* Kick start progress on this collective's schedule */
    mpi_errno = MPII_Genutil_sched_poke(sched, &is_complete, &made_progress);
    if (is_complete) {
        MPIR_Request_complete(reqp);
        goto fn_exit;
    }

    /* Enqueue schedule and activate progress hook if not already activated */
    reqp->u.nbc.coll.sched = (void *) sched;

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_TSP_QUEUE_MUTEX);
    DL_APPEND(MPII_coll_queue.head, &(reqp->u.nbc.coll));
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_TSP_QUEUE_MUTEX);

    MPIR_Progress_hook_activate(MPII_Genutil_progress_hook_id);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPII_GENUTIL_SCHED_START);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
