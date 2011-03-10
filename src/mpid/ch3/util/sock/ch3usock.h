/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CH3USOCK_H_INCLUDED
#define CH3USOCK_H_INCLUDED

#ifndef MPIDU_SOCK_H_INCLUDED
#include "mpidu_sock.h" 
#endif

typedef enum MPIDI_CH3I_Conn_state
{
    CONN_STATE_UNCONNECTED,
    CONN_STATE_LISTENING,
    CONN_STATE_CONNECTING,
    CONN_STATE_CONNECT_ACCEPT, 
    CONN_STATE_OPEN_CSEND,
    CONN_STATE_OPEN_CRECV,
    CONN_STATE_OPEN_LRECV_PKT,
    CONN_STATE_OPEN_LRECV_DATA,
    CONN_STATE_OPEN_LSEND,
    CONN_STATE_CONNECTED,
    CONN_STATE_CLOSING,
    CONN_STATE_CLOSED,
    CONN_STATE_FAILED
} MPIDI_CH3I_Conn_state;

typedef struct MPIDI_CH3I_Connection
{
    MPIDI_VC_t * vc;
    MPIDU_Sock_t sock;
    MPIDI_CH3I_Conn_state state;
    struct MPID_Request * send_active;
    struct MPID_Request * recv_active;
    MPIDI_CH3_Pkt_t pkt;
    char * pg_id;
    MPID_IOV iov[2];
} MPIDI_CH3I_Connection_t;


/* These implement the connection state machine for socket connections */
int MPIDI_CH3_Sockconn_handle_accept_event( void );
int MPIDI_CH3_Sockconn_handle_connect_event( MPIDI_CH3I_Connection_t *, int );
int MPIDI_CH3_Sockconn_handle_close_event( MPIDI_CH3I_Connection_t * );
int MPIDI_CH3_Sockconn_handle_conn_event( MPIDI_CH3I_Connection_t * );
int MPIDI_CH3_Sockconn_handle_connopen_event( MPIDI_CH3I_Connection_t * );
int MPIDI_CH3_Sockconn_handle_connwrite( MPIDI_CH3I_Connection_t * );

/* Start the process of creating a socket connection */
int MPIDI_CH3I_Sock_connect( MPIDI_VC_t *, const char[], int );

/* Create/free a new socket connection */
int MPIDI_CH3I_Connection_alloc(MPIDI_CH3I_Connection_t **);
void MPIDI_CH3I_Connection_free(MPIDI_CH3I_Connection_t *);

/* Routines to get the socket address */
int MPIDU_CH3U_GetSockInterfaceAddr( int, char *, int, MPIDU_Sock_ifaddr_t * );

/* Return a string for the connection state */
#ifdef USE_DBG_LOGGING
const char * MPIDI_Conn_GetStateString(int);
const char * MPIDI_CH3_VC_GetStateString( struct MPIDI_VC * );
#endif

int MPIDU_Sock_get_conninfo_from_bc( const char *bc, 
				     char *host_description, int maxlen,
				     int *port, MPIDU_Sock_ifaddr_t *ifaddr, 
				     int *hasIfaddr );

/* These two routines from util/sock initialize and shutdown the 
   socket used to establish connections.  */
int MPIDU_CH3I_SetupListener( MPIDU_Sock_set_t );
int MPIDU_CH3I_ShutdownListener( void );

/* macro to access the channel_private fields in the vc */
#define VC_CH(vc) ((MPIDI_CH3I_VC *)(vc)->channel_private)

#endif
