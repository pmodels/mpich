/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Continue object: a wrapper to a continue callback */
struct MPIR_Continue {
    MPIR_Request *cont_req;
    MPIX_Continue_cb_function *cb;
    void *cb_data;
    MPIR_cc_t pending_request_count;
    struct MPIR_Continue *next;
    bool is_immediate;
};
typedef struct MPIR_Continue MPIR_Continue;

/* Continue context object: carrying data for each op request */
struct MPIR_Continue_context {
    struct MPIR_Continue* continue_ptr;
    MPI_Status *status_ptr;
    /* Used by the on-hold list */
    struct MPIR_Continue_context *next;
    MPIR_Request *op_request;
};
typedef struct MPIR_Continue_context MPIR_Continue_context;

struct {
    struct MPIR_Continue *head, *tail;
    MPID_Thread_mutex_t lock;
} g_deferred_cont_list = {NULL, NULL};

__thread struct {
    struct MPIR_Continue *head, *tail;
} tls_deferred_cont_list = {NULL, NULL};

void complete_op_request(MPIR_Request *op_request, bool in_cs, void *cb_context, bool defer_complete, bool in_request_callback);
void MPIR_Continue_callback(MPIR_Request *op_request, bool in_cs, void *cb_context);
void attach_continue_context(MPIR_Continue_context *context_ptr, bool defer_complete);

void MPIR_Continue_global_init()
{
    g_deferred_cont_list.head = NULL;
    g_deferred_cont_list.tail = NULL;
    int err;
    MPID_Thread_mutex_create(&g_deferred_cont_list.lock, &err);
    MPIR_Assert(err == 0);
}

void MPIR_Continue_global_finalize()
{
    MPIR_Assert(g_deferred_cont_list.head == NULL);
    MPIR_Assert(g_deferred_cont_list.tail == NULL);
    int err;
    MPID_Thread_mutex_destroy(&g_deferred_cont_list.lock, &err);
    MPIR_Assert(err == 0);
}

int MPIR_Continue_init_impl(int flags, int max_poll,
                            MPIR_Info *info_ptr,
                            MPIR_Request **cont_req_ptr)
{
    MPIR_Request *cont_req = MPIR_Request_create(MPIR_REQUEST_KIND__CONTINUE);
    MPIR_Cont_request_inactivate(cont_req);
    /* We use cc to track how many continue object has been attached to this continuation request. */
    MPIR_cc_set(&cont_req->cc, 0);
    {
        int err;
        MPID_Thread_mutex_create(&cont_req->u.cont.cont_context_on_hold_list.lock, &err);
        MPIR_Assert(err == 0);
        MPID_Thread_mutex_create(&cont_req->u.cont.ready_poll_only_cont_list.lock, &err);
        MPIR_Assert(err == 0);
    }
    /* Initialize the on-hold context list */
    cont_req->u.cont.cont_context_on_hold_list.head = NULL;
    cont_req->u.cont.cont_context_on_hold_list.tail = NULL;
    /* Initialize the poll-only continue list */
    cont_req->u.cont.ready_poll_only_cont_list.head = NULL;
    cont_req->u.cont.ready_poll_only_cont_list.tail = NULL;
    cont_req->u.cont.is_pool_only = flags & MPIX_CONT_POLL_ONLY;
    cont_req->u.cont.max_poll = max_poll;
    cont_req->u.cont.state = (MPID_Progress_state_cnt *) MPL_malloc(sizeof(MPID_Progress_state_cnt), MPL_MEM_OTHER);
    for (int i = 0; i < MPIDI_CH4_MAX_VCIS; ++i) {
        MPL_atomic_release_store_uint64(&cont_req->u.cont.state->vci_refcount[i].val, 0);
    }
    *cont_req_ptr = cont_req;
    return MPI_SUCCESS;
}

void MPIR_Continue_destroy_impl(MPIR_Request *cont_req)
{
    MPL_free(cont_req->u.cont.state);
    MPIR_Assert(cont_req->kind == MPIR_REQUEST_KIND__CONTINUE);
    {
        int err;
        MPID_Thread_mutex_destroy(&cont_req->u.cont.cont_context_on_hold_list.lock, &err);
        MPIR_Assert(err == 0);
        MPID_Thread_mutex_destroy(&cont_req->u.cont.ready_poll_only_cont_list.lock, &err);
        MPIR_Assert(err == 0);
    }
    MPIR_Assert(cont_req->u.cont.cont_context_on_hold_list.head == NULL);
    MPIR_Assert(cont_req->u.cont.cont_context_on_hold_list.tail == NULL);
    MPIR_Assert(cont_req->u.cont.ready_poll_only_cont_list.head == NULL);
    MPIR_Assert(cont_req->u.cont.ready_poll_only_cont_list.tail == NULL);
}

int MPIR_Continue_start(MPIR_Request * cont_request_ptr)
{
    struct MPIR_Continue_context *tmp_head = NULL, *tmp_tail = NULL;
    MPID_THREAD_CS_ENTER(VCI, cont_request_ptr->u.cont.cont_context_on_hold_list.lock);
    MPIR_Cont_request_activate(cont_request_ptr);
    if (cont_request_ptr->u.cont.cont_context_on_hold_list.head) {
        tmp_head = cont_request_ptr->u.cont.cont_context_on_hold_list.head;
        tmp_tail = cont_request_ptr->u.cont.cont_context_on_hold_list.tail;
        cont_request_ptr->u.cont.cont_context_on_hold_list.head = NULL;
        cont_request_ptr->u.cont.cont_context_on_hold_list.tail = NULL;
    }
    MPID_THREAD_CS_EXIT(VCI, cont_request_ptr->u.cont.cont_context_on_hold_list.lock);
    /* Attach those on-hold continue context */
    while (tmp_head) {
        MPIR_Continue_context *context_ptr = tmp_head;
        LL_DELETE(tmp_head, tmp_tail, context_ptr);
        attach_continue_context(context_ptr, false);
    }
    return MPI_SUCCESS;
}

void attach_continue_context(MPIR_Continue_context *context_ptr, bool defer_complete) {
    /* record the corresponding VCI for the continuation request to progress */
    if (context_ptr->continue_ptr->cont_req) {
        int vci = MPIDI_Request_get_vci(context_ptr->op_request);
        MPL_atomic_fetch_add_int(&context_ptr->continue_ptr->cont_req->u.cont.state->vci_refcount[vci].val, 1);
    }
    /* Attach the continue context to the op request */
    if (!MPIR_Register_callback(context_ptr->op_request, MPIR_Continue_callback, context_ptr, false)) {
        /* the request has already been completed. */
        complete_op_request(context_ptr->op_request, false, context_ptr, defer_complete, false);
    }
}

int MPIR_Continue_impl(MPIR_Request *op_request_ptr,
                       MPIX_Continue_cb_function *cb, void *cb_data,
                       int flags, MPI_Status *status,
                       MPIR_Request *cont_request_ptr)
{
    return MPIR_Continueall_impl(1, &op_request_ptr, cb, cb_data, flags, status, cont_request_ptr);
}

int MPIR_Continueall_impl(int count, MPIR_Request *request_ptrs[],
                          MPIX_Continue_cb_function *cb, void *cb_data, int flags,
                          MPI_Status *array_of_statuses, MPIR_Request *cont_request_ptr)
{
    if (cont_request_ptr) {
        /* Add one continue to the continuation request */
        int was_incompleted;
        MPIR_cc_incr(cont_request_ptr->cc_ptr, &was_incompleted);
        if (!was_incompleted) {
            MPIR_Request_add_ref(cont_request_ptr);
            /* A hack for now since continuation request can jump
             * between complete and incomplete multiple times */
            cont_request_ptr->cbs_invoked = false;
        }
    }
    /* Set various condition variables */
    bool defer_complete = flags & MPIX_CONT_DEFER_COMPLETE;
    /* Create the continue object for every continue callback */
    MPIR_Continue *continue_ptr = (MPIR_Continue *) MPL_malloc(sizeof(MPIR_Continue), MPL_MEM_OTHER);
    continue_ptr->cont_req = cont_request_ptr;
    continue_ptr->cb = cb;
    continue_ptr->cb_data = cb_data;
    MPIR_cc_set(&continue_ptr->pending_request_count, count);
    if (flags & MPIX_CONT_IMMEDIATE)
        continue_ptr->is_immediate = true;
    else
        continue_ptr->is_immediate = false;

    for (int i = 0; i < count; i++) {
        /* Create the continue context object for every op request */
        MPIR_Continue_context *context_ptr = (MPIR_Continue_context *) MPL_malloc(sizeof(MPIR_Continue_context), MPL_MEM_OTHER);
        context_ptr->continue_ptr = continue_ptr;
        MPIR_Assert(MPI_STATUS_IGNORE == MPI_STATUSES_IGNORE);
        if (array_of_statuses != MPI_STATUS_IGNORE) {
            context_ptr->status_ptr = &array_of_statuses[i];
        } else {
            context_ptr->status_ptr = MPI_STATUS_IGNORE;
        }
        context_ptr->op_request = request_ptrs[i];
        /* if the continue request is not activated yet, do not attach */
        bool is_on_hold = false;
        if (cont_request_ptr && !MPIR_Cont_request_is_active(cont_request_ptr)) {
            MPID_THREAD_CS_ENTER(VCI, cont_request_ptr->u.cont.cont_context_on_hold_list.lock);
            if (!MPIR_Cont_request_is_active(cont_request_ptr)) {
                /* The continuation request is inactive. Do not attach yet. */
                LL_APPEND(cont_request_ptr->u.cont.cont_context_on_hold_list.head,
                          cont_request_ptr->u.cont.cont_context_on_hold_list.tail,
                          context_ptr);
                is_on_hold = true;
            }
            MPID_THREAD_CS_EXIT(VCI, cont_request_ptr->u.cont.cont_context_on_hold_list.lock);
        }
        /* attach the continue context to op request */
        if (!is_on_hold) {
            attach_continue_context(context_ptr, defer_complete);
        }
    }
    return MPI_SUCCESS;
}

void execute_continue(MPIR_Continue *continue_ptr, bool in_cs, int which_cs)
{
    MPIR_Request *cont_req_ptr = continue_ptr->cont_req;
    /* Invoke the continue callback */
    continue_ptr->cb(MPI_SUCCESS, continue_ptr->cb_data);
    MPL_free(continue_ptr);
    /* Signal the continuation request */
    /* TODO: Find a suitable request complete function for continuation requests */
    if (cont_req_ptr) {
        int incomplete;
        MPIR_cc_decr(cont_req_ptr->cc_ptr, &incomplete);
        if (!incomplete) {
            /* TODO: reason about the safety of invoking the callback for continuation request here*/
//        MPIR_Invoke_callback(cont_req_ptr, false);
            MPIR_Request_free_with_safety(cont_req_ptr, !(in_cs && MPIR_REQUEST_POOL(cont_req_ptr) == which_cs), NULL);
        }
    }
}

void complete_op_request(MPIR_Request *op_request, bool in_cs, void *cb_context, bool defer_complete, bool in_request_callback)
{
    MPIR_Continue_context *context_ptr = (MPIR_Continue_context *) cb_context;
    MPIR_Continue *continue_ptr = context_ptr->continue_ptr;
    /* Decrease the continuation request VCI counter */
    MPIR_Request *cont_req_ptr = continue_ptr->cont_req;
    if (cont_req_ptr) {
        int vci = MPIDI_Request_get_vci(op_request);
        MPL_atomic_fetch_sub_int(&cont_req_ptr->u.cont.state->vci_refcount[vci].val, 1);
    }
    /* Complete this operation request */
    /* FIXME: MPIR_Request_completion_processing can call MPIR_Request_free,
     * which might lead to deadlock */
    int rc = MPIR_Request_completion_processing(
            op_request, context_ptr->status_ptr);
    if (context_ptr->status_ptr != MPI_STATUS_IGNORE)
        context_ptr->status_ptr->MPI_ERROR = rc;
    if (!MPIR_Request_is_persistent(op_request)) {
        MPIR_Request_free_with_safety(op_request, !in_cs, NULL);
    }
    MPL_free(context_ptr);
    /* Signal the continue callback */
    int incomplete;
    MPIR_cc_decr(&continue_ptr->pending_request_count, &incomplete);
    if (!incomplete) {
        /* All the op requests associated with this continue callback have completed */
        MPIR_Request *cont_req_ptr = continue_ptr->cont_req;
        if (cont_req_ptr && cont_req_ptr->u.cont.is_pool_only) {
            // Pool-only continuation request
            // Push to the continuation request local ready list
            MPID_THREAD_CS_ENTER(VCI, cont_req_ptr->u.cont.ready_poll_only_cont_list.lock);
            LL_APPEND(cont_req_ptr->u.cont.ready_poll_only_cont_list.head,
                      cont_req_ptr->u.cont.ready_poll_only_cont_list.tail,
                      continue_ptr);
            MPID_THREAD_CS_EXIT(VCI, cont_req_ptr->u.cont.ready_poll_only_cont_list.lock);
        } else if (defer_complete) {
            // Deferred completion.
            MPID_THREAD_CS_ENTER(VCI, g_deferred_cont_list.lock);
            LL_APPEND(g_deferred_cont_list.head,
                      g_deferred_cont_list.tail,
                      continue_ptr);
            MPID_THREAD_CS_EXIT(VCI, g_deferred_cont_list.lock);
        } else if (in_cs && !continue_ptr->is_immediate) {
            // General-purpose continuation request. We are in a VCI CS
            // Push to the tls ready list
            LL_APPEND(tls_deferred_cont_list.head,
                      tls_deferred_cont_list.tail,
                      continue_ptr);
        } else {
            execute_continue(continue_ptr, in_cs, MPIR_REQUEST_POOL(op_request));
        }
    }

}

void MPIR_Continue_callback(MPIR_Request *op_request, bool in_cs, void *cb_context)
{
    complete_op_request(op_request, in_cs, cb_context, false, true);
}

int MPIR_Continue_progress_tls()
{
    int count = 0;
    while (tls_deferred_cont_list.head) {
        /* We have to poll all the things to ensure progress */
        MPIR_Continue *continue_ptr = tls_deferred_cont_list.head;
        LL_DELETE(tls_deferred_cont_list.head, tls_deferred_cont_list.tail, continue_ptr);
        execute_continue(continue_ptr, false, 0 /* Does not matter */);
        ++count;
    }
    return count;
}

int MPIR_Continue_progress_request(MPIR_Request *cont_request_ptr)
{
    MPIR_Assert(cont_request_ptr && cont_request_ptr->kind == MPIR_REQUEST_KIND__CONTINUE);
    int count = 0;
    if (cont_request_ptr->u.cont.ready_poll_only_cont_list.head) {
        struct MPIR_Continue *local_head = NULL, *local_tail = NULL;
        MPID_THREAD_CS_ENTER(VCI, cont_request_ptr->u.cont.ready_poll_only_cont_list.lock);
        /* TODO: use a more efficient way to pop this list */
        while (cont_request_ptr->u.cont.ready_poll_only_cont_list.head) {
            MPIR_Continue *continue_ptr = cont_request_ptr->u.cont.ready_poll_only_cont_list.head;
            LL_DELETE(cont_request_ptr->u.cont.ready_poll_only_cont_list.head,
                      cont_request_ptr->u.cont.ready_poll_only_cont_list.tail,
                      continue_ptr);
            LL_APPEND(local_head, local_tail, continue_ptr);
            if (cont_request_ptr->u.cont.max_poll && ++count >= cont_request_ptr->u.cont.max_poll)
                break;
        }
        MPID_THREAD_CS_EXIT(VCI, cont_request_ptr->u.cont.ready_poll_only_cont_list.lock);
        while (local_head) {
            MPIR_Continue *continue_ptr = local_head;
            LL_DELETE(local_head, local_tail, continue_ptr);
            execute_continue(continue_ptr, false, 0 /* Does not matter */);
        }
    }
    return count;
}

void MPIR_Continue_progress(MPIR_Request *request)
{
    int count = 0;
    int max_poll = 0; // By default we poll unlimited time
    if (request && request->kind == MPIR_REQUEST_KIND__CONTINUE) {
        // This is a continuation request
        count += MPIR_Continue_progress_request(request);
        max_poll = request->u.cont.max_poll;
    }
    // make progress on the global list
    if (g_deferred_cont_list.head) {
        struct MPIR_Continue *local_head = NULL, *local_tail = NULL;
        MPID_THREAD_CS_ENTER(VCI, g_deferred_cont_list.lock);
        /* TODO: use a more efficient way to pop this list */
        while ((!max_poll || count < max_poll) && g_deferred_cont_list.head) {
            MPIR_Continue *continue_ptr = g_deferred_cont_list.head;
            LL_DELETE(g_deferred_cont_list.head,
                      g_deferred_cont_list.tail,
                      continue_ptr);
            LL_APPEND(local_head, local_tail, continue_ptr);
            ++count;
        }
        MPID_THREAD_CS_EXIT(VCI, g_deferred_cont_list.lock);
        while (local_head) {
            MPIR_Continue *continue_ptr = local_head;
            LL_DELETE(local_head, local_tail, continue_ptr);
            execute_continue(continue_ptr, false, 0 /* Does not matter */);
        }
    }
}