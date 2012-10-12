/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"

#undef FUNCNAME
#define FUNCNAME handle_probe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int handle_probe(const ptl_event_t *e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *const req = e->user_ptr;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_PROBE);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_PROBE);

    if (e->ni_fail_type == PTL_NI_NO_MATCH) {
        REQ_PTL(req)->found = FALSE;
        goto fn_exit;
    }

    REQ_PTL(req)->found = TRUE;
    req->status.MPI_SOURCE = NPTL_MATCH_GET_RANK(e->match_bits);
    req->status.MPI_TAG = NPTL_MATCH_GET_TAG(e->match_bits);
    req->status.count = NPTL_HEADER_GET_LENGTH(e->match_bits);

    MPIDI_CH3U_Request_complete(req);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_PROBE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_probe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_probe(MPIDI_VC_t *vc, int source, int tag, MPID_Comm *comm, int context_offset, MPI_Status *status)
{
    MPIU_Assertp(0 && "This function shouldn't be called.");
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_iprobe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_iprobe(MPIDI_VC_t *vc, int source, int tag, MPID_Comm *comm, int context_offset, int *flag, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ptl_vc_area *const vc_ptl = VC_PTL(vc);
    int ret;
    ptl_process_t id_any;
    ptl_me_t me;
    MPID_Request *req;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_IPROBE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_IPROBE);

    id_any.phys.nid = PTL_NID_ANY;
    id_any.phys.pid = PTL_PID_ANY;
    
    /* create a request */
    req = MPID_Request_create();
    MPIU_ERR_CHKANDJUMP1(!req, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Request_create");
    MPIU_Object_set_ref(req, 2); /* 1 ref for progress engine and 1 ref for us */
    REQ_PTL(req)->event_handler = handle_probe;

    /* create a dummy ME to use for searching the list */
    me.start = NULL;
    me.length = 0;
    me.ct_handle = PTL_CT_NONE;
    me.uid = PTL_UID_ANY;
    me.options = ( PTL_ME_OP_PUT | PTL_ME_USE_ONCE );
    me.min_free = 0;
    me.match_bits = NPTL_MATCH(tag, comm->context_id + context_offset, source);

    if (source == MPI_ANY_SOURCE)
        me.match_id = id_any;
    else
        me.match_id = vc_ptl->id;

    if (tag == MPI_ANY_TAG)
        me.ignore_bits = NPTL_MATCH_IGNORE_ANY_TAG;
    else
        me.ignore_bits = NPTL_MATCH_IGNORE;

    /* submit a search request */
    ret = PtlMESearch(MPIDI_nem_ptl_ni, MPIDI_nem_ptl_pt, &me, PTL_SEARCH_ONLY, req);
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**ptlmesearch", "**ptlmesearch %s", MPID_nem_ptl_strerror(ret));
    DBG_MSG_MESearch("REG", vc->pg_rank, me, req);
    
    /* wait for search request to complete */
    do {
        mpi_errno = MPID_nem_ptl_poll(FALSE);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    } while (!MPID_Request_is_complete(req));

    *flag = REQ_PTL(req)->found;
    *status = req->status;
    
    MPID_Request_release(req);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_IPROBE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_improbe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_improbe(MPIDI_VC_t *vc, int source, int tag, MPID_Comm *comm, int context_offset, int *flag,
                         MPID_Request **message, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_IMPROBE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_IMPROBE);



 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_IMPROBE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_anysource_iprobe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_anysource_iprobe(int tag, MPID_Comm * comm, int context_offset, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_IPROBE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_IPROBE);


 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_IPROBE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_anysource_improbe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_anysource_improbe(int tag, MPID_Comm * comm, int context_offset, int *flag, MPID_Request **message,
                                   MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_IMPROBE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_IMPROBE);


 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_ANYSOURCE_IMPROBE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

