/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/* FIXME: Packet types used for establishing connections */
typedef struct
{
    MPIDI_CH3_Pkt_type_t type;
    int port_name_tag;
}
MPIDI_CH3I_Pkt_sc_conn_accept_t;

/* FIXME: What does this routine do? */
/* FIXME: It appears that this routine is used as part of the 
   initial "get the peers connected" code in comm_connect/accept and
   spawn */

#undef FUNCNAME
#define FUNCNAME  MPIDI_CH3I_Connect_to_root_sshm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Connect_to_root_sshm(const char * port_name, 
				    MPIDI_VC_t ** new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIDI_CH3I_VC *vcch;
    MPIU_CHKPMEM_DECL(1);
    int port_name_tag;
    int connected;
    MPIDI_CH3_Pkt_t pkt;
    MPIDI_CH3I_Pkt_sc_conn_accept_t *acceptpkt = 
	    (MPIDI_CH3I_Pkt_sc_conn_accept_t *)&pkt;
    int num_written;
    char *cached_pg_id, *dummy_id = "";
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT_SSHM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT_SSHM);

    mpi_errno = MPIDI_GetTagFromPort(port_name, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_port_name_tag");
    }
    MPIU_DBG_MSG_D(CH3_CONNECT,VERBOSE,"port tag %d",port_name_tag);

    if (*new_vc != NULL)
    {
	MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"using old connection");
	vc = *new_vc;
    }
    else
    {
	MPIU_CHKPMEM_MALLOC(vc,MPIDI_VC_t *,sizeof(MPIDI_VC_t),mpi_errno,"vc");
	/* FIXME - where does this vc get freed? */

	*new_vc = vc;

	MPIDI_VC_Init(vc, NULL, 0);
    }
    vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    vcch->state = MPIDI_CH3I_VC_STATE_CONNECTING;

    connected = FALSE;
    /* Make it so that the pg_id is not matched on the other side by sending a 
       dummy value */
    cached_pg_id = MPIDI_Process.my_pg->id;
    MPIDI_Process.my_pg->id = dummy_id;
    mpi_errno = MPIDI_CH3I_Shm_connect(vc, port_name, &connected);
    MPIDI_Process.my_pg->id = cached_pg_id;
    if (mpi_errno != MPI_SUCCESS) {
	/* FIXME: Do we want to free the vc instead? Or put this into the fail
	   block? */
	vcch->state = MPIDI_CH3I_VC_STATE_FAILED;
	MPIU_ERR_POP(mpi_errno);
    }
    if (!connected) {
	/* FIXME: Non conforming (not internationalizable) error message.
	   why not just MPIU_ERR_POP? */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to establish a shared memory queue connection");
	goto fn_fail;
    }

    MPIDI_CH3I_SHM_Add_to_writer_list(vc);
    vcch->state = MPIDI_CH3I_VC_STATE_CONNECTED;
    vcch->shm_reading_pkt = TRUE;
    vcch->send_active = MPIDI_CH3I_SendQ_head(vcch); /* MT */

    MPIDI_Pkt_init(acceptpkt, MPIDI_CH3I_PKT_SC_CONN_ACCEPT);
    acceptpkt->port_name_tag = port_name_tag;

    mpi_errno = MPIDI_CH3I_SHM_write(vc, acceptpkt, sizeof(MPIDI_CH3_Pkt_t), 
				     &num_written);
    if (mpi_errno != MPI_SUCCESS || num_written != sizeof(MPIDI_CH3_Pkt_t)) {
	MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT_SSHM);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#ifdef USE_DBG_LOGGING
const char * MPIDI_CH3_VC_SshmGetStateString( struct MPIDI_VC *vc )
{
    const char *name = "unknown";
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    static char asdigits[20];
    int    state = vcch->state;
    
    switch (state) {
    case MPIDI_CH3I_VC_STATE_UNCONNECTED: name = "CH3I_VC_STATE_UNCONNECTED"; break;
    case MPIDI_CH3I_VC_STATE_CONNECTING:  name = "CH3I_VC_STATE_CONNECTING"; break;
    case MPIDI_CH3I_VC_STATE_CONNECTED:   name = "CH3I_VC_STATE_CONNECTED"; break;
    case MPIDI_CH3I_VC_STATE_FAILED:      name = "CH3I_VC_STATE_FAILED"; break;
    default:
	MPIU_Snprintf( asdigits, sizeof(asdigits), "%d", state );
	asdigits[20-1] = 0;
	name = (const char *)asdigits;
    }
    return name;
}
#endif
