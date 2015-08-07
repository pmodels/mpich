/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpichconf.h"
#include "mpidi_ch3_impl.h"

#include "mpidu_sock.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

/*  MPIDI_CH3U_Init_sock - does socket specific channel initialization
 *     publish_bc_p - if non-NULL, will be a pointer to the original position 
 *                    of the bc_val and should
 *                    do KVS Put/Commit/Barrier on business card before 
 *                    returning
 *     bc_key_p     - business card key buffer pointer.  
 *     bc_val_p     - business card value buffer pointer, updated to the next
 *                    available location or freed if published.
 *     val_max_sz_p - ptr to maximum value buffer size reduced by the number 
 *                    of characters written
 *                               
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Init_sock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Init_sock(int has_parent, MPIDI_PG_t *pg_p, int pg_rank,
			 char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pg_size;
    int p;

    /* FIXME: Why are these unused? */
    MPIU_UNREFERENCED_ARG(has_parent);
    MPIU_UNREFERENCED_ARG(pg_rank);

    /*
     * Initialize the VCs associated with this process group (and thus 
     * MPI_COMM_WORLD)
     */

    /* FIXME: Get the size from the process group */
    pg_size = MPIDI_PG_Get_size(pg_p);

    /* FIXME: This should probably be the same as MPIDI_VC_InitSock.  If
       not, why not? */
    /* FIXME: Note that MPIDI_CH3_VC_Init sets state, sendq_head and tail.
       so this should be MPIDI_CH3_VC_Init( &pg_p->vct[p] );
       followed by MPIDI_VC_InitSock( ditto );  
       In fact, there should be a single VC_Init call here */
    /* FIXME: Why isn't this MPIDI_VC_Init( vc, NULL, 0 )? */
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3I_VC *vcch = &pg_p->vct[p].ch;
	vcch->sendq_head = NULL;
	vcch->sendq_tail = NULL;
	vcch->state      = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	vcch->sock       = MPIDU_SOCK_INVALID_SOCK;
	vcch->conn       = NULL;
    }    

    mpi_errno = MPIDI_CH3U_Get_business_card_sock(pg_rank, 
						  bc_val_p, val_max_sz_p);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_buscard");
    }

 fn_exit:
    
    return mpi_errno;
    
 fn_fail:
    /* FIXME: This doesn't belong here, since the pg is not created in 
       this routine */
    /* --BEGIN ERROR HANDLING-- */
    if (pg_p != NULL) 
    {
	MPIDI_PG_Destroy(pg_p);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* This routine initializes Sock-specific elements of the VC */
int MPIDI_VC_InitSock( MPIDI_VC_t *vc ) 
{
    MPIDI_CH3I_VC *vcch = &vc->ch;
    vcch->sock               = MPIDU_SOCK_INVALID_SOCK;
    vcch->conn               = NULL;
    return 0;
}

#ifdef USE_DBG_LOGGING
const char * MPIDI_Conn_GetStateString(int state) 
{
    const char *name = "unknown";

    switch (state) {
    case CONN_STATE_UNCONNECTED:     name = "CONN_STATE_UNCONNECTED"; break;
    case CONN_STATE_LISTENING:       name = "CONN_STATE_LISTENING"; break;
    case CONN_STATE_CONNECTING:      name = "CONN_STATE_CONNECTING"; break;
    case CONN_STATE_CONNECT_ACCEPT:  name = "CONN_STATE_CONNECT_ACCEPT"; break; 
    case CONN_STATE_OPEN_CSEND:      name = "CONN_STATE_OPEN_CSEND"; break;
    case CONN_STATE_OPEN_CRECV:      name = "CONN_STATE_OPEN_CRECV"; break;
    case CONN_STATE_OPEN_LRECV_PKT:  name = "CONN_STATE_OPEN_LRECV_PKT"; break;
    case CONN_STATE_OPEN_LRECV_DATA: name = "CONN_STATE_OPEN_LRECV_DATA"; break;
    case CONN_STATE_OPEN_LSEND:      name = "CONN_STATE_OPEN_LSEND"; break;
    case CONN_STATE_CONNECTED:       name = "CONN_STATE_CONNECTED"; break;
    case CONN_STATE_CLOSING:         name = "CONN_STATE_CLOSING"; break;
    case CONN_STATE_CLOSED:          name = "CONN_STATE_CLOSED"; break;
    case CONN_STATE_DISCARD:         name = "CONN_STATE_DISCARD"; break;
    case CONN_STATE_FAILED:          name = "CONN_STATE_FAILE"; break;
    }

    return name;
}

#endif
