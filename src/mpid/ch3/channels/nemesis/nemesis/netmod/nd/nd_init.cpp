/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "nd_impl.h"

/* FIXME: Remove these - we bypass the Nemesis proc/module queues
MPID_nem_queue_ptr_t MPID_Nem_nd_free_queue = 0;
MPID_nem_queue_ptr_t MPID_Nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_Nem_process_free_queue = 0;

static MPID_nem_queue_t _free_queue;
*/

/* The Nemesis netmod functions are used by the upper
 * layer - written in C
 */
extern "C" {
MPID_nem_netmod_funcs_t MPIDI_Nem_nd_funcs = {
    MPID_Nem_nd_init,
    MPID_Nem_nd_finalize,
    MPID_Nem_nd_poll,
    MPID_Nem_nd_get_business_card,
    MPID_Nem_nd_connect_to_root,
    MPID_Nem_nd_vc_init,
    MPID_Nem_nd_vc_destroy,
    MPID_Nem_nd_vc_terminate
};
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_init(MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_INIT);

    /* Initialize Executive */
    mpi_errno = MPIU_ExInitialize();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Initialize ND state machine */
    mpi_errno = MPID_Nem_nd_sm_init();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Listen for conns & create the business card */
    mpi_errno = MPID_Nem_nd_listen_for_conn(pg_rank, bc_val_p, val_max_sz_p);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_INIT);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_vc_init(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_VC_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_VC_INIT);

    vc->sendNoncontig_fn      = MPID_Nem_nd_send_noncontig;

    MPID_NEM_ND_VCCH_NETMOD_STATE_SET(vc, MPID_NEM_ND_VC_STATE_DISCONNECTED);
    vc_ch->iStartContigMsg    = MPID_Nem_nd_istart_contig_msg;
    vc_ch->iSendContig        = MPID_Nem_nd_send_contig;
    vc_ch->next = NULL;
    vc_ch->prev = NULL;

    MPID_NEM_ND_VCCH_NETMOD_CONN_HND_INIT(vc);
    MPID_NEM_ND_VCCH_NETMOD_TMP_CONN_HND_INIT(vc);
    MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_INIT(vc);
    MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_INIT(vc);
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_VC_INIT);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}
