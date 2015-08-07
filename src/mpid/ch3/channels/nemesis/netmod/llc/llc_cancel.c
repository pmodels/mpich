/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ts=8 sts=4 sw=4 noexpandtab : */
/*
 *
 */



#include "mpid_nem_impl.h"
#include "llc_impl.h"

//#define MPID_NEM_LLC_DEBUG_PROBE
#ifdef MPID_NEM_LLC_DEBUG_PROBE
#define dprintf printf
#else
#define dprintf(...)
#endif

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_cancel_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* returns zero in case request is canceled */
int MPID_nem_llc_cancel_recv(struct MPIDI_VC *vc, struct MPID_Request *req)
{
    int canceled;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LLC_CANCEL_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LLC_CANCEL_RECV);

    /* returns zero in case request is canceled */
    canceled = LLC_req_approve_recv((LLC_cmd_t *) REQ_FIELD(req, cmds));

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LLC_CANCEL_RECV);
    return canceled;
  fn_fail:
    goto fn_exit;
}
