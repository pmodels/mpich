/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <assert.h>
#include "mpiimpl.h"

struct MPII_Nsched_active_comm *MPII_Nsched_active_comm_list_head = NULL;
struct MPII_Nsched_active_comm *MPII_Nsched_active_comm_list_tail = NULL;

int MPIR_Nsched_are_pending(void)
{
    return !!MPII_Nsched_active_comm_list_head;
}

int MPIR_Nsched_kickoff(MPIR_Nsched * s, struct MPIR_Comm *comm)
{
    if (comm->nsched_list_head == NULL) {
        struct MPII_Nsched_active_comm *ac;

        ac = MPL_malloc(sizeof(struct MPII_Nsched_active_comm));

        ac->comm = comm;
        ac->next = NULL;

        if (MPII_Nsched_active_comm_list_head == NULL)
            MPID_Progress_activate_hook(MPIR_Nsched_progress_hook_id);

        MPII_NSCHED_ACTIVE_COMM_ENQUEUE(ac);
    }

    MPII_NSCHED_SCHED_ENQUEUE(comm, s);

    return MPI_SUCCESS;
}

static int make_sched_progress(MPIR_Nsched * s, int *made_progress)
{
    MPII_Nsched_elm *e, *tmp;

    *made_progress = 0;

    if (s->elm_list_head == NULL)
        return MPI_SUCCESS;

  sync_point:
    /* if the first entry is a sync, remove it */
    while (s->elm_list_head->type == MPII_NSCHED_ELM_TYPE__SYNC) {
        tmp = s->elm_list_head;
        MPII_NSCHED_ELM_DEQUEUE(s, tmp);
        MPL_free(tmp);

        *made_progress = 1;

        if (s->elm_list_head == NULL)
            return MPI_SUCCESS;
    }

    /* now the first entry is not a sync, so we can issue all pending
     * operations till before the next sync */
    MPL_LL_FOREACH(s->elm_list_head, e) {
        if (e->type == MPII_NSCHED_ELM_TYPE__SYNC)
            break;
        if (e->issued)
            continue;

        e->issued = 1;

        /* found a pending op, issue it */
        if (e->type == MPII_NSCHED_ELM_TYPE__ISEND) {
            int context_id;

            if (e->u.isend.comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
                context_id = MPIR_CONTEXT_INTRA_PT2PT;
            else
                context_id = MPIR_CONTEXT_INTER_PT2PT;

            MPID_Isend(e->u.isend.buf, e->u.isend.count, e->u.isend.datatype,
                       e->u.isend.dest, e->u.isend.tag, e->u.isend.comm_ptr, context_id, &e->req);
        }
        else if (e->type == MPII_NSCHED_ELM_TYPE__IRECV) {
            int context_id;

            if (e->u.irecv.comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
                context_id = MPIR_CONTEXT_INTRA_PT2PT;
            else
                context_id = MPIR_CONTEXT_INTER_PT2PT;

            MPID_Irecv(e->u.irecv.buf, e->u.irecv.count, e->u.irecv.datatype,
                       e->u.irecv.dest, e->u.irecv.tag, e->u.irecv.comm_ptr, context_id, &e->req);
        }
        else {
            assert(0);
        }

        *made_progress = 1;
    }

    /* all ops that could have been issued have been issued, now we
     * cleanup everything that has completed */
    MPL_LL_FOREACH(s->elm_list_head, e) {
        /* when we encounter the first unissued op, we break out */
        if (e->issued == 0)
            break;

        /* issued and request is NULL, free it */
        if (e->req == NULL) {
            MPII_NSCHED_ELM_DEQUEUE(s, e);
            MPL_free(e);
            *made_progress = 1;
        }
        else if (MPIR_Request_is_complete(e->req)) {
            MPIR_Request_free(e->req);
            MPII_NSCHED_ELM_DEQUEUE(s, e);
            MPL_free(e);
            *made_progress = 1;
        }
    }

    if (s->elm_list_head->type == MPII_NSCHED_ELM_TYPE__SYNC)
        goto sync_point;

    return MPI_SUCCESS;
}

int MPIR_Nsched_progress_hook(int *made_progress)
{
    struct MPII_Nsched_active_comm *ac;
    MPIR_Nsched *s;
    int tmp;

    *made_progress = 0;
    MPL_LL_FOREACH(MPII_Nsched_active_comm_list_head, ac) {
        MPL_LL_FOREACH(ac->comm->nsched_list_head, s) {
            make_sched_progress(s, &tmp);
            *made_progress |= tmp;

            if (s->elm_list_head == NULL) {
                MPID_Request_complete(s->req);
                MPII_NSCHED_SCHED_DEQUEUE(ac->comm, s);
                MPL_free(s);
            }
        }

        if (ac->comm->nsched_list_head == NULL) {
            MPII_NSCHED_ACTIVE_COMM_DEQUEUE(ac);
            MPL_free(ac);
        }
    }

    if (MPII_Nsched_active_comm_list_head == NULL)
        MPID_Progress_deactivate_hook(MPIR_Nsched_progress_hook_id);

    return MPI_SUCCESS;
}
