/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ts=8 sts=4 sw=4 noexpandtab : */
/*
 *
 */



#include "mpid_nem_impl.h"
#include "llc_impl.h"

//#define MPID_NEM_LLC_DEBUG_VC
#ifdef MPID_NEM_LLC_DEBUG_VC
#define dprintf printf
#else
#define dprintf(...)
#endif

/* function prototypes */

static int llc_vc_init(MPIDI_VC_t * vc);

static MPIDI_Comm_ops_t comm_ops = {
    .recv_posted = MPID_nem_llc_recv_posted,
    .send = MPID_nem_llc_isend, /* wait is performed separately after calling this */
    .rsend = MPID_nem_llc_isend,
    .ssend = MPID_nem_llc_issend,
    .isend = MPID_nem_llc_isend,
    .irsend = MPID_nem_llc_isend,
    .issend = MPID_nem_llc_issend,

    .send_init = NULL,
    .bsend_init = NULL,
    .rsend_init = NULL,
    .ssend_init = NULL,
    .startall = NULL,

    .cancel_send = NULL,
    .cancel_recv = MPID_nem_llc_cancel_recv,

    .probe = MPID_nem_llc_probe,
    .iprobe = MPID_nem_llc_iprobe,
    .improbe = MPID_nem_llc_improbe
};

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_vc_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_vc_init(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LLC_VC_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LLC_VC_INIT);

    mpi_errno = llc_vc_init(vc);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LLC_VC_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_vc_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_vc_destroy(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LLC_VC_DESTROY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LLC_VC_DESTROY);
    /* free any resources associated with this VC here */

    {
        MPID_nem_llc_vc_area *vc_llc = VC_LLC(vc);

        vc_llc->endpoint = 0;
    }

    /* wait until all UNSOLICITED are done */
    while (VC_FIELD(vc, unsolicited_count)) {
        MPID_nem_llc_poll(1);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LLC_VC_DESTROY);
    return mpi_errno;
    //fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_vc_terminate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_vc_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LLC_VC_TERMINATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LLC_VC_TERMINATE);

    dprintf("llc_vc_terminate,enter,%d->%d\n", MPIDI_Process.my_pg_rank, vc->pg_rank);

    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LLC_VC_TERMINATE);
    return mpi_errno;
    //fn_fail:
    goto fn_exit;
}

/* ============================================== */

#undef FUNCNAME
#define FUNCNAME llc_vc_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int llc_vc_init(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;



    {
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE,
                       "MPIDI_NEM_VC_NETMOD_AREA_LEN = %d\n", MPIDI_NEM_VC_NETMOD_AREA_LEN);
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE,
                       "MPIDI_NEM_REQ_NETMOD_AREA_LEN = %d", MPIDI_NEM_REQ_NETMOD_AREA_LEN);
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE,
                       "MPID_nem_llc_vc_area = %d\n", (int) sizeof(MPID_nem_llc_vc_area));
    }

    /* MPIDI_CH3I_VC: src/mpid/ch3/channels/nemesis/include/mpidi_ch3_pre.h */
    {
        MPIDI_CH3I_VC *vc_ch = &vc->ch;
        MPID_nem_llc_vc_area *vc_llc = VC_LLC(vc);

        vc_llc->endpoint = vc;
        mpi_errno =
            MPID_nem_llc_kvs_get_binary(vc->pg_rank,
                                        "llc_rank",
                                        (char *) &vc_llc->remote_endpoint_addr, sizeof(int));
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_kvs_get_binary");
        dprintf("llc_vc_init,my_pg_rank=%d,pg_rank=%d,my_llc_rank=%d,llc_rank=%ld\n",
                MPIDI_Process.my_pg_rank, vc->pg_rank, MPID_nem_llc_my_llc_rank,
                vc_llc->remote_endpoint_addr);

        vc_llc->send_queue.head = 0;    /* GENERIC_Q_DECL */
        vc_llc->send_queue.tail = 0;    /* GENERIC_Q_DECL */

        vc->eager_max_msg_sz = (12 * 1024);
        vc->ready_eager_max_msg_sz = (12 * 1024);
        /* vc->rndvSend_fn = 0 */
        /* vc->rndvRecv_fn = 0 */
        vc->sendNoncontig_fn = MPID_nem_llc_SendNoncontig;
#ifdef	ENABLE_COMM_OVERRIDES
        vc->comm_ops = &comm_ops;
#endif

        vc_ch->iStartContigMsg = MPID_nem_llc_iStartContigMsg;
        vc_ch->iSendContig = MPID_nem_llc_iSendContig;

#ifdef	ENABLE_CHECKPOINTING
        vc_ch->ckpt_pause_send_vc = 0 /* MPID_nem_llc_ckpt_pause_send_vc */ ;
        vc_ch->ckpt_continue_vc = 0 /* MPID_nem_llc_ckpt_continue_vc */ ;
        vc_ch->ckpt_restart_vc = 0 /* = MPID_nem_llc_ckpt_restart_vc */ ;
#endif

        MPIDI_CHANGE_VC_STATE(vc, ACTIVE);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#if 0   /* not use */
#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_vc_prnt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPID_nem_llc_vc_prnt(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;

    /* MPIU_OBJECT_HEADER; */
    /* src/include/mpihandlemem.h */
    /* int handle; */
    /* MPIU_Handle_ref_count ref_count; */
    /* MPIDI_VC_State_t state; */
    /* struct MPIDI_PG *pg; */
    /* int pg_rank; */
    /* int lpid; */
    /* MPID_Node_id_t node_id; */
    /* int port_name_tag; */
    /* MPID_Seqnum_t seqnum_send; *//* MPID_USE_SEQUENCE_NUMBERS */
    /* MPID_Seqnum_t seqnum_recv; *//* MPID_CH3_MSGS_UNORDERED */
    /* MPIDI_CH3_Pkt_send_container_t *msg_reorder_queue; */
    /* int (*rndvSend_fn)(); */
    /* int (*rndvRecv_fn)(); */
    /* int eager_max_msg_sz; */
    /* int ready_eager_max_msg_sz; */
    /* int (*sendNonconfig_gn)(); */
    /* MPIDI_Comm_ops_t *comm_ops; *//* ENABLE_COMM_OVERRIDES */
    /* MPIDI_CH3_VC_DECL *//* MPIDI_CH3_VC_DECL */
    /* src/mpid/ch3/channels/nemesis/include/mpidi_ch3_pre.h */

    return mpi_errno;
}
#endif
