/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "nd_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Nem_tcp_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_FINALIZE);

    /* Finalize the ND state machine */
    MPID_Nem_nd_sm_finalize();
    /* Finalize the executive */
    MPIU_ExFinalize();

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_FINALIZE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_tcp_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_ckpt_shutdown()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_CKPT_SHUTDOWN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_CKPT_SHUTDOWN);

    mpi_errno = MPID_Nem_nd_finalize();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_CKPT_SHUTDOWN);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_VC_DESTROY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_VC_DESTROY);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_VC_DESTROY);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_vc_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_vc_terminate (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_VC_TERMINATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_VC_TERMINATE);

    /* Poll till no more pending/posted sends */
    while(!MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_EMPTY(vc)
        || !MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_EMPTY(vc)){
        mpi_errno = MPID_Nem_nd_sm_poll(1);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    vc_ch->next = NULL;
    vc_ch->prev = NULL;
    MPID_NEM_ND_VCCH_NETMOD_STATE_SET(vc, MPID_NEM_ND_VC_STATE_DISCONNECTED);

    mpi_errno = MPID_Nem_nd_conn_disc(MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPID_NEM_ND_VCCH_NETMOD_CONN_HND_SET(vc, MPID_NEM_ND_CONN_HND_INVALID);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_VC_TERMINATE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}
