/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "tsp_gentran_types.h"
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "utlist.h"

static void vtx_record_completion(MPII_Genutil_vtx_t * vtxp, MPII_Genutil_sched_t * sched);

static void vtx_extend_utarray(UT_array * dst_array, int n_elems, int *elems)
{
    int i;

    for (i = 0; i < n_elems; i++) {
        utarray_push_back(dst_array, &elems[i], MPL_MEM_COLL);
    }
}

static void vtx_record_issue(MPII_Genutil_vtx_t * vtxp, MPII_Genutil_sched_t * sched)
{
    vtxp->vtx_state = MPII_GENUTIL_VTX_STATE__ISSUED;
    LL_APPEND(sched->issued_head, sched->issued_tail, vtxp);
}

static void vtx_issue(int vtxid, MPII_Genutil_vtx_t * vtxp, MPII_Genutil_sched_t * sched)
{
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_VTX_ISSUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_VTX_ISSUE);

    /* Check if the vertex has not already been issued and its
     * incoming dependencies have completed */
    if (vtxp->vtx_state == MPII_GENUTIL_VTX_STATE__INIT && vtxp->pending_dependencies == 0) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Issuing vertex %d", vtxid));

        switch (vtxp->vtx_kind) {
            case MPII_GENUTIL_VTX_KIND__ISEND:{
                    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

                    MPIC_Isend(vtxp->u.isend.buf,
                               vtxp->u.isend.count,
                               vtxp->u.isend.dt,
                               vtxp->u.isend.dest,
                               vtxp->u.isend.tag, vtxp->u.isend.comm, &vtxp->u.isend.req, &errflag);

                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (isend) issued, tag = %d",
                                     vtxp->u.isend.tag));
                    vtx_record_issue(vtxp, sched);
                }
                break;

            case MPII_GENUTIL_VTX_KIND__IRECV:{
                    MPIC_Irecv(vtxp->u.irecv.buf,
                               vtxp->u.irecv.count,
                               vtxp->u.irecv.dt,
                               vtxp->u.irecv.src, vtxp->u.irecv.tag, vtxp->u.irecv.comm,
                               &vtxp->u.irecv.req);

                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "  --> GENTRAN transport (irecv) issued"));
                    vtx_record_issue(vtxp, sched);
                }
                break;

            case MPII_GENUTIL_VTX_KIND__IMCAST:{
                    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

                    for (i = 0; i < vtxp->u.imcast.num_dests; i++)
                        MPIC_Isend(vtxp->u.imcast.buf,
                                   vtxp->u.imcast.count,
                                   vtxp->u.imcast.dt,
                                   *(int *) utarray_eltptr(vtxp->u.imcast.dests, i),
                                   vtxp->u.imcast.tag, vtxp->u.imcast.comm, &vtxp->u.imcast.req[i],
                                   &errflag);

                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (imcast) issued, tag = %d",
                                     vtxp->u.imcast.tag));
                    vtx_record_issue(vtxp, sched);
                }
                break;

            case MPII_GENUTIL_VTX_KIND__ISSEND:{
                    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

                    MPIC_Issend(vtxp->u.issend.buf,
                                vtxp->u.issend.count,
                                vtxp->u.issend.dt,
                                vtxp->u.issend.dest,
                                vtxp->u.issend.tag, vtxp->u.issend.comm, &vtxp->u.issend.req,
                                &errflag);

                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (issend) issued, tag = %d",
                                     vtxp->u.issend.tag));
                    vtx_record_issue(vtxp, sched);
                }
                break;


            case MPII_GENUTIL_VTX_KIND__REDUCE_LOCAL:{
                    vtx_record_issue(vtxp, sched);
                    MPIR_Reduce_local(vtxp->u.reduce_local.inbuf,
                                      vtxp->u.reduce_local.inoutbuf,
                                      vtxp->u.reduce_local.count,
                                      vtxp->u.reduce_local.datatype, vtxp->u.reduce_local.op);
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (reduce_local) performed"));

                    vtx_record_completion(vtxp, sched);
                }
                break;

            case MPII_GENUTIL_VTX_KIND__LOCALCOPY:{
                    vtx_record_issue(vtxp, sched);
                    MPIR_Localcopy(vtxp->u.localcopy.sendbuf,
                                   vtxp->u.localcopy.sendcount,
                                   vtxp->u.localcopy.sendtype,
                                   vtxp->u.localcopy.recvbuf,
                                   vtxp->u.localcopy.recvcount, vtxp->u.localcopy.recvtype);
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (localcopy) performed"));

                    vtx_record_completion(vtxp, sched);
                }
                break;
            case MPII_GENUTIL_VTX_KIND__SELECTIVE_SINK:{
                    vtx_record_issue(vtxp, sched);
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (selective sink) performed"));
                    /* Nothin to do, just record completion */
                    vtx_record_completion(vtxp, sched);
                }
                break;
            case MPII_GENUTIL_VTX_KIND__SINK:{
                    vtx_record_issue(vtxp, sched);
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "  --> GENTRAN transport (sink) performed"));
                    /* Nothin to do, just record completion */
                    vtx_record_completion(vtxp, sched);
                }
                break;
            case MPII_GENUTIL_VTX_KIND__FENCE:{
                    vtx_record_issue(vtxp, sched);
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "  --> GENTRAN transport (fence) performed"));
                    /* Nothin to do, just record completion */
                    vtx_record_completion(vtxp, sched);
                }
                break;
            default:{
                    int mpi_errno, done;
                    MPII_Genutil_vtx_type_t *vtype = ut_type_array(&sched->generic_types,
                                                                   MPII_Genutil_vtx_type_t *) +
                        vtxp->vtx_kind - MPII_GENUTIL_VTX_KIND__LAST - 1;
                    MPIR_Assert(vtype != NULL);
                    mpi_errno = vtype->issue_fn(vtxp, &done);
                    MPIR_Assert(mpi_errno == MPI_SUCCESS);
                    if (done)
                        vtx_record_completion(vtxp, sched);
                    else
                        vtx_record_issue(vtxp, sched);
                    break;
                }
        }

#ifdef MPL_USE_DBG_LOGGING
        /* print issued vertex list */
        {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Issued vertices list: "));
            vtx_t *vtxp_tmp;
            LL_FOREACH(sched->issued_head, vtxp_tmp) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "%d", vtxp_tmp->vtx_id));
            }
        }
#endif
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_VTX_ISSUE);
}


static void vtx_record_completion(MPII_Genutil_vtx_t * vtxp, MPII_Genutil_sched_t * sched)
{
    int i;
    UT_array *out_vtcs = vtxp->out_vtcs;

    vtxp->vtx_state = MPII_GENUTIL_VTX_STATE__COMPLETE;
    sched->completed_vtcs++;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Number of completed vertices = %d", sched->completed_vtcs));

    /* update outgoing vertices about completion of this vertex */

    /* Get the list of outgoing vertices */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Number of outgoing vertices of %d = %d", vtxp->vtx_id,
                     utarray_len(out_vtcs)));

    /* for each outgoing vertex of vertex *vtxp, decrement number of
     * unfinished dependencies */
    for (i = 0; i < utarray_len(out_vtcs); i++) {
        int outvtx_id = *(int *) utarray_eltptr(out_vtcs, i);
        int pending_dependencies =
            --(((vtx_t *) utarray_eltptr(sched->vtcs, outvtx_id))->pending_dependencies);

        /* if all dependencies of the outgoing vertex are complete,
         * issue the vertex */
        if (pending_dependencies == 0) {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "Issuing vertex number %d", outvtx_id));
            vtx_issue(outvtx_id, (vtx_t *) utarray_eltptr(sched->vtcs, outvtx_id), sched);
        }
    }

    LL_DELETE(sched->issued_head, sched->issued_tail, vtxp);
}


int MPII_Genutil_progress_hook(int *made_progress)
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
        *made_progress = FALSE;

    /* Go over up to MPIR_COLL_PROGRESS_MAX_COLLS collecives in the
     * queue and make progress on them */
    DL_FOREACH_SAFE(MPII_coll_queue.head, coll_req, coll_req_tmp) {
        /* make progress on the collective operation */
        int done;
        MPII_Genutil_sched_t *sched = (MPII_Genutil_sched_t *) (coll_req->sched);

        /* make progress on the collective */
        mpi_errno = MPII_Genutil_sched_poke(sched, &done, made_progress);

        if (done) {
            MPIR_Request *req;

            coll_req->sched = NULL;

            req = MPL_container_of(coll_req, MPIR_Request, u.nbc.coll);
            DL_DELETE(MPII_coll_queue.head, coll_req);
            MPIR_Request_complete(req);
        }
        if (++count >= MPIR_CVAR_PROGRESS_MAX_COLLS)
            break;
    }

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

    utarray_new(dst->in_vtcs, &ut_int_icd, MPL_MEM_COLL);
    utarray_concat(dst->in_vtcs, src->in_vtcs, MPL_MEM_COLL);
    utarray_new(dst->out_vtcs, &ut_int_icd, MPL_MEM_COLL);
    utarray_concat(dst->out_vtcs, src->out_vtcs, MPL_MEM_COLL);

    dst->pending_dependencies = src->pending_dependencies;
    dst->u = src->u;
    dst->next = src->next;
}


void MPII_Genutil_vtx_dtor(void *_elt)
{
    vtx_t *elt = (vtx_t *) _elt;

    utarray_free(elt->in_vtcs);
    utarray_free(elt->out_vtcs);
}


void MPII_Genutil_vtx_add_dependencies(MPII_Genutil_sched_t * sched, int vtx_id,
                                       int n_in_vtcs, int *in_vtcs)
{
    int i;
    UT_array *out_vtcs;
    MPII_Genutil_vtx_t *vtx;
    UT_array *in;

    vtx = (vtx_t *) utarray_eltptr(sched->vtcs, vtx_id);
    MPIR_Assert(vtx != NULL);
    in = vtx->in_vtcs;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "Updating in_vtcs of vtx %d, vtx_kind %d, in->used %d, n_in_vtcs %d", vtx_id,
                     vtx->vtx_kind, utarray_len(in), n_in_vtcs));

    /* insert the incoming edges */
    vtx_extend_utarray(in, n_in_vtcs, in_vtcs);

    /* update the list of outgoing vertices of the incoming
     * vertices */
    for (i = 0; i < n_in_vtcs; i++) {
        int in_vtx_id = *(int *) utarray_eltptr(in, i);
        vtx_t *in_vtx = (vtx_t *) utarray_eltptr(sched->vtcs, in_vtx_id);
        MPIR_Assert(in_vtx != NULL);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "invtx: %d", in_vtx_id));
        out_vtcs = in_vtx->out_vtcs;
        vtx_extend_utarray(out_vtcs, 1, &vtx_id);

        /* increment pending_dependencies only if the incoming
         * vertex is not complete yet */
        if (in_vtx->vtx_state != MPII_GENUTIL_VTX_STATE__COMPLETE)
            vtx->pending_dependencies++;
    }

    /* check if there was any fence operation and add appropriate dependencies.
     * The application will never explicity specify a dependency on it,
     * the transport has to make sure that the dependency on the fence operation is met */
    if (sched->last_fence != -1 && sched->last_fence != vtx_id) {
        /* add the last fence vertex as an incoming vertex to vtx */
        vtx_extend_utarray(in, 1, &(sched->last_fence));

        /* add vtx as outgoing vtx of last_fence */
        vtx_t *sched_fence = (vtx_t *) utarray_eltptr(sched->vtcs, sched->last_fence);
        MPIR_Assert(sched_fence != NULL);
        out_vtcs = sched_fence->out_vtcs;
        vtx_extend_utarray(out_vtcs, 1, &vtx_id);

        /* increment pending_dependencies only if the incoming
         * vertex is not complete yet */
        if (sched_fence->vtx_state != MPII_GENUTIL_VTX_STATE__COMPLETE)
            vtx->pending_dependencies++;
    }
}


int MPII_Genutil_vtx_create(MPII_Genutil_sched_t * sched, MPII_Genutil_vtx_t ** vtx)
{
    MPII_Genutil_vtx_t *vtxp;

    utarray_extend_back(sched->vtcs, MPL_MEM_COLL);
    *vtx = (vtx_t *) utarray_back(sched->vtcs);
    vtxp = *vtx;

    /* allocate memory for storing incoming and outgoing vertices */
    utarray_new(vtxp->in_vtcs, &ut_int_icd, MPL_MEM_COLL);
    utarray_new(vtxp->out_vtcs, &ut_int_icd, MPL_MEM_COLL);

    vtxp->vtx_state = MPII_GENUTIL_VTX_STATE__INIT;
    vtxp->vtx_id = sched->total_vtcs++;
    vtxp->pending_dependencies = 0;
    vtxp->next = NULL;

    return vtxp->vtx_id;        /* return vertex vtx_id */
}


int MPII_Genutil_sched_poke(MPII_Genutil_sched_t * sched, int *is_complete, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    vtx_t *vtxp, *vtxp_tmp;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPII_GENUTIL_SCHED_POKE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPII_GENUTIL_SCHED_POKE);

    if (sched->total_vtcs == 0) /* NULL schedule, nothing to issue */
        goto done;

    if (made_progress)
        *made_progress = FALSE;

    *is_complete = FALSE;

    /* If issued list is empty, go over the vertices and issue ready
     * vertices.  Issued list should be empty only when the
     * MPII_Genutil_sched_poke function is called for the first time
     * on the schedule. */
    if (sched->issued_head == NULL) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "issued list is empty, issue ready vtcs"));

        if (made_progress)
            *made_progress = TRUE;

        /* Go over all the vertices and issue ready vertices */
        for (i = 0; i < sched->total_vtcs; i++) {
            vtx_issue(i, (vtx_t *) utarray_eltptr(sched->vtcs, i), sched);
        }

        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "completed traversal of vtcs, sched->total_vtcs: %d, sched->completed_vtcs: %d",
                         sched->total_vtcs, sched->completed_vtcs));
    }

    LL_FOREACH_SAFE(sched->issued_head, vtxp, vtxp_tmp) {
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
                    vtx_record_completion(vtxp, sched);
                    if (made_progress)
                        *made_progress = TRUE;
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
                    vtx_record_completion(vtxp, sched);
                    if (made_progress)
                        *made_progress = TRUE;
                }
                break;

            case MPII_GENUTIL_VTX_KIND__IMCAST:
                for (i = vtxp->u.imcast.last_complete + 1; i < vtxp->u.imcast.num_dests; i++) {
                    if (MPIR_Request_is_complete(vtxp->u.imcast.req[i])) {
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                        (MPL_DBG_FDEST,
                                         "  --> GENTRAN transport imcast vertex %d complete", i));
                        MPIR_Request_free(vtxp->u.imcast.req[i]);
                        vtxp->u.imcast.req[i] = NULL;
                        vtxp->u.imcast.last_complete = i;
                        if (made_progress)
                            *made_progress = TRUE;
                    } else {
                        /* we are only checking in sequence, hence break
                         * out at the first incomplete isend */
                        break;
                    }
                }
                if (i == vtxp->u.imcast.num_dests)
                    vtx_record_completion(vtxp, sched);
                break;

            case MPII_GENUTIL_VTX_KIND__ISSEND:
                if (MPIR_Request_is_complete(vtxp->u.issend.req)) {
                    MPIR_Request_free(vtxp->u.issend.req);
                    vtxp->u.issend.req = NULL;
#ifdef MPL_USE_DBG_LOGGING
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST,
                                     "  --> GENTRAN transport (vtx_kind=%d) complete",
                                     vtxp->vtx_kind));
                    if (vtxp->u.issend.count >= 1)
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                        (MPL_DBG_FDEST, "data sent: %d",
                                         *(int *) (vtxp->u.issend.buf)));
#endif
                    vtx_record_completion(vtxp, sched);
                    if (made_progress)
                        *made_progress = TRUE;
                }
                break;

            default:
                if (vtxp->vtx_kind > MPII_GENUTIL_VTX_KIND__LAST) {
                    int is_completed;
                    MPII_Genutil_vtx_type_t *vtype = ut_type_array(&sched->generic_types,
                                                                   MPII_Genutil_vtx_type_t *) +
                        vtxp->vtx_kind - MPII_GENUTIL_VTX_KIND__LAST - 1;
                    MPIR_Assert(vtype != NULL);
                    mpi_errno = vtype->complete_fn(vtxp, &is_completed);
                    MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno,
                                        MPI_ERR_OTHER, "**other");
                    if (is_completed) {
                        vtx_record_completion(vtxp, sched);
                        if (made_progress)
                            *made_progress = TRUE;
                    }
                }
                break;
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

  done:
    *is_complete = (sched->completed_vtcs == sched->total_vtcs);
    if (*is_complete) {
        if (made_progress)
            *made_progress = TRUE;

        MPII_Genutil_sched_free(sched);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPII_GENUTIL_SCHED_POKE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
