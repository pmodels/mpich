/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_WAIT_H_INCLUDED
#define CH4_WAIT_H_INCLUDED

#include "ch4_impl.h"

/* append the VCI to the vci_count existing progress state */
MPL_STATIC_INLINE_PREFIX void MPIDI_append_progress_vci_n(int n, MPIR_Request **reqs,
                                                          MPID_Progress_state *state) {
    state->flag = MPIDI_PROGRESS_ALL; /* TODO: check request is_local/anysource */
    state->progress_made = 0;
    int idx = state->vci_count;
    for (int i = 0; i < n; i++) {
        if (!MPIR_Request_is_active(reqs[i])) {
            continue;
        }

        if (HANDLE_IS_BUILTIN(reqs[i]->handle) || MPIR_Request_is_complete(reqs[i])) {
            continue;
        }

        int vci = MPIDI_Request_get_vci(reqs[i]);
        int found = 0;
        for (int j = 0; j < idx; j++) {
            if (state->vci[j] == vci) {
                found = 1;
                break;
            }
        }
        if (!found) {
            state->vci[idx++] = vci;
        }
    }
    state->vci_count = idx;
    for (int i = state->vci_count; i < idx; i++) {
        int vci = state->vci[i];
        state->progress_counts[i] = MPL_atomic_relaxed_load_int(&MPIDI_VCI(vci).progress_count);
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_set_progress_vci_n(int n, MPIR_Request **reqs,
                                                       MPID_Progress_state *state) {
    state->vci_count = 0;
    MPIDI_append_progress_vci_n(n, reqs, state);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_set_progress_vci(MPIR_Request *req,
                                                     MPID_Progress_state *state) {
    state->flag = MPIDI_PROGRESS_ALL; /* TODO: check request is_local/anysource */
    state->progress_made = 0;

    int vci = MPIDI_Request_get_vci(req);
    state->progress_counts[0] = MPL_atomic_relaxed_load_int(&MPIDI_VCI(vci).progress_count);
    state->vci_count = 1;
    state->vci[0] = vci;

    /* try to see if we have some child requests
     * we append the child's VCI to make sure we progress on both the main request and the child
     * ones. This avoids deadlocks when waiting for CTS/RTS etc (typical in partitioned comm)*/
    int n_child_req = 0;
    MPIR_Request **child_req = NULL;
    /* TODO: we might want to skip the requests already completed? when doing so we could also not
     * include the main request's VCI */
    if (req->kind == MPIR_REQUEST_KIND__PART_SEND) {
        child_req = MPIDIG_PART_SREQUEST(req, tag_req_ptr);
        n_child_req = MPIDIG_PART_REQUEST(req, msg_part);
    } else if (req->kind == MPIR_REQUEST_KIND__PART_RECV) {
        child_req = MPIDIG_PART_RREQUEST(req, tag_req_ptr);
        n_child_req = MPIDIG_PART_REQUEST(req, msg_part);
    }
    if (n_child_req > 0 && child_req) {
        MPIDI_append_progress_vci_n(n_child_req, child_req, state);
    }
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
