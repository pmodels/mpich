/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SOCKSM_H_INCLUDED
#define SOCKSM_H_INCLUDED

#ifdef HAVE_SYS_POLL_H
    #include <sys/poll.h>
#endif
#include <stddef.h>
#ifdef HAVE_WINNT_H
    #include <winnt.h>
#endif
#include "mpiu_os_wrappers.h"
#include "mpiu_ex.h"

enum SOCK_CONSTS {  /* more type safe than #define's */
    LISTENQLEN = 10,
    POLL_CONN_TIME = 2
};

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

enum CONSTS {
    CONN_TBL_INIT_SIZE = 20,
    CONN_TBL_GROW_SIZE = 10,
    CONN_WAITSET_INIT_SIZE = 20,
    CONN_INVALID_RANK = -1,
    NEGOMSG_DATA_LEN = 4, /* Length of data during negotiatiion message exchanges. */
    SLEEP_INTERVAL = 1500,
    PROGSM_TIMES = 20
};


typedef enum {
    MPID_NEM_NEWTCP_MODULE_SOCK_ERROR_EOF, /* either a socket error or EOF received from peer */
    MPID_NEM_NEWTCP_MODULE_SOCK_CONNECTED,
    MPID_NEM_NEWTCP_MODULE_SOCK_NOEVENT /*  No poll event on socket */
}MPID_NEM_NEWTCP_MODULE_SOCK_STATUS_t;

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
    M_(CONN_STATE_TS_CLOSED),                   \
    M_(CONN_STATE_TC_C_CNTING),                 \
    M_(CONN_STATE_TC_C_CNTD),                   \
    M_(CONN_STATE_TC_C_RANKSENT),               \
    M_(CONN_STATE_TC_C_TMPVCSENT),              \
    M_(CONN_STATE_TC_C_UB_),                    \
    M_(CONN_STATE_TA_C_CNTD),                   \
    M_(CONN_STATE_TA_C_RANKRCVD),               \
    M_(CONN_STATE_TA_C_TMPVCRCVD),              \
    M_(CONN_STATE_TA_C_UB_),                    \
    M_(CONN_STATE_TS_COMMRDY),                  \
    M_(CONN_STATE_TS_D_QUIESCENT)

#define SOCK_STATE_                             \
    LISTEN_STATE_,                              \
    CONN_STATE_

#define IS_LISTEN_CONN_STATE(_state)    ((CONN_STATE_TC_C_UB_ < (_state)) &&    \
                                        ((_state) < CONN_STATE_TA_C_UB_) )

/* REQ - Request, RSP - Response */

typedef enum CONN_TYPE {CONN_TYPE_, CONN_TYPE_SIZE} Conn_type_t;

typedef enum MPID_nem_newtcp_module_sock_state {SOCK_STATE_LB_, SOCK_STATE_, SOCK_STATE_SIZE}
    MPID_nem_newtcp_module_sock_state_t;

#define SOCK_STATE_TO_STRING(_state) \
    (( (_state) > SOCK_STATE_LB_ && (_state) < SOCK_STATE_SIZE ) ? SOCK_STATE_STR[_state] : "out_of_range")
/*
  Note: event numbering starts from 1, as 0 is assumed to be the state of all-events cleared
 */
typedef enum sockconn_event {EVENT_CONNECT = 1, EVENT_DISCONNECT} 
    sockconn_event_t;

#undef M_
#define M_(x) #x

#if defined(SOCKSM_H_DEFGLOBALS_)
const char *const CONN_TYPE_STR[CONN_TYPE_SIZE] = {CONN_TYPE_};
const char *const SOCK_STATE_STR[SOCK_STATE_SIZE] = {SOCK_STATE_};
#elif defined(SOCKSM_H_EXTERNS_)
extern const char *const CONN_TYPE_STR[];
extern const char *const SOCK_STATE_STR[];
#endif

#undef M_

#define SOCKCONN_EX_RD_HANDLERS_SET(sc, succ_fn, fail_fn)      \
            MPIU_ExInitOverlapped(&(sc->rd_ov), succ_fn, fail_fn)
#define SOCKCONN_EX_WR_HANDLERS_SET(sc, succ_fn, fail_fn)      \
            MPIU_ExInitOverlapped(&(sc->wr_ov), succ_fn, fail_fn)

#define CHANGE_STATE(_sc, _state) do {                              \
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "CHANGE_STATE %p (state= %s to %s)", _sc, SOCK_STATE_TO_STRING((_sc)->state), SOCK_STATE_TO_STRING(_state))); \
    (_sc)->state = _state;                               \
    SOCKCONN_EX_RD_HANDLERS_SET((_sc), sc_state_info[_state].sc_state_rd_success_handler, sc_state_info[_state].sc_state_rd_fail_handler); \
    SOCKCONN_EX_WR_HANDLERS_SET((_sc), sc_state_info[_state].sc_state_wr_success_handler, sc_state_info[_state].sc_state_wr_fail_handler); \
} while(0)

/* We might need to explicitly call the handlers when we don't post
 * a req to EX . This will help us compare perf with and without
 * posting all req to EX.
 */
#define CALL_RD_HANDLER(_sc_p)  (sc_state_info[_sc_p->state].sc_state_rd_success_handler(&(_sc_p->rd_ov)))
#define CALL_RD_FAIL_HANDLER(_sc_p)  (sc_state_info[_sc_p->state].sc_state_rd_fail_handler(&(_sc_p->rd_ov)))
#define CALL_WR_HANDLER(_sc_p)  (sc_state_info[_sc_p->state].sc_state_wr_success_handler(&(_sc_p->wr_ov)))
#define CALL_WR_FAIL_HANDLER(_sc_p)  (sc_state_info[_sc_p->state].sc_state_wr_fail_handler(&(_sc_p->wr_ov)))

struct MPID_nem_new_tcp_module_sockconn;
typedef struct MPID_nem_new_tcp_module_sockconn sockconn_t;

typedef int (*handler_func_t) (MPIU_EXOVERLAPPED *ov);

/* FIXME: This structure can be moved to sock wrappers once MPIU_Sock_fd_t
 * becomes more opaque OR we replace MPIU_Sock_fd_t with an opaque MPIU_Sock_t
 * Also, look into the MSMPI code on how to organize this private struct members into
 * different states, listen_state/accept_state/read_state/write_state,
 * depending on the type of socket.
 */
typedef struct sock_read_context{
    /* tmp iov is required for supporting reads posted with ex without iov */
    MPID_IOV tmp_iov;
    /* iov points to the current IOV being read */
    MPID_IOV *iov;
    /* tmp_iov and iov are not used at the same time */
    /* n_iov is the length of tmp_iov/iov */
    int n_iov;
    /* nb shows the number of bytes read */
    int nb;
} sock_read_context_t;

typedef struct sock_write_context{
    /* tmp iov is required for supporting writes posted with ex without iov */
    MPID_IOV tmp_iov;
    /* iov points to the current IOV being written */
    MPID_IOV *iov;
    /* tmp_iov and iov are not used at the same time */
    /* n_iov is the current length of tmp_iov/iov */
    int n_iov;
    /* nb shows the number of bytes written */
    int nb;
} sock_write_context_t;

typedef struct sock_listen_context{
    /* Accept buffer is used for storing local address and remote address
     * As per msdn docs, when using 
     * sizeof buffer for local address >= 16 + sizeof(local address of server)
     * & sizeof buffer for remote address >= 16 + sizeof(remote address)
     */
    char accept_buffer[sizeof(struct sockaddr_in)*2+32];
    /* connfd is the newly connected sock */
    MPIU_SOCKW_Sockfd_t connfd;
    /* nb = bytes read from the AcceptEx() call */
    int nb;
} sock_listen_context_t;

/* Max retries to connect to a host */
#define MAX_CONN_RETRIES    5
typedef struct sock_connect_context{
    /* Host to connect to ...*/
    const char *host;
    /* Port to connect to ...*/
    int port;
    /* Currently instead of storing host and port we just store
     * the pointer to sockaddr struct
     */
    struct sockaddr_in sin;
    /* No of retries */
    int retry_cnt;
} sock_connect_context_t;

typedef struct sock_close_context{
    /* 0/1 - 1 if sock conn is closing */
    int conn_closing;
} sock_close_context_t;

/* 1 if sockconn is closing, 0 otherwise */
#define IS_SOCKCONN_CLOSING(scp)   (scp->close.conn_closing == 1)
/* FIXME: Check whether the elements of sockconn is aligned correctly */
typedef struct MPID_nem_new_tcp_module_sockconn{
    MPIU_SOCKW_Sockfd_t fd;
    /* Executive overlapped struct for read/listen */
    MPIU_EXOVERLAPPED   rd_ov;
    /* Executive overlapped struct for write/connect */
    MPIU_EXOVERLAPPED   wr_ov;
 
    sock_read_context_t     read;
    sock_write_context_t    write;
    union{
        sock_connect_context_t  connect;
        sock_listen_context_t   accept;
    };
    sock_close_context_t    close;
    /* Used during posting read/write for id/tmp_vc & reading command headers */
    char *tmp_buf;
    int tmp_buf_len;

    /* FIXME: Remove the index. We no longer need it */
    int index;

    /* Used to prevent the usage of uninitialized pg info.  is_same_pg, pg_rank,
     * and pg_id are _ONLY VALID_ if this (pg_is_set) is true */
    int pg_is_set;   
    int is_same_pg;  /* TRUE/FALSE -  */
/*     FIXME: see whether this can be removed, by using only pg_id = NULL or non-NULL */
/*      NULL = if same_pg and valid pointer if different pgs. */

    int is_tmpvc;
    int pg_rank; /*  rank and id cached here to avoid chasing pointers in vc and vc->pg */
    char *pg_id; /*  MUST be used only if is_same_pg == FALSE */

    MPID_nem_newtcp_module_sock_state_t state;
    MPIDI_VC_t *vc;
    /* Conn_type_t conn_type;  Probably useful for debugging/analyzing purposes. */
} MPID_nem_new_tcp_module_sockconn_t;

/* FIXME: CONTAINING_RECORD is only defined for windows 
 * - Use offsetof() to define CONTAINING_RECORD() on unix
 */
#define GET_SOCKCONN_FROM_EX_RD_OV(ov_addr)                    \
            CONTAINING_RECORD(ov_addr, MPID_nem_new_tcp_module_sockconn_t, rd_ov)
#define GET_SOCKCONN_FROM_EX_WR_OV(ov_addr)                    \
            CONTAINING_RECORD(ov_addr, MPID_nem_new_tcp_module_sockconn_t, wr_ov)


typedef enum MPIDI_nem_newtcp_module_pkt_type {
    MPIDI_NEM_NEWTCP_MODULE_PKT_ID_INFO, /*  ID = rank + pg_id */
    MPIDI_NEM_NEWTCP_MODULE_PKT_ID_ACK,
    MPIDI_NEM_NEWTCP_MODULE_PKT_ID_NAK,
    MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_REQ,
    MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_ACK,
    MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_NAK,
    MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_INFO,
    MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_ACK,
    MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_NAK
} MPIDI_nem_newtcp_module_pkt_type_t;
    
typedef struct MPIDI_nem_newtcp_module_header {
    MPIDI_nem_newtcp_module_pkt_type_t pkt_type;
    int datalen;
} MPIDI_nem_newtcp_module_header_t;

typedef struct MPIDI_nem_newtcp_module_idinfo {
    int pg_rank;
/*      Commented intentionally */
/*        char pg_id[pg_id_len+1];  Memory is dynamically allocated for pg_id_len+1 */
/*      Also, this is optional. Sent only, if the recipient belongs to a different pg. */
/*      As long as another variable length field needs to be sent across(if at all required */
/*      in the future), datalen of header itself is enough to find the offset of pg_id      */
/*      in the packet to be sent. */
} MPIDI_nem_newtcp_module_idinfo_t;

/* FIXME: bc actually contains port_name info */
typedef struct MPIDI_nem_newtcp_module_portinfo {
    int port_name_tag;
} MPIDI_nem_newtcp_module_portinfo_t;

#define MPID_nem_newtcp_module_vc_is_connected(vc) (VC_FIELD(vc, sc) && VC_FIELD(vc, sc)->state == CONN_STATE_TS_COMMRDY)
int MPID_nem_newtcp_module_state_accept_success_handler(MPIU_EXOVERLAPPED *rd_ov);
int MPID_nem_newtcp_module_state_accept_fail_handler(MPIU_EXOVERLAPPED *rd_ov);
int MPID_nem_newtcp_module_post_readv_ex(sockconn_t *sc, MPID_IOV *iov, int n_iov);
int MPID_nem_newtcp_module_post_read_ex(sockconn_t *sc, void *buf, int nb);
int MPID_nem_newtcp_module_post_writev_ex(sockconn_t *sc, MPID_IOV *iov, int n_iov);
int MPID_nem_newtcp_module_post_write_ex(sockconn_t *sc, void *buf, int nb);
#endif
