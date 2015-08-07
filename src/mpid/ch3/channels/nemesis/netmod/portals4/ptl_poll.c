/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"
#include "rptl.h"

#define OVERFLOW_LENGTH (1024*1024)
#define NUM_OVERFLOW_ME 8

static ptl_handle_me_t overflow_me_handle[NUM_OVERFLOW_ME];
static void *overflow_buf[NUM_OVERFLOW_ME];

static int append_overflow(int i);

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_poll_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_poll_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIU_CHKPMEM_DECL(NUM_OVERFLOW_ME);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_POLL_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_POLL_INIT);

    /* create overflow buffers */
    for (i = 0; i < NUM_OVERFLOW_ME; ++i) {
        MPIU_CHKPMEM_MALLOC(overflow_buf[i], void *, OVERFLOW_LENGTH, mpi_errno, "overflow buffer");
        mpi_errno = append_overflow(i);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
 fn_exit:
    MPIU_CHKPMEM_COMMIT();
 fn_exit2:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_POLL_INIT);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit2;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_poll_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_poll_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_POLL_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_POLL_FINALIZE);
    
    for (i = 0; i < NUM_OVERFLOW_ME; ++i) {
        if (overflow_me_handle[i] != PTL_INVALID_HANDLE) {
            ret = PtlMEUnlink(overflow_me_handle[i]);
            MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeunlink", "**ptlmeunlink %s", MPID_nem_ptl_strerror(ret));
        }
        MPIU_Free(overflow_buf[i]);
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_POLL_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME append_overflow
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int append_overflow(int i)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    ptl_me_t me;
    ptl_process_t id_any;
    MPIDI_STATE_DECL(MPID_STATE_APPEND_OVERFLOW);

    MPIDI_FUNC_ENTER(MPID_STATE_APPEND_OVERFLOW);

    MPIU_Assert(i >= 0 && i < NUM_OVERFLOW_ME);
    
    id_any.phys.pid = PTL_PID_ANY;
    id_any.phys.nid = PTL_NID_ANY;

    me.start = overflow_buf[i];
    me.length = OVERFLOW_LENGTH;
    me.ct_handle = PTL_CT_NONE;
    me.uid = PTL_UID_ANY;
    me.options = ( PTL_ME_OP_PUT | PTL_ME_MANAGE_LOCAL | PTL_ME_NO_TRUNCATE | PTL_ME_MAY_ALIGN |
                   PTL_ME_IS_ACCESSIBLE | PTL_ME_EVENT_LINK_DISABLE );
    me.match_id = id_any;
    me.match_bits = 0;
    me.ignore_bits = ~((ptl_match_bits_t)0);
    me.min_free = PTL_LARGE_THRESHOLD;
    
    /* if there is no space to append the entry, process outstanding events and try again */
    ret = PtlMEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_pt, &me, PTL_OVERFLOW_LIST, (void *)(size_t)i,
                      &overflow_me_handle[i]);
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend", "**ptlmeappend %s", MPID_nem_ptl_strerror(ret));

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_APPEND_OVERFLOW);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_poll
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_poll(int is_blocking_poll)
{
    int mpi_errno = MPI_SUCCESS;
    ptl_event_t event;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_POLL);

    /* MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_POLL); */

    while (1) {
        int ctl_event = FALSE;

        /* Check the rptls EQ first. It should never return an event. */
        ret = MPID_nem_ptl_rptl_eqget(MPIDI_nem_ptl_rpt_eq, &event);
        MPIU_Assert(ret == PTL_EQ_EMPTY);

        /* check EQs for events */
        ret = MPID_nem_ptl_rptl_eqget(MPIDI_nem_ptl_eq, &event);
        MPIR_ERR_CHKANDJUMP(ret == PTL_EQ_DROPPED, mpi_errno, MPI_ERR_OTHER, "**eqdropped");
        if (ret == PTL_EQ_EMPTY) {
            ret = MPID_nem_ptl_rptl_eqget(MPIDI_nem_ptl_get_eq, &event);
            MPIR_ERR_CHKANDJUMP(ret == PTL_EQ_DROPPED, mpi_errno, MPI_ERR_OTHER, "**eqdropped");

            if (ret == PTL_EQ_EMPTY) {
                ret = MPID_nem_ptl_rptl_eqget(MPIDI_nem_ptl_control_eq, &event);
                MPIR_ERR_CHKANDJUMP(ret == PTL_EQ_DROPPED, mpi_errno, MPI_ERR_OTHER, "**eqdropped");

                if (ret == PTL_EQ_EMPTY) {
                    ret = MPID_nem_ptl_rptl_eqget(MPIDI_nem_ptl_origin_eq, &event);
                    MPIR_ERR_CHKANDJUMP(ret == PTL_EQ_DROPPED, mpi_errno, MPI_ERR_OTHER, "**eqdropped");
                } else {
                    ctl_event = TRUE;
                }

                /* all EQs are empty */
                if (ret == PTL_EQ_EMPTY)
                    break;
            }
        }
        MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptleqget", "**ptleqget %s", MPID_nem_ptl_strerror(ret));
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Received event %s pt_idx=%d ni_fail=%s list=%s user_ptr=%p hdr_data=%#lx mlength=%lu rlength=%lu",
                                                MPID_nem_ptl_strevent(&event), event.pt_index, MPID_nem_ptl_strnifail(event.ni_fail_type),
                                                MPID_nem_ptl_strlist(event.ptl_list), event.user_ptr, event.hdr_data, event.mlength, event.rlength));
        MPIR_ERR_CHKANDJUMP2(event.ni_fail_type != PTL_NI_OK && event.ni_fail_type != PTL_NI_NO_MATCH, mpi_errno, MPI_ERR_OTHER, "**ptlni_fail", "**ptlni_fail %s %s", MPID_nem_ptl_strevent(&event), MPID_nem_ptl_strnifail(event.ni_fail_type));

        /* special case for events on the control portal */
        if (ctl_event) {
            mpi_errno = MPID_nem_ptl_nm_ctl_event_handler(&event);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            continue;
        }

        switch (event.type) {
        case PTL_EVENT_PUT:
            if (event.ptl_list == PTL_OVERFLOW_LIST)
                break;
        case PTL_EVENT_PUT_OVERFLOW:
        case PTL_EVENT_GET:
        case PTL_EVENT_SEND:
        case PTL_EVENT_REPLY:
        case PTL_EVENT_SEARCH: {
            MPID_Request * const req = event.user_ptr;
            MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "req = %p", req);
            MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "REQ_PTL(req)->event_handler = %p", REQ_PTL(req)->event_handler);
            if (REQ_PTL(req)->event_handler) {
                mpi_errno = REQ_PTL(req)->event_handler(&event);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            break;
        }
        case PTL_EVENT_AUTO_FREE:
            mpi_errno = append_overflow((size_t)event.user_ptr);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            break;
        case PTL_EVENT_AUTO_UNLINK:
            overflow_me_handle[(size_t)event.user_ptr] = PTL_INVALID_HANDLE;
            break;
        case PTL_EVENT_LINK:
            /* ignore */
            break;
        case PTL_EVENT_ACK:
        default:
            MPL_error_printf("Received unexpected event type: %d %s", event.type, MPID_nem_ptl_strevent(&event));
            MPIR_ERR_INTERNALANDJUMP(mpi_errno, "Unexpected event type");
        }
    }

 fn_exit:
    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_POLL); */
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
