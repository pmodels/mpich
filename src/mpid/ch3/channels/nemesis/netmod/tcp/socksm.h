/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SOCKSM_H
#define SOCKSM_H

#include <sys/poll.h>
#include <stddef.h> 

enum SOCK_CONSTS {  /* more type safe than #define's */
    LISTENQLEN = 10,
    POLL_CONN_TIME = 2
};

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

enum CONSTS {
    CONN_PLFD_TBL_INIT_SIZE = 20,
    CONN_PLFD_TBL_GROW_SIZE = 10,
    CONN_INVALID_FD = -1,
    CONN_INVALID_RANK = -1,
    NEGOMSG_DATA_LEN = 4, /* Length of data during negotiation message exchanges. */
    SLEEP_INTERVAL = 1500,
    PROGSM_TIMES = 20
};

typedef enum {
    MPID_NEM_TCP_SOCK_ERROR_EOF, /* either a socket error or EOF received from peer */
    MPID_NEM_TCP_SOCK_CONNECTED,
    MPID_NEM_TCP_SOCK_NOEVENT /*  No poll event on socket */
}MPID_NEM_TCP_SOCK_STATUS_t;

#define M_(x) x
#define CONN_TYPE_ M_(TYPE_CONN), M_(TYPE_ACPT)

/*  Note :  '_' denotes sub-states */
/*  For example, CSO_DISCCONNECTING_DISCREQSENT, CSO_DISCONNECTING_DISCRSPRCVD are sub-states */
/*  of CSO_DISCONNECTING */
/*  LSO - Listening SOcket states */
#define LISTEN_STATE_                           \
    M_(LISTEN_STATE_CLOSED),                    \
    M_(LISTEN_STATE_LISTENING)

/*
  CONN_STATE - Connection states of socket
  TC = Type connected (state of a socket that was issued a connect on)
  TA = Type Accepted (state of a socket returned by accept)
  TS = Type Shared (state of either TC or TA)

  C - Connection sub-states
  D - Disconnection sub-states
*/

#define CONN_STATE_                             \
    M_(CONN_STATE_UNDEFINED),                   \
    M_(CONN_STATE_TS_CLOSED),                   \
    M_(CONN_STATE_TC_C_CNTING),                 \
    M_(CONN_STATE_TC_C_CNTD),                   \
    M_(CONN_STATE_TC_C_RANKSENT),               \
    M_(CONN_STATE_TC_C_TMPVCSENT),              \
    M_(CONN_STATE_TA_C_CNTD),                   \
    M_(CONN_STATE_TA_C_RANKRCVD),               \
    M_(CONN_STATE_TA_C_TMPVCRCVD),              \
    M_(CONN_STATE_TS_COMMRDY)

/* REQ - Request, RSP - Response */

typedef enum CONN_TYPE {CONN_TYPE_, CONN_TYPE_SIZE} Conn_type_t;
typedef enum MPID_nem_tcp_Listen_State {LISTEN_STATE_INVALID = -1, LISTEN_STATE_, LISTEN_STATE_SIZE}
    MPID_nem_tcp_Listen_State_t ;

typedef enum MPID_nem_tcp_Conn_State {CONN_STATE_INVALID = -1, CONN_STATE_, CONN_STATE_SIZE}
    MPID_nem_tcp_Conn_State_t;

/*
  Note: event numbering starts from 1, as 0 is assumed to be the state of all-events cleared
 */
typedef enum sockconn_event {EVENT_CONNECT = 1, EVENT_DISCONNECT}
    sockconn_event_t;

#undef M_
#define M_(x) #x

#if defined(SOCKSM_H_DEFGLOBALS_)
const char *const CONN_TYPE_STR[CONN_TYPE_SIZE] = {CONN_TYPE_};
const char *const LISTEN_STATE_STR[LISTEN_STATE_SIZE] = {LISTEN_STATE_};
const char *const CONN_STATE_STR[CONN_STATE_SIZE] = {CONN_STATE_};
#elif defined(SOCKSM_H_EXTERNS_)
extern const char *const CONN_TYPE_STR[];
extern const char *const LISTEN_STATE_STR[];
extern const char *const CONN_STATE_STR[];
#endif
#undef M_

#ifdef USE_DBG_LOGGING
#define CONN_STATE_TO_STRING(_cstate) \
    (( (_cstate) >= CONN_STATE_TS_CLOSED && (_cstate) < CONN_STATE_SIZE ) ? CONN_STATE_STR[_cstate] : "out_of_range")

#define DBG_CHANGE_STATE(_sc, _cstate) do { \
    const char *state_str = NULL; \
    const char *old_state_str = NULL; \
    if (MPIU_DBG_SELECTED(NEM_SOCK_DET,VERBOSE)) { \
        if ((_sc)) { \
            old_state_str = CONN_STATE_TO_STRING((_sc)->state.cstate); \
            state_str     = CONN_STATE_TO_STRING(_cstate); \
        } \
        MPIU_DBG_OUT_FMT(NEM_SOCK_DET, (MPIU_DBG_FDEST, "CHANGE_STATE(_sc=%p, _cstate=%d (%s)) - old_state=%s sc->vc=%p", _sc, _cstate, state_str, old_state_str, (_sc)->vc)); \
    } \
} while (0)
#else
#  define CONN_STATE_TO_STRING(_cstate) "unavailable"
#  define DBG_CHANGE_STATE(_sc, _cstate) do { /*nothing*/ } while (0)
#endif

#define CHANGE_STATE(_sc, _cstate) do { \
    DBG_CHANGE_STATE(_sc, _cstate); \
    (_sc)->state.cstate = (_cstate); \
    (_sc)->handler = sc_state_info[_cstate].sc_state_handler; \
    MPID_nem_tcp_plfd_tbl[(_sc)->index].events = sc_state_info[_cstate].sc_state_plfd_events; \
} while(0)

struct MPID_nem_new_tcp_sockconn;
typedef struct MPID_nem_new_tcp_sockconn sockconn_t;

/* FIXME: should plfd really be const const?  Some handlers may need to change the plfd entry. */
typedef int (*handler_func_t) (struct pollfd *const plfd, sockconn_t *const conn);
struct MPID_nem_new_tcp_sockconn{
    int fd;
    int index;

    /* Used to prevent the usage of uninitialized pg info.  is_same_pg, pg_rank,
     * and pg_id are _ONLY VALID_ if this (pg_is_set) is true */
    int pg_is_set;   
    int is_same_pg; /* boolean, true if the process on the other end of this sc
                       is in the same PG as us (this process) */
    int is_tmpvc;

    int pg_rank; /*  rank and id cached here to avoid chasing pointers in vc and vc->pg */
    char *pg_id; /*  MUST be used only if is_same_pg == FALSE */
    union {
        MPID_nem_tcp_Conn_State_t cstate;
        MPID_nem_tcp_Listen_State_t lstate; 
    }state;
    MPIDI_VC_t *vc;
    /* Conn_type_t conn_type;  Probably useful for debugging/analyzing purposes. */
    handler_func_t handler;
};

typedef enum MPIDI_nem_tcp_socksm_pkt_type {
    MPIDI_NEM_TCP_SOCKSM_PKT_ID_INFO, /*  ID = rank + pg_id */
    MPIDI_NEM_TCP_SOCKSM_PKT_ID_ACK,
    MPIDI_NEM_TCP_SOCKSM_PKT_ID_NAK,
    MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_INFO,
    MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_ACK,
    MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_NAK,
    MPIDI_NEM_TCP_SOCKSM_PKT_CLOSED
} MPIDI_nem_tcp_socksm_pkt_type_t;
    
typedef struct MPIDI_nem_tcp_header {
    MPIDI_nem_tcp_socksm_pkt_type_t pkt_type;
    MPIDI_msg_sz_t datalen;
} MPIDI_nem_tcp_header_t;

typedef struct MPIDI_nem_tcp_idinfo {
    int pg_rank;
/*      Commented intentionally */
/*        char pg_id[pg_id_len+1];  Memory is dynamically allocated for pg_id_len+1 */
/*      Also, this is optional. Sent only, if the recipient belongs to a different pg. */
/*      As long as another variable length field needs to be sent across(if at all required */
/*      in the future), datalen of header itself is enough to find the offset of pg_id      */
/*      in the packet to be sent. */
} MPIDI_nem_tcp_idinfo_t;

/* FIXME: bc actually contains port_name info */
typedef struct MPIDI_nem_tcp_portinfo {
    int port_name_tag;
} MPIDI_nem_tcp_portinfo_t;


#define MPID_nem_tcp_vc_is_connected(vc_tcp) (vc_tcp->sc && vc_tcp->sc->state.cstate == CONN_STATE_TS_COMMRDY)
#define MPID_nem_tcp_vc_send_paused(vc_tcp) (vc_tcp->send_paused)

#endif
