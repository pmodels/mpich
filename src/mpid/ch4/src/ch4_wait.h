/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_WAIT_H_INCLUDED
#define CH4_WAIT_H_INCLUDED

#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_add_vci_to_state(int vci,
                                                     MPID_Progress_state * state) 
{
    MPIR_Assert(vci < MPIDI_CH4_MAX_VCIS);
    for (int i = 0; i < state->vci_count; ++i) {
        if (state->vci[i] == vci) {
            return;
        }
    }
    MPIR_Assert(state->vci_count < MPIDI_CH4_MAX_VCIS);
    state->vci[state->vci_count++] = vci;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_add_progress_vci_cont(MPIR_Request * req,
                                                          MPID_Progress_state * state) 
{
    MPIR_Assert(req->kind == MPIR_REQUEST_KIND__CONTINUE);
    for (int i = 0; i < MPIDI_CH4_MAX_VCIS; ++i) {
        if (MPL_atomic_relaxed_load_int64(&req->u.cont.state->vci_refcount[i].val) > 0) {
            MPIDI_add_vci_to_state(i, state);
        }
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_set_progress_vci(MPIR_Request * req,
                                                     MPID_Progress_state * state)
{
    state->flag = MPIDI_PROGRESS_ALL;   /* TODO: check request is_local/anysource */

    state->vci_count = 0;
    if (req->kind == MPIR_REQUEST_KIND__CONTINUE) {
        MPIDI_add_progress_vci_cont(req, state);
    } else {
        int vci = MPIDI_Request_get_vci(req);

        state->vci_count = 1;
        state->vci[0] = vci;
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_set_progress_vci_n(int n, MPIR_Request ** reqs,
                                                       MPID_Progress_state * state)
{
    state->flag = MPIDI_PROGRESS_ALL;   /* TODO: check request is_local/anysource */

    state->vci_count = 0;
    int idx = 0;
    for (int i = 0; i < n; i++) {
        if (!MPIR_Request_is_active(reqs[i])) {
            continue;
        }

        if (HANDLE_IS_BUILTIN(reqs[i]->handle) || MPIR_Request_is_complete(reqs[i])) {
            continue;
        }

        if (reqs[i]->kind == MPIR_REQUEST_KIND__CONTINUE) {
            MPIDI_add_progress_vci_cont(reqs[i], state);
        } else {
            int vci = MPIDI_Request_get_vci(reqs[i]);
            MPIDI_add_vci_to_state(vci, state);
        }
    }
    state->vci_count = idx;
}

/* MPID_Test, MPID_Testall, MPID_Testany, MPID_Testsome */

MPL_STATIC_INLINE_PREFIX int MPID_Test(MPIR_Request * request_ptr, int *flag, MPI_Status * status)
{
    MPID_Progress_state progress_state;

    MPIDI_set_progress_vci(request_ptr, &progress_state);
    return MPIR_Test_state(request_ptr, flag, status, &progress_state);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testall(int count, MPIR_Request * request_ptrs[],
                                          int *flag, MPI_Status array_of_statuses[],
                                          int requests_property)
{
    MPID_Progress_state progress_state;

    MPIDI_set_progress_vci_n(count, request_ptrs, &progress_state);
    return MPIR_Testall_state(count, request_ptrs, flag, array_of_statuses, requests_property,
                              &progress_state);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testany(int count, MPIR_Request * request_ptrs[],
                                          int *indx, int *flag, MPI_Status * status)
{
    MPID_Progress_state progress_state;

    MPIDI_set_progress_vci_n(count, request_ptrs, &progress_state);
    return MPIR_Testany_state(count, request_ptrs, indx, flag, status, &progress_state);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testsome(int incount, MPIR_Request * request_ptrs[],
                                           int *outcount, int array_of_indices[],
                                           MPI_Status array_of_statuses[])
{
    MPID_Progress_state progress_state;

    MPIDI_set_progress_vci_n(incount, request_ptrs, &progress_state);
    return MPIR_Testsome_state(incount, request_ptrs, outcount, array_of_indices,
                               array_of_statuses, &progress_state);
}

/* MPID_Wait, MPID_Waitall, MPID_Waitany, MPID_Waitsome */

MPL_STATIC_INLINE_PREFIX int MPID_Wait(MPIR_Request * request_ptr, MPI_Status * status)
{
    MPID_Progress_state progress_state;

    MPIDI_set_progress_vci(request_ptr, &progress_state);
    return MPIR_Wait_state(request_ptr, status, &progress_state);
}

MPL_STATIC_INLINE_PREFIX int MPID_Waitall(int count, MPIR_Request * request_ptrs[],
                                          MPI_Status array_of_statuses[], int request_properties)
{
    MPID_Progress_state progress_state;

    MPIDI_set_progress_vci_n(count, request_ptrs, &progress_state);
    return MPIR_Waitall_state(count, request_ptrs, array_of_statuses,
                              request_properties, &progress_state);
}

MPL_STATIC_INLINE_PREFIX int MPID_Waitany(int count, MPIR_Request * request_ptrs[],
                                          int *indx, MPI_Status * status)
{
    MPID_Progress_state progress_state;

    MPIDI_set_progress_vci_n(count, request_ptrs, &progress_state);
    return MPIR_Waitany_state(count, request_ptrs, indx, status, &progress_state);
}

MPL_STATIC_INLINE_PREFIX int MPID_Waitsome(int incount, MPIR_Request * request_ptrs[],
                                           int *outcount, int array_of_indices[],
                                           MPI_Status array_of_statuses[])
{
    MPID_Progress_state progress_state;

    MPIDI_set_progress_vci_n(incount, request_ptrs, &progress_state);
    return MPIR_Waitsome_state(incount, request_ptrs, outcount, array_of_indices,
                               array_of_statuses, &progress_state);
}

#endif /* CH4_WAIT_H_INCLUDED */
