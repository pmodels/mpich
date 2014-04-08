/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ts=8 sts=4 sw=4 noexpandtab : */
/*
 *
 */



#include "mpid_nem_impl.h"
#include "tofu_impl.h"

/* global variables */

/* src/mpid/ch3/channels/nemesis/include/mpid_nem_nets.h */

MPID_nem_netmod_funcs_t MPIDI_nem_tofu_funcs = {
    .init		= MPID_nem_tofu_init,
    .finalize		= MPID_nem_tofu_finalize,
#ifdef	ENABLE_CHECKPOINTING
    .ckpt_precheck	= NULL,
    .ckpt_restart	= NULL,
    .ckpt_continue	= NULL,
#endif
    .poll		= MPID_nem_tofu_poll,
    .get_business_card	= MPID_nem_tofu_get_business_card,
    .connect_to_root	= MPID_nem_tofu_connect_to_root,
    .vc_init		= MPID_nem_tofu_vc_init,
    .vc_destroy		= MPID_nem_tofu_vc_destroy,
    .vc_terminate	= MPID_nem_tofu_vc_terminate,
    .anysource_iprobe	= MPID_nem_tofu_anysource_iprobe,
    .anysource_improbe	= MPID_nem_tofu_anysource_improbe,
};


static MPIDI_Comm_ops_t comm_ops = {
    .recv_posted = MPID_nem_tofu_recv_posted,
    .send = MPID_nem_tofu_isend, /* wait is performed separately after calling this */
    .rsend = NULL,
    .ssend = NULL,
    .isend = MPID_nem_tofu_isend,
    .irsend = NULL,
    .issend = NULL,
    
    .send_init = NULL,
    .bsend_init = NULL,
    .rsend_init = NULL,
    .ssend_init = NULL,
    .start_all = NULL,
    
    .cancel_send = NULL,
    .cancel_recv = NULL,

    .prove = NULL,
    .iprove = NULL,
    .improve = NULL
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tofu_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tofu_init (MPIDI_PG_t *pg_p, int pg_rank,
		  char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TOFU_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TOFU_INIT);

    rc = LLC_init();
    MPIU_ERR_CHKANDJUMP(rc != 0, mpi_errno, MPI_ERR_OTHER, "**fail");

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TOFU_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tofu_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tofu_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TOFU_GET_BUSINESS_CARD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TOFU_GET_BUSINESS_CARD);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TOFU_GET_BUSINESS_CARD);
    return mpi_errno;
    //fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tofu_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tofu_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TOFU_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TOFU_CONNECT_TO_ROOT);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TOFU_CONNECT_TO_ROOT);
    return mpi_errno;
    //fn_fail:
    goto fn_exit;
}

/* ============================================== */
/* ================ tofu_probe.c ================ */
/* ============================================== */

#undef FUNCNAME
#define FUNCNAME MPID_nem_tofu_anysource_iprobe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_tofu_anysource_iprobe(int tag, MPID_Comm *comm, int context_offset, int *flag, MPI_Status *status)
{
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_tofu_anysource_improbe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_tofu_anysource_improbe(int tag, MPID_Comm *comm, int context_offset, int *flag, MPID_Request **message, MPI_Status *status)
{
    return MPI_SUCCESS;
}
