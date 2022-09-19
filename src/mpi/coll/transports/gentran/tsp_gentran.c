/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "tsp_impl.h"
#include "gentran_impl.h"

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

int MPIR_TSP_sched_create(MPIR_TSP_sched_t * s, bool is_persistent)
{
    MPII_Genutil_sched_t *sched = MPL_malloc(sizeof(MPII_Genutil_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);

    sched->total_vtcs = 0;
    sched->completed_vtcs = 0;
    sched->last_fence = -1;

    /* initialize array for storing vertices */
    utarray_init(&sched->vtcs, &vtx_t_icd);

    utarray_init(&sched->buffers, &ut_ptr_icd);
    utarray_init(&sched->generic_types, &vtx_type_t_icd);

    sched->issued_head = NULL;
    sched->issued_tail = NULL;

    sched->is_persistent = is_persistent;

    *s = sched;
    return MPI_SUCCESS;
}

int MPIR_TSP_sched_free(MPIR_TSP_sched_t s)
{
    MPII_Genutil_sched_t *sched = s;
    int mpi_errno ATTRIBUTE((unused)) = MPI_SUCCESS;
    int i;
    vtx_t *vtx;
    void **p;

    /* free up the sched resources */
    vtx = ut_type_array(&sched->vtcs, vtx_t *);
    MPII_Genutil_vtx_type_t *vtype =
        ut_type_array(&sched->generic_types, MPII_Genutil_vtx_type_t *);
    for (i = 0; i < sched->total_vtcs; i++, vtx++) {
        MPIR_ERR_CHKANDJUMP(vtx == NULL, mpi_errno, MPI_ERR_OTHER, "**nullvertex");
        switch (vtx->vtx_kind) {
            case MPII_GENUTIL_VTX_KIND__ISEND:
                MPIR_Comm_release(vtx->u.isend.comm);
                MPIR_Datatype_release_if_not_builtin(vtx->u.isend.dt);
                break;
            case MPII_GENUTIL_VTX_KIND__ISSEND:
                MPIR_Comm_release(vtx->u.issend.comm);
                MPIR_Datatype_release_if_not_builtin(vtx->u.issend.dt);
                break;
            case MPII_GENUTIL_VTX_KIND__IRECV:
                MPIR_Comm_release(vtx->u.irecv.comm);
                MPIR_Datatype_release_if_not_builtin(vtx->u.irecv.dt);
                break;
            case MPII_GENUTIL_VTX_KIND__IRECV_STATUS:
                MPIR_Comm_release(vtx->u.irecv_status.comm);
                MPIR_Datatype_release_if_not_builtin(vtx->u.irecv_status.dt);
                break;
            case MPII_GENUTIL_VTX_KIND__IMCAST:
                MPIR_Comm_release(vtx->u.imcast.comm);
                MPIR_Datatype_release_if_not_builtin(vtx->u.imcast.dt);
                MPL_free(vtx->u.imcast.req);
                utarray_done(&vtx->u.imcast.dests);
                break;
            case MPII_GENUTIL_VTX_KIND__REDUCE_LOCAL:
                MPIR_Op_release_if_not_builtin(vtx->u.reduce_local.op);
                MPIR_Datatype_release_if_not_builtin(vtx->u.reduce_local.datatype);
                break;
            case MPII_GENUTIL_VTX_KIND__LOCALCOPY:
                MPIR_Datatype_release_if_not_builtin(vtx->u.localcopy.sendtype);
                MPIR_Datatype_release_if_not_builtin(vtx->u.localcopy.recvtype);
                break;
            case MPII_GENUTIL_VTX_KIND__SCHED:
                /* In normal case, sub schedule is free'ed when it is done
                 * only when the sub-scheduler is persistent */
                if (vtx->u.sched.is_persistent) {
                    mpi_errno = MPIR_TSP_sched_free(vtx->u.sched.sched);
                    MPIR_ERR_CHECK(mpi_errno);
                }
                break;
            default:
                if (vtx->vtx_kind > MPII_GENUTIL_VTX_KIND__LAST) {
                    MPII_Genutil_vtx_type_t *type =
                        vtype + vtx->vtx_kind - MPII_GENUTIL_VTX_KIND__LAST - 1;
                    MPIR_Assert(type != NULL);
                    if (type->free_fn != NULL) {
                        mpi_errno = type->free_fn(vtx->u.generic.data);
                        MPIR_ERR_CHECK(mpi_errno);
                    }
                }
        }
    }

    /* free up the allocated buffers */
    p = NULL;
    while ((p = (void **) utarray_next(&sched->buffers, p)))
        MPL_free(*p);

    utarray_done(&sched->vtcs);
    utarray_done(&sched->buffers);
    utarray_done(&sched->generic_types);
    MPL_free(sched);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_TSP_sched_new_type(MPIR_TSP_sched_t s, MPIR_TSP_sched_issue_fn issue_fn,
                            MPIR_TSP_sched_complete_fn complete_fn, MPIR_TSP_sched_free_fn free_fn)
{
    MPII_Genutil_sched_t *sched = s;
    MPII_Genutil_vtx_type_t *vtype, *newtype;
    int type_id, i;

    /* only free_fn can be NULL */
    MPIR_Assert(issue_fn && complete_fn);

    /* search if the new type already exists in the table */
    vtype = ut_type_array(&sched->generic_types, MPII_Genutil_vtx_type_t *);
    for (i = 0; i < utarray_len(&sched->generic_types); i++) {
        if (vtype->issue_fn == issue_fn && vtype->complete_fn == complete_fn &&
            vtype->free_fn == free_fn) {
            return MPII_GENUTIL_VTX_KIND__LAST + i + 1;
        }
        vtype++;
    }

    type_id = MPII_GENUTIL_VTX_KIND__LAST + utarray_len(&sched->generic_types) + 1;
    utarray_extend_back(&sched->generic_types, MPL_MEM_COLL);
    newtype = (MPII_Genutil_vtx_type_t *) utarray_back(&sched->generic_types);
    newtype->id = type_id;
    newtype->issue_fn = issue_fn;
    newtype->complete_fn = complete_fn;
    newtype->free_fn = free_fn;

    return type_id;
}

int MPIR_TSP_sched_generic(int type_id, void *data,
                           MPIR_TSP_sched_t s, int n_in_vtcs, int *in_vtcs, int *vtx_id)
{
    MPII_Genutil_sched_t *sched = s;
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

int MPIR_TSP_sched_isend(const void *buf,
                         MPI_Aint count,
                         MPI_Datatype dt,
                         int dest,
                         int tag,
                         MPIR_Comm * comm_ptr, MPIR_TSP_sched_t s, int n_in_vtcs, int *in_vtcs,
                         int *vtx_id)
{
    MPII_Genutil_sched_t *sched = s;
    vtx_t *vtxp;
    int mpi_errno = MPI_SUCCESS;
    /* assign a new vertex */
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);
    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__ISEND;
    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);

    /* store the arguments */
    vtxp->u.isend.buf = buf;
    vtxp->u.isend.count = count;
    vtxp->u.isend.dt = dt;
    vtxp->u.isend.dest = dest;
    vtxp->u.isend.tag = tag;
    vtxp->u.isend.comm = comm_ptr;

    /* the user may free the comm & type after initiating but before the
     * underlying send is actually posted, so we must add a reference here and
     * release it at entry completion time */
    MPIR_Comm_add_ref(comm_ptr);
    MPIR_Datatype_add_ref_if_not_builtin(dt);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] isend", *vtx_id));

    return mpi_errno;
}


int MPIR_TSP_sched_irecv(void *buf,
                         MPI_Aint count,
                         MPI_Datatype dt,
                         int source,
                         int tag,
                         MPIR_Comm * comm_ptr, MPIR_TSP_sched_t s, int n_in_vtcs, int *in_vtcs,
                         int *vtx_id)
{
    MPII_Genutil_sched_t *sched = s;
    vtx_t *vtxp;
    int mpi_errno = MPI_SUCCESS;
    /* assign a new vertex */
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__IRECV;
    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);

    /* record the arguments */
    vtxp->u.irecv.buf = buf;
    vtxp->u.irecv.count = count;
    vtxp->u.irecv.dt = dt;
    vtxp->u.irecv.src = source;
    vtxp->u.irecv.tag = tag;
    vtxp->u.irecv.comm = comm_ptr;

    MPIR_Comm_add_ref(comm_ptr);
    MPIR_Datatype_add_ref_if_not_builtin(dt);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] irecv", *vtx_id));

    return mpi_errno;
}

int MPIR_TSP_sched_irecv_status(void *buf,
                                MPI_Aint count,
                                MPI_Datatype dt,
                                int source,
                                int tag,
                                MPIR_Comm * comm_ptr, MPI_Status * status,
                                MPIR_TSP_sched_t sched, int n_in_vtcs, int *in_vtcs, int *vtx_id)
{
    vtx_t *vtxp;
    int mpi_errno = MPI_SUCCESS;;

    /* assign a new vertex */
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__IRECV_STATUS;
    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);

    /* record the arguments */
    vtxp->u.irecv_status.buf = buf;
    vtxp->u.irecv_status.count = count;
    vtxp->u.irecv_status.dt = dt;
    vtxp->u.irecv_status.src = source;
    vtxp->u.irecv_status.tag = tag;
    vtxp->u.irecv_status.comm = comm_ptr;
    vtxp->u.irecv_status.status = status;

    MPIR_Comm_add_ref(comm_ptr);
    MPIR_Datatype_add_ref_if_not_builtin(dt);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] irecv_status", *vtx_id));

    return mpi_errno;
}



int MPIR_TSP_sched_imcast(const void *buf,
                          MPI_Aint count,
                          MPI_Datatype dt,
                          int *dests,
                          int num_dests,
                          int tag,
                          MPIR_Comm * comm_ptr, MPIR_TSP_sched_t s, int n_in_vtcs, int *in_vtcs,
                          int *vtx_id)
{
    MPII_Genutil_sched_t *sched = s;
    vtx_t *vtxp;
    int mpi_errno = MPI_SUCCESS;
    /* assign a new vertex */
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);
    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__IMCAST;
    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);

    /* store the arguments */
    vtxp->u.imcast.buf = (void *) buf;
    vtxp->u.imcast.count = count;
    vtxp->u.imcast.dt = dt;
    vtxp->u.imcast.num_dests = num_dests;
    utarray_init(&vtxp->u.imcast.dests, &ut_int_icd);
    utarray_reserve(&vtxp->u.imcast.dests, num_dests, MPL_MEM_COLL);
    memcpy(ut_int_array(&vtxp->u.imcast.dests), dests, num_dests * sizeof(int));
    vtxp->u.imcast.tag = tag;
    vtxp->u.imcast.comm = comm_ptr;
    vtxp->u.imcast.req =
        (struct MPIR_Request **) MPL_malloc(sizeof(struct MPIR_Request *) * num_dests,
                                            MPL_MEM_COLL);
    vtxp->u.imcast.last_complete = -1;

    MPIR_Comm_add_ref(comm_ptr);
    MPIR_Datatype_add_ref_if_not_builtin(dt);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] imcast", *vtx_id));
    return mpi_errno;
}

int MPIR_TSP_sched_issend(const void *buf,
                          MPI_Aint count,
                          MPI_Datatype dt,
                          int dest,
                          int tag,
                          MPIR_Comm * comm_ptr, MPIR_TSP_sched_t s, int n_in_vtcs, int *in_vtcs,
                          int *vtx_id)
{
    MPII_Genutil_sched_t *sched = s;
    vtx_t *vtxp;
    int mpi_errno = MPI_SUCCESS;
    /* assign a new vertex */
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);
    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__ISSEND;
    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);

    /* store the arguments */
    vtxp->u.issend.buf = buf;
    vtxp->u.issend.count = count;
    vtxp->u.issend.dt = dt;
    vtxp->u.issend.dest = dest;
    vtxp->u.issend.tag = tag;
    vtxp->u.issend.comm = comm_ptr;

    MPIR_Comm_add_ref(comm_ptr);
    MPIR_Datatype_add_ref_if_not_builtin(dt);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] issend", *vtx_id));

    return mpi_errno;
}

int MPIR_TSP_sched_reduce_local(const void *inbuf, void *inoutbuf, MPI_Aint count,
                                MPI_Datatype datatype, MPI_Op op, MPIR_TSP_sched_t s,
                                int n_in_vtcs, int *in_vtcs, int *vtx_id)
{
    MPII_Genutil_sched_t *sched = s;
    vtx_t *vtxp;
    int mpi_errno = MPI_SUCCESS;
    /* assign a new vertex */
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__REDUCE_LOCAL;
    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);

    /* record the arguments */
    vtxp->u.reduce_local.inbuf = inbuf;
    vtxp->u.reduce_local.inoutbuf = inoutbuf;
    vtxp->u.reduce_local.count = count;
    vtxp->u.reduce_local.datatype = datatype;
    vtxp->u.reduce_local.op = op;

    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIR_Op_add_ref_if_not_builtin(op);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] reduce_local", *vtx_id));

    return mpi_errno;
}


int MPIR_TSP_sched_localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                             MPIR_TSP_sched_t s, int n_in_vtcs, int *in_vtcs, int *vtx_id)
{
    MPII_Genutil_sched_t *sched = s;
    vtx_t *vtxp;
    int mpi_errno = MPI_SUCCESS;
    /* assign a new vertex */
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__LOCALCOPY;
    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);

    /* record the arguments */
    vtxp->u.localcopy.sendbuf = sendbuf;
    vtxp->u.localcopy.sendcount = sendcount;
    vtxp->u.localcopy.sendtype = sendtype;
    vtxp->u.localcopy.recvbuf = recvbuf;
    vtxp->u.localcopy.recvcount = recvcount;
    vtxp->u.localcopy.recvtype = recvtype;

    MPIR_Datatype_add_ref_if_not_builtin(sendtype);
    MPIR_Datatype_add_ref_if_not_builtin(recvtype);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] localcopy", *vtx_id));

    return mpi_errno;
}


/* Transport function that adds a callback type vertex in the graph */
int MPIR_TSP_sched_cb(MPIR_TSP_cb_t cb_p, void *cb_data, MPIR_TSP_sched_t sched,
                      int n_in_vtcs, int *in_vtcs, int *vtx_id)
{
    vtx_t *vtxp;
    int mpi_errno = MPI_SUCCESS;

    /* assign a new vertex */
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__CB;
    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);

    /* record the arguments */
    vtxp->u.cb.cb_p = cb_p;
    vtxp->u.cb.cb_data = cb_data;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Gentran: schedule [%d] cb", *vtx_id));

    return mpi_errno;
}

/* Transport function that adds a no op vertex in the graph that has
 * all the vertices posted before it as incoming vertices */
int MPIR_TSP_sched_sink(MPIR_TSP_sched_t s, int *vtx_id)
{
    MPII_Genutil_sched_t *sched = s;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: sched [sink] total=%d", sched->total_vtcs));

    vtx_t *vtxp;
    vtx_t *sched_fence;
    int i, n_in_vtcs = 0;
    int *in_vtcs;
    int mpi_errno = MPI_SUCCESS;

    MPIR_CHKLMEM_DECL(1);

    /* assign a new vertex */
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__SINK;

    MPIR_CHKLMEM_MALLOC(in_vtcs, int *, sizeof(int) * (*vtx_id),
                        mpi_errno, "in_vtcs buffer", MPL_MEM_COLL);
    /* record incoming vertices */
    sched_fence = ut_type_array(&sched->vtcs, vtx_t *) + *vtx_id - 1;
    MPIR_ERR_CHKANDJUMP(sched_fence == NULL, mpi_errno, MPI_ERR_OTHER, "**nofence");
    for (i = *vtx_id - 1; i >= 0; i--) {
        if (sched_fence->vtx_kind == MPII_GENUTIL_VTX_KIND__FENCE)
            /* no need to record this and any vertex before fence.
             * Dependency on the last fence call will be added by
             * the subsequent call to MPIC_Genutil_vtx_add_dependencies function */
            break;
        else {
            /* this vertex only depends on the sink when it has no
             * outgoing dependence     */
            if (utarray_len(&sched_fence->out_vtcs) == 0)
                in_vtcs[n_in_vtcs++] = i;
        }
        sched_fence--;
    }

    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);
    MPIR_CHKLMEM_FREEALL();
  fn_exit:
    return mpi_errno;
  fn_fail:
    /* TODO - Replace this with real error handling */
    MPIR_Assert(MPI_SUCCESS == mpi_errno);
    goto fn_exit;
}

int MPIR_TSP_sched_fence(MPIR_TSP_sched_t s)
{
    MPII_Genutil_sched_t *sched = s;
    int mpi_errno ATTRIBUTE((unused)) = MPI_SUCCESS;
    int fence_id;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Gentran: scheduling a fence"));

    /* fence operation is an extension to fence, so we can reuse the fence call */
    mpi_errno = MPIR_TSP_sched_sink(sched, &fence_id);
    MPIR_ERR_CHECK(mpi_errno);
    /* change the vertex kind from SINK to FENCE */
    vtx_t *sched_fence = (vtx_t *) utarray_eltptr(&sched->vtcs, fence_id);
    MPIR_ERR_CHKANDJUMP(sched_fence == NULL, mpi_errno, MPI_ERR_OTHER, "**nofence");
    sched_fence->vtx_kind = MPII_GENUTIL_VTX_KIND__FENCE;
    sched->last_fence = fence_id;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_TSP_sched_selective_sink(MPIR_TSP_sched_t s, int n_in_vtcs, int *in_vtcs, int *vtx_id)
{
    MPII_Genutil_sched_t *sched = s;
    vtx_t *vtxp;
    int mpi_errno = MPI_SUCCESS;
    /* assign a new vertex */
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__SELECTIVE_SINK;
    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] selective_sink task", *vtx_id));

    return mpi_errno;
}

void *MPIR_TSP_sched_malloc(size_t size, MPIR_TSP_sched_t s)
{
    MPII_Genutil_sched_t *sched = s;
    void *addr = MPL_malloc(size, MPL_MEM_COLL);
    utarray_push_back(&sched->buffers, &addr, MPL_MEM_COLL);
    return addr;
}

int MPIR_TSP_sched_sub_sched(MPIR_TSP_sched_t s, MPIR_TSP_sched_t subs,
                             int n_in_vtcs, int *in_vtcs, int *vtx_id)
{
    MPII_Genutil_sched_t *sched = s;
    MPII_Genutil_sched_t *subsched = subs;
    int mpi_errno = MPI_SUCCESS;
    vtx_t *vtxp;

    /* assign a new vertex */
    *vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__SCHED;
    MPII_Genutil_vtx_add_dependencies(sched, *vtx_id, n_in_vtcs, in_vtcs);
    vtxp->u.sched.sched = subsched;
    vtxp->u.sched.req = NULL;
    vtxp->u.sched.is_persistent = subsched->is_persistent;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] sub-schedule", *vtx_id));

    return mpi_errno;
}

int MPIR_TSP_sched_start(MPIR_TSP_sched_t s, MPIR_Comm * comm, MPIR_Request ** req)
{
    MPII_Genutil_sched_t *sched = s;
    int mpi_errno = MPI_SUCCESS;
    int is_complete;
    int made_progress;
    MPIR_Request *reqp;

    MPIR_FUNC_ENTER;

    if (unlikely(sched->total_vtcs == 0)) {
        if (!sched->is_persistent) {
            mpi_errno = MPIR_TSP_sched_free(sched);
            MPIR_ERR_CHECK(mpi_errno);
        }
        *req = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
        MPIR_ERR_CHKANDSTMT(*req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        goto fn_exit;
    }

    /* Create a request */
    reqp = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!reqp)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    *req = reqp;
    MPIR_Request_add_ref(reqp);
    sched->req = reqp;

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
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_TSP_sched_reset(MPIR_TSP_sched_t s)
{
    MPII_Genutil_sched_t *sched = s;
    int mpi_errno = MPI_SUCCESS;
    vtx_t *vtx;
    int i;

    MPIR_FUNC_ENTER;

    MPIR_Assert(sched->is_persistent);

    sched->completed_vtcs = 0;
    sched->issued_head = sched->issued_tail = NULL;

    vtx = ut_type_array(&sched->vtcs, vtx_t *);
    for (i = 0; i < sched->total_vtcs; i++, vtx++) {
        MPIR_ERR_CHKANDJUMP(!vtx, mpi_errno, MPI_ERR_OTHER, "**nomem");
        vtx->pending_dependencies = vtx->num_dependencies;
        vtx->vtx_state = MPII_GENUTIL_VTX_STATE__INIT;
        switch (vtx->vtx_kind) {
            case MPII_GENUTIL_VTX_KIND__IMCAST:
                vtx->u.imcast.last_complete = -1;
                break;
            case MPII_GENUTIL_VTX_KIND__SCHED:
                MPIR_TSP_sched_reset(vtx->u.sched.sched);
                break;
            default:
                break;
        }
    }
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
