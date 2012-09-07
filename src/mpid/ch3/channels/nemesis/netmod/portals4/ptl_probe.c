/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


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
int MPID_nem_ptl_iprobe(MPIDI_VC_t *vc,  int source, int tag, MPID_Comm *comm, int context_offset, int *flag, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_IPROBE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_IPROBE);

    me.ct_handle = PTL_CT_NONE;
    me.uid = PTL_UID_ANY;
    me.options = ( PTL_ME_OP_PUT | PTL_ME_IS_ACCESSIBLE | PTL_ME_EVENT_LINK_DISABLE |
                   PTL_ME_EVENT_UNLINK_DISABLE | PTL_ME_USE_ONCE );

    if (source == MPI_ANY_SOURCE)
        me.match_id = PTL_ID_ANY;
    else
        me.match_id = vc_ptl->id;
    
    me.match_bits = NPTL_MATCH(tag, context_id);

    if (tag == MPI_ANY_TAG)
        me.ignore_bits = NPTL_ANY_TAG_MASK;
    else
        me.ignore_bits = 0;
    me.min_free = 0;


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
int MPID_nem_ptl_improbe(MPIDI_VC_t *vc,  int source, int tag, MPID_Comm *comm, int context_offset, int *flag,
                         MPID_Request **message, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    ptl_me_t me;
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

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_cancel_recv
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_cancel_recv(MPIDI_VC_t *vc,  MPID_Request_t *rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_CANCEL_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_CANCEL_RECV);


 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_CANCEL_RECV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
