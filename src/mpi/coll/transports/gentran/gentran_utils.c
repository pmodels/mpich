/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"
#include "gentran_impl.h"

/* LL_DELETE for use inside LL_FOREACH_SAFE: unlinks del in O(1), given its predecessor prev
 * (NULL if del is the head) */
#define MY_LL_DELETE_INLINE(head, tail, prev, del) \
    do { \
        if (prev) { \
            (prev)->next = (del)->next; \
        } else { \
            (head) = (del)->next; \
        } \
        if ((tail) == (del)) { \
            (tail) = (prev); \
        } \
    } while (0)


/* Issues vtxp, which has no pending dependency; *immediate_complete tells whether it completed
 * at once. The caller releases its dependents. */
static int vtx_issue(int vtxid, MPII_Genutil_vtx_t * vtxp, MPII_Genutil_sched_t * sched,
                     bool * immediate_complete)
{
    int mpi_errno ATTRIBUTE((unused)) = MPI_SUCCESS;
    MPIR_Request *r = sched->req;
    bool done = false;          /* complete already */

    MPIR_FUNC_ENTER;

    MPIR_Assert(vtxp->vtx_state == MPII_GENUTIL_VTX_STATE__INIT && vtxp->pending_dependencies == 0);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Issuing vertex %d", vtxid));

    switch (vtxp->vtx_kind) {
        case MPII_GENUTIL_VTX_KIND__ISEND:{
                if (vtxp->u.isend.sync) {
                    MPIC_Issend(vtxp->u.isend.buf, vtxp->u.isend.count, vtxp->u.isend.dt,
                                vtxp->u.isend.dest, sched->tag, vtxp->u.isend.comm,
                                &vtxp->u.isend.req, r->u.nbc.coll_attr);
                } else {
                    MPIC_Isend(vtxp->u.isend.buf, vtxp->u.isend.count, vtxp->u.isend.dt,
                               vtxp->u.isend.dest, sched->tag, vtxp->u.isend.comm,
                               &vtxp->u.isend.req, r->u.nbc.coll_attr);
                }

                if (MPIR_Request_is_complete(vtxp->u.isend.req)) {
                    MPIR_Request_free(vtxp->u.isend.req);
                    vtxp->u.isend.req = NULL;
#ifdef MPL_USE_DBG_LOGGING
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (vtx_kind=%d) complete",
                                     vtxp->vtx_kind));
                    if (vtxp->u.isend.count >= 1)
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                        (MPL_DBG_FDEST, "data sent: %d",
                                         *(int *) (vtxp->u.isend.buf)));
#endif
                    done = true;
                } else {
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (isend) issued, tag = %d",
                                     sched->tag));
                }
            }
            break;

        case MPII_GENUTIL_VTX_KIND__IRECV:{
                MPIC_Irecv(vtxp->u.irecv.buf,
                           vtxp->u.irecv.count,
                           vtxp->u.irecv.dt,
                           vtxp->u.irecv.src, sched->tag, vtxp->u.irecv.comm, &vtxp->u.irecv.req);

                if (MPIR_Request_is_complete(vtxp->u.irecv.req)) {
                    MPIR_Request_free(vtxp->u.irecv.req);
                    vtxp->u.irecv.req = NULL;
#ifdef MPL_USE_DBG_LOGGING
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (vtx_kind=%d) complete",
                                     vtxp->vtx_kind));
                    if (vtxp->u.irecv.count >= 1)
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                        (MPL_DBG_FDEST, "data recvd: %d",
                                         *(int *) (vtxp->u.irecv.buf)));
#endif
                    done = true;
                } else {
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "  --> GENTRAN transport (irecv) issued"));
                }
            }
            break;

        case MPII_GENUTIL_VTX_KIND__IRECV_STATUS:{
                MPIC_Irecv(vtxp->u.irecv_status.buf,
                           vtxp->u.irecv_status.count,
                           vtxp->u.irecv_status.dt,
                           vtxp->u.irecv_status.src, sched->tag,
                           vtxp->u.irecv_status.comm, &vtxp->u.irecv_status.req);

                if (MPIR_Request_is_complete(vtxp->u.irecv_status.req)) {
                    if (vtxp->u.irecv_status.status != MPI_STATUS_IGNORE) {
                        MPI_Aint recvd;
                        vtxp->u.irecv_status.status->MPI_ERROR =
                            vtxp->u.irecv_status.req->status.MPI_ERROR;
                        MPIR_Get_count_impl(&vtxp->u.irecv_status.req->status,
                                            MPIR_BYTE_INTERNAL, &recvd);
                        MPIR_STATUS_SET_COUNT(*(vtxp->u.irecv_status.status), recvd);
                    }
                    MPIR_Request_free(vtxp->u.irecv_status.req);
                    vtxp->u.irecv_status.req = NULL;
#ifdef MPL_USE_DBG_LOGGING
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (vtx_kind=%d) complete",
                                     vtxp->vtx_kind));
                    if (vtxp->u.irecv_status.count >= 1)
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                        (MPL_DBG_FDEST, "data recvd: %d",
                                         *(int *) (vtxp->u.irecv_status.buf)));
#endif
                    done = true;
                } else {
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "  --> GENTRAN transport (irecv) issued"));
                }
            }
            break;

        case MPII_GENUTIL_VTX_KIND__REDUCE_LOCAL:{
                MPIR_Reduce_local(vtxp->u.reduce_local.inbuf,
                                  vtxp->u.reduce_local.inoutbuf,
                                  vtxp->u.reduce_local.count,
                                  vtxp->u.reduce_local.datatype, vtxp->u.reduce_local.op);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "  --> GENTRAN transport (reduce_local) performed"));

                done = true;
            }
            break;

        case MPII_GENUTIL_VTX_KIND__LOCALCOPY:{
                MPIR_Localcopy(vtxp->u.localcopy.sendbuf,
                               vtxp->u.localcopy.sendcount,
                               vtxp->u.localcopy.sendtype,
                               vtxp->u.localcopy.recvbuf,
                               vtxp->u.localcopy.recvcount, vtxp->u.localcopy.recvtype);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "  --> GENTRAN transport (localcopy) performed"));

                done = true;
            }
            break;
        case MPII_GENUTIL_VTX_KIND__SELECTIVE_SINK:{
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "  --> GENTRAN transport (selective sink) performed"));
                /* Nothing to do, just record completion */
                done = true;
            }
            break;
        case MPII_GENUTIL_VTX_KIND__SINK:{
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "  --> GENTRAN transport (sink) performed"));
                /* Nothing to do, just record completion */
                done = true;
            }
            break;
        case MPII_GENUTIL_VTX_KIND__FENCE:{
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "  --> GENTRAN transport (fence) performed"));
                /* Nothing to do, just record completion */
                done = true;
            }
            break;
        case MPII_GENUTIL_VTX_KIND__SCHED:{
                mpi_errno = MPIR_TSP_sched_start(vtxp->u.sched.sched, NULL, &vtxp->u.sched.req);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "  --> GENTRAN transport (subsched) issued"));
            }
            break;
        case MPII_GENUTIL_VTX_KIND__CB:{
                /* ignore communicator and tag */
                int ret_errno = vtxp->u.cb.cb_p(NULL, -1, vtxp->u.cb.cb_data);
                if (unlikely(ret_errno)) {
                    if (0 == r->u.nbc.coll_attr) {
                        if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(ret_errno)) {
                            r->u.nbc.coll_attr = MPIR_ERR_PROC_FAILED;
                        } else {
                            r->u.nbc.coll_attr = MPIR_ERR_OTHER;
                        }
                    }
                }
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "  --> GENTRAN transport (cb) performed"));
                done = true;
            }
            break;
        default:{
                int is_done = 0;
                MPII_Genutil_vtx_type_t *vtype = ut_type_array(&sched->generic_types,
                                                               MPII_Genutil_vtx_type_t *) +
                    vtxp->vtx_kind - MPII_GENUTIL_VTX_KIND__LAST - 1;
                MPIR_Assert(vtype != NULL);
                mpi_errno = vtype->issue_fn(vtxp->u.generic.data, &is_done);
                MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**other");
                done = is_done;
                break;
            }
    }

    if (done) {
        vtxp->vtx_state = MPII_GENUTIL_VTX_STATE__COMPLETE;
        sched->completed_vtcs++;
    } else {
        vtxp->vtx_state = MPII_GENUTIL_VTX_STATE__ISSUED;
        LL_APPEND(sched->issued_head, sched->issued_tail, vtxp);
    }
    *immediate_complete = done;

#ifdef MPL_USE_DBG_LOGGING
    /* print issued vertex list */
    {
        vtx_t *vtxp_tmp = vtxp;
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Issued vertices list: "));
        LL_FOREACH(sched->issued_head, vtxp_tmp) {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "%d", vtxp_tmp->vtx_id));
        }
    }
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static void vtx_release_in_start(MPII_Genutil_vtx_list_t * deps, MPII_Genutil_sched_t * sched);
static void vtx_release_in_poke(MPII_Genutil_vtx_list_t * deps, MPII_Genutil_sched_t * sched);
static void push_ready(MPII_Genutil_sched_t * sched, int vtx_id);
static int drain_ready(MPII_Genutil_sched_t * sched);

static void vtx_release_dependents_impl(MPII_Genutil_vtx_list_t * deps,
                                        MPII_Genutil_sched_t * sched, bool queue)
{
    int *ids = MPII_Genutil_vtx_list_data(deps);
    for (int i = 0; i < deps->n; i++) {
        int outvtx_id = ids[i];
        vtx_t *vtx = ut_type_array(&sched->vtcs, vtx_t *) + outvtx_id;

        if (--vtx->pending_dependencies == 0 && queue) {
            push_ready(sched, outvtx_id);
        }
    }
}

/* Counts down the dependents in deps (a vertex's issue_out_vtcs or completion_out_vtcs). Used by
 * MPII_Genutil_sched_start, whose scan reaches the released ones itself. */
static void vtx_release_in_start(MPII_Genutil_vtx_list_t * deps, MPII_Genutil_sched_t * sched)
{
    vtx_release_dependents_impl(deps, sched, false);
}

/* Counts down the dependents in deps, and queues the released ones in ready_vtcs_during_poke,
 * which the poke drains in order, instead of issuing them here.
 *
 * The queue issues sends to multiple targets round robin. Issuing the released vertices here,
 * recursively, would go depth first: at the root of a pipelined tree, the send of a chunk to the
 * first child releases the next chunk to that child, and so on, so a message to a later child
 * waits until every ready chunk to the earlier children has been issued, which adds to its
 * latency. */
static void vtx_release_in_poke(MPII_Genutil_vtx_list_t * deps, MPII_Genutil_sched_t * sched)
{
    vtx_release_dependents_impl(deps, sched, true);
}

static void push_ready(MPII_Genutil_sched_t * sched, int vtx_id)
{
    utarray_push_back_int(&sched->ready_vtcs_during_poke, &vtx_id, MPL_MEM_COLL);
}

/* Issues the ready vertices in the order they became ready */
static int drain_ready(MPII_Genutil_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    UT_array *ready = &sched->ready_vtcs_during_poke;

    /* vtx_issue may append more */
    for (int head = 0; head < utarray_len(ready); head++) {
        int id = ut_int_array(ready)[head];
        vtx_t *vtx = ut_type_array(&sched->vtcs, vtx_t *) + id;
        bool done;

        mpi_errno = vtx_issue(id, vtx, sched, &done);
        MPIR_ERR_CHECK(mpi_errno);
        if (done) {
            vtx_release_in_poke(&vtx->completion_out_vtcs, sched);
        }
        vtx_release_in_poke(&vtx->issue_out_vtcs, sched);
    }

  fn_exit:
    utarray_clear(ready);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPII_Genutil_progress_hook(int vci, int *made_progress)
{
    int count = 0;
    int mpi_errno = MPI_SUCCESS;
    MPII_Coll_req_t *coll_req, *coll_req_tmp;

    /* gentran progress may issue operations (e.g. MPIC_Isend) that will call into progress
     * again (ref: MPIDI_OFI_retry_progress). Similar to MPIDU_Sched_progress, we should skip
     * to avoid recursive entering.
     */
    static int in_genutil_progress = 0;

    if (in_genutil_progress) {
        return MPI_SUCCESS;
    }

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_TSP_QUEUE_MUTEX);
    in_genutil_progress = 1;

    if (made_progress)
        *made_progress = false;

    /* Go over up to MPIR_COLL_PROGRESS_MAX_COLLS collecives in the
     * queue and make progress on them */
    DL_FOREACH_SAFE(MPII_coll_queue.head, coll_req, coll_req_tmp) {
        /* make progress on the collective operation */
        int done, progress = false;
        MPII_Genutil_sched_t *sched = (MPII_Genutil_sched_t *) (coll_req->sched);

        /* make progress on the collective */
        mpi_errno = MPII_Genutil_sched_poke(sched, &done, &progress);

        if (done) {
            MPIR_Request *req;

            coll_req->sched = NULL;

            req = MPL_container_of(coll_req, MPIR_Request, u.nbc.coll);
            DL_DELETE(MPII_coll_queue.head, coll_req);
            MPIR_Request_complete(req);
        }
        if (progress) {
            count++;
        }
        if (MPIR_CVAR_PROGRESS_MAX_COLLS > 0 && count >= MPIR_CVAR_PROGRESS_MAX_COLLS)
            break;
    }

    if (made_progress && count)
        *made_progress = true;

    if (MPII_coll_queue.head == NULL)
        MPIR_Progress_hook_deactivate(MPII_Genutil_progress_hook_id);

    in_genutil_progress = 0;
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_TSP_QUEUE_MUTEX);

    return mpi_errno;
}


void MPII_Genutil_vtx_copy(void *_dst, const void *_src)
{
    vtx_t *dst = (vtx_t *) _dst;
    vtx_t *src = (vtx_t *) _src;

    dst->vtx_kind = src->vtx_kind;
    dst->vtx_state = src->vtx_state;
    dst->vtx_id = src->vtx_id;

    MPII_Genutil_vtx_list_init(&dst->completion_out_vtcs);
    for (int i = 0; i < src->completion_out_vtcs.n; i++) {
        MPII_Genutil_vtx_list_push(&dst->completion_out_vtcs,
                                   MPII_Genutil_vtx_list_data(&src->completion_out_vtcs)[i]);
    }
    MPII_Genutil_vtx_list_init(&dst->issue_out_vtcs);
    for (int i = 0; i < src->issue_out_vtcs.n; i++) {
        MPII_Genutil_vtx_list_push(&dst->issue_out_vtcs,
                                   MPII_Genutil_vtx_list_data(&src->issue_out_vtcs)[i]);
    }

    dst->pending_dependencies = src->pending_dependencies;
    dst->num_dependencies = src->num_dependencies;
    dst->u = src->u;
    dst->next = src->next;
}


void MPII_Genutil_vtx_dtor(void *_elt)
{
    vtx_t *elt = (vtx_t *) _elt;

    MPII_Genutil_vtx_list_free(&elt->completion_out_vtcs);
    MPII_Genutil_vtx_list_free(&elt->issue_out_vtcs);
}


void MPII_Genutil_vtx_add_dependencies(MPII_Genutil_sched_t * sched, int vtx_id,
                                       int n_in_vtcs, int *in_vtcs)
{
    int i;
    MPII_Genutil_vtx_t *vtx;

    vtx = ut_type_array(&sched->vtcs, vtx_t *) + vtx_id;
    MPIR_Assert(vtx != NULL);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "Updating in_vtcs of vtx %d, vtx_kind %d, n_in_vtcs %d", vtx_id,
                     vtx->vtx_kind, n_in_vtcs));

    /* update the list of outgoing vertices of the incoming
     * vertices */
    for (i = 0; i < n_in_vtcs; i++) {
        vtx_t *in_vtx = (vtx_t *) utarray_eltptr(&sched->vtcs, in_vtcs[i]);
        MPIR_Assert(in_vtx != NULL);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "invtx: %d", in_vtcs[i]));
        MPII_Genutil_vtx_list_push(&in_vtx->completion_out_vtcs, vtx_id);

        /* increment pending_dependencies only if the incoming
         * vertex is not complete yet */
        if (in_vtx->vtx_state != MPII_GENUTIL_VTX_STATE__COMPLETE) {
            vtx->num_dependencies++;
            vtx->pending_dependencies++;
        }
    }

    /* check if there was any fence operation and add appropriate dependencies.
     * The application will never explicitly specify a dependency on it,
     * the transport has to make sure that the dependency on the fence operation is met */
    if (sched->last_fence != -1 && sched->last_fence != vtx_id && n_in_vtcs == 0) {
        /* add vtx as outgoing vtx of last_fence */
        vtx_t *sched_fence = (vtx_t *) utarray_eltptr(&sched->vtcs, sched->last_fence);
        MPIR_Assert(sched_fence != NULL);
        MPII_Genutil_vtx_list_push(&sched_fence->completion_out_vtcs, vtx_id);

        /* increment pending_dependencies only if the incoming
         * vertex is not complete yet */
        if (sched_fence->vtx_state != MPII_GENUTIL_VTX_STATE__COMPLETE) {
            vtx->pending_dependencies++;
            vtx->num_dependencies++;
        }
    }
}


int MPII_Genutil_vtx_create(MPII_Genutil_sched_t * sched, MPII_Genutil_vtx_t ** vtx)
{
    MPII_Genutil_vtx_t *vtxp;

    utarray_extend_back(&sched->vtcs, MPL_MEM_COLL);
    *vtx = (vtx_t *) utarray_back(&sched->vtcs);
    vtxp = *vtx;

    MPII_Genutil_vtx_list_init(&vtxp->completion_out_vtcs);
    MPII_Genutil_vtx_list_init(&vtxp->issue_out_vtcs);

    vtxp->vtx_state = MPII_GENUTIL_VTX_STATE__INIT;
    vtxp->vtx_id = sched->total_vtcs++;
    vtxp->pending_dependencies = 0;
    vtxp->num_dependencies = 0;
    vtxp->next = NULL;

    return vtxp->vtx_id;        /* return vertex vtx_id */
}


/* Issues the vertices with no dependency, when the schedule starts. A vertex they release is
 * issued when the scan reaches it, since dependencies point to older vertices. */
int MPII_Genutil_sched_start(MPII_Genutil_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;

    vtx_t *vtx = ut_type_array(&sched->vtcs, vtx_t *);
    for (int i = 0; i < sched->total_vtcs; i++, vtx++) {
        if (vtx->pending_dependencies == 0) {
            bool done;

            mpi_errno = vtx_issue(i, vtx, sched, &done);
            MPIR_ERR_CHECK(mpi_errno);
            if (done) {
                vtx_release_in_start(&vtx->completion_out_vtcs, sched);
            }
            vtx_release_in_start(&vtx->issue_out_vtcs, sched);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_Genutil_sched_poke(MPII_Genutil_sched_t * sched, int *is_complete, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    vtx_t *vtxp, *vtxp_tmp;

    MPIR_FUNC_ENTER;

    MPIR_Assert(sched->total_vtcs > 0);

    if (made_progress)
        *made_progress = FALSE;

    *is_complete = FALSE;

    vtx_t *prev = NULL;
    LL_FOREACH_SAFE(sched->issued_head, vtxp, vtxp_tmp) {
        bool done = false;
        MPIR_Assert(vtxp->vtx_state == MPII_GENUTIL_VTX_STATE__ISSUED);

        switch (vtxp->vtx_kind) {
            case MPII_GENUTIL_VTX_KIND__ISEND:
                if (MPIR_Request_is_complete(vtxp->u.isend.req)) {
                    MPIR_Request_free(vtxp->u.isend.req);
                    vtxp->u.isend.req = NULL;
#ifdef MPL_USE_DBG_LOGGING
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (vtx_kind=%d) complete",
                                     vtxp->vtx_kind));
                    if (vtxp->u.isend.count >= 1)
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                        (MPL_DBG_FDEST, "data sent: %d",
                                         *(int *) (vtxp->u.isend.buf)));
#endif
                    done = true;
                }
                break;

            case MPII_GENUTIL_VTX_KIND__IRECV:
                if (MPIR_Request_is_complete(vtxp->u.irecv.req)) {
                    MPIR_Request_free(vtxp->u.irecv.req);
                    vtxp->u.irecv.req = NULL;
#ifdef MPL_USE_DBG_LOGGING
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (vtx_kind=%d) complete",
                                     vtxp->vtx_kind));
                    if (vtxp->u.irecv.count >= 1)
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                        (MPL_DBG_FDEST, "data recvd: %d",
                                         *(int *) (vtxp->u.irecv.buf)));
#endif
                    done = true;
                }
                break;

            case MPII_GENUTIL_VTX_KIND__IRECV_STATUS:
                if (MPIR_Request_is_complete(vtxp->u.irecv_status.req)) {
                    if (vtxp->u.irecv_status.status != MPI_STATUS_IGNORE) {
                        MPI_Aint recvd;
                        vtxp->u.irecv_status.status->MPI_ERROR =
                            vtxp->u.irecv_status.req->status.MPI_ERROR;
                        MPIR_Get_count_impl(&vtxp->u.irecv_status.req->status, MPIR_BYTE_INTERNAL,
                                            &recvd);
                        MPIR_STATUS_SET_COUNT(*(vtxp->u.irecv_status.status), recvd);
                    }
                    MPIR_Request_free(vtxp->u.irecv_status.req);
                    vtxp->u.irecv_status.req = NULL;
#ifdef MPL_USE_DBG_LOGGING
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (vtx_kind=%d) complete",
                                     vtxp->vtx_kind));
                    if (vtxp->u.irecv_status.count >= 1)
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                        (MPL_DBG_FDEST, "data recvd: %d",
                                         *(int *) (vtxp->u.irecv_status.buf)));
#endif
                    done = true;
                }
                break;

            case MPII_GENUTIL_VTX_KIND__SCHED:
                if (MPIR_Request_is_complete(vtxp->u.sched.req)) {
                    MPIR_Request_free(vtxp->u.sched.req);
                    vtxp->u.sched.req = NULL;
#ifdef MPL_USE_DBG_LOGGING
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (vtx_kind=%d) complete",
                                     vtxp->vtx_kind));
#endif
                    done = true;
                }
                break;

            default:
                if (vtxp->vtx_kind > MPII_GENUTIL_VTX_KIND__LAST) {
                    int is_completed;
                    MPII_Genutil_vtx_type_t *vtype = ut_type_array(&sched->generic_types,
                                                                   MPII_Genutil_vtx_type_t *) +
                        vtxp->vtx_kind - MPII_GENUTIL_VTX_KIND__LAST - 1;
                    MPIR_Assert(vtype != NULL);
                    mpi_errno = vtype->complete_fn(vtxp->u.generic.data, &is_completed);
                    MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno,
                                        MPI_ERR_OTHER, "**other");
                    done = is_completed;
                }
                break;
        }

        if (!done) {
            prev = vtxp;
        } else {
            MY_LL_DELETE_INLINE(sched->issued_head, sched->issued_tail, prev, vtxp);

            vtxp->vtx_state = MPII_GENUTIL_VTX_STATE__COMPLETE;
            sched->completed_vtcs++;
            vtx_release_in_poke(&vtxp->completion_out_vtcs, sched);
            if (made_progress)
                *made_progress = TRUE;
        }
    }

#ifdef MPL_USE_DBG_LOGGING
    if (sched->completed_vtcs == sched->total_vtcs) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "  --> GENTRAN transport (test) complete:  sched->total_vtcs=%d",
                         sched->total_vtcs));
    }
#endif

    mpi_errno = drain_ready(sched);
    MPIR_ERR_CHECK(mpi_errno);

    *is_complete = (sched->completed_vtcs == sched->total_vtcs);
    if (*is_complete) {
        if (made_progress)
            *made_progress = TRUE;

        /* error handling */
        switch (sched->req->u.nbc.coll_attr) {
            case MPIR_ERR_PROC_FAILED:
                MPIR_ERR_SET(sched->req->status.MPI_ERROR, MPIX_ERR_PROC_FAILED, "**comm");
                break;
            case MPIR_ERR_OTHER:
                MPIR_ERR_SET(sched->req->status.MPI_ERROR, MPI_ERR_OTHER, "**comm");
                break;
            case 0:
            default:
                break;
        }

        if (sched->is_persistent == false) {
            mpi_errno = MPIR_TSP_sched_free(sched);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
