/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"

#define NUMBUFS 20
#define BUFLEN  (sizeof(MPIDI_CH3_Pkt_t) + PTL_MAX_EAGER)

static char recvbuf[BUFLEN][NUMBUFS];
static ptl_le_t recvbuf_le[NUMBUFS];
static ptl_handle_le_t recvbuf_le_handle[NUMBUFS];

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_poll_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_poll_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_POLL_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_POLL_INIT);

    for (i = 0; i < NUMBUFS; ++i) {
        recvbuf_le[i].start = recvbuf[i];
        recvbuf_le[i].length = BUFLEN;
        recvbuf_le[i].ct_handle = PTL_CT_NONE;
        recvbuf_le[i].uid = PTL_UID_ANY;
        recvbuf_le[i].options = (PTL_LE_OP_PUT | PTL_LE_USE_ONCE |
                                 PTL_LE_EVENT_UNLINK_DISABLE | PTL_LE_EVENT_LINK_DISABLE);
        ret = PtlLEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &recvbuf_le[i], PTL_PRIORITY_LIST, (void *)(uint64_t)i,
                          &recvbuf_le_handle[i]);
        MPIU_ERR_CHKANDJUMP(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend");
    }


 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_POLL_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_poll_finalize
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_poll_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_POLL_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_POLL_FINALIZE);
    
    for (i = 0; i < NUMBUFS; ++i) {
        ret = PtlLEUnlink(recvbuf_le_handle[i]);
        MPIU_ERR_CHKANDJUMP(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeunlink");
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_POLL_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_poll
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_poll(int is_blocking_poll)
{
    int mpi_errno = MPI_SUCCESS;
    ptl_event_t event;
    int ret;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_POLL);

    /* MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_POLL); */

    while (1) {
        ret = PtlEQGet(MPIDI_nem_ptl_eq, &event);
        if (ret == PTL_EQ_EMPTY)
            break;
        MPIU_ERR_CHKANDJUMP(ret, mpi_errno, MPI_ERR_OTHER, "**ptleqget");
        switch (event.type) {
            MPID_Request * const req = e->user_ptr;
        case PTL_EVENT_PUT:
        case PTL_EVENT_GET:
        case PTL_EVENT_ACK:
        case PTL_EVENT_REPLY:
        case PTL_EVENT_SEARCH:
            mpi_errno = REQ_PTL(sreq)->event_handler(e);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            break;
        case PTL_EVENT_PUT_OVERFLOW:
            MPIU_ERR_INTERNALANDJUMP(mpi_errno, "Overflow event");
            break
        default:
            MPIU_ERR_INTERNALANDJUMP(mpi_errno, "Unexpected event type");
        }
    }
    
    
        
        

#if 0 /* used for non-matching message passing */
        switch (event.type) {
        case PTL_EVENT_PUT:
            if (event.ni_fail_type) {
                /* FIXME: handle comm failures */
                printf("Message received with error (%d) from process %lu\n", event.ni_fail_type, (uint64_t)event.hdr_data);
                assert(0);
            }
            /* FIXME: doesn't handle dynamic connections */
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Put received(size=%lu id=(%#x,%#x) pt=%#x)", event.rlength,
                                                    event.initiator.phys.nid, event.initiator.phys.pid, event.pt_index));
            MPIDI_PG_Get_vc_set_active(MPIDI_Process.my_pg, (uint64_t)event.hdr_data, &vc);
            mpi_errno = MPID_nem_handle_pkt(vc, event.start, event.rlength);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            assert(event.start == recvbuf[(uint64_t)event.user_ptr]);
            ret = PtlLEAppend(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_control_pt, &recvbuf_le[(uint64_t)event.user_ptr],
                              PTL_PRIORITY_LIST, event.user_ptr, &recvbuf_le_handle[(uint64_t)event.user_ptr]);
            MPIU_ERR_CHKANDJUMP(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmeappend");
            break;
        case PTL_EVENT_SEND:
            if (event.ni_fail_type) {
                /* FIXME: handle comm failures */
                printf("Message send completed with error (%d)\n", event.ni_fail_type);
                assert(0);
            }
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Send completed");
            mpi_errno = MPID_nem_ptl_ev_send_handler(&event);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            break;
        case PTL_EVENT_ACK:
            if (event.ni_fail_type) {
                /* FIXME: handle comm failures */
                printf("ACK received with error (%d) sb=%p\n", event.ni_fail_type, event.user_ptr);
                assert(0);
            }
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "ACK received sb=%p", event.user_ptr));
            break;
        default:
            /* FIXME: figure out what other events to expect */
            printf("Got unexpected event %d\n", event.type);
            break;
        }
    }
#endif


 fn_exit:
    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_POLL); */
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
