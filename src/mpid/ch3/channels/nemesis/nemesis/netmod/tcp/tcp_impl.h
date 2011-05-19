/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef TCP_IMPL_H
#define TCP_IMPL_H

#include "mpid_nem_impl.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "socksm.h"

/* globals */
extern struct pollfd *MPID_nem_tcp_plfd_tbl;
extern sockconn_t MPID_nem_tcp_g_lstn_sc;
extern struct pollfd MPID_nem_tcp_g_lstn_plfd;

typedef enum{MPID_NEM_TCP_VC_STATE_DISCONNECTED,
             MPID_NEM_TCP_VC_STATE_CONNECTED,
             MPID_NEM_TCP_VC_STATE_ERROR
} MPID_nem_tcp_vc_state_t;

#define MPIDI_NEM_TCP_MAX_CONNECT_RETRIES 100

typedef GENERIC_Q_DECL(struct MPID_Request) MPIDI_nem_tcp_request_queue_t;

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct
{
    struct sockaddr_in sock_id;
    MPID_nem_tcp_vc_state_t state;
    struct MPID_nem_new_tcp_sockconn *sc;
    int send_paused;
    MPIDI_nem_tcp_request_queue_t send_queue;
    MPIDI_nem_tcp_request_queue_t paused_send_queue;
    /* this is a count of how many sc objects refer to this vc */
    int sc_ref_count;
    int connect_retry_count; /* number of times we've tried to connect */
} MPID_nem_tcp_vc_area;

/* macro for tcp private in VC */
#define VC_TCP(vc) ((MPID_nem_tcp_vc_area *)VC_CH((vc))->netmod_area.padding)

#define ASSIGN_SC_TO_VC(vc_tcp_, sc_) do {      \
        (vc_tcp_)->sc = (sc_);                  \
    } while (0)

/* functions */
int MPID_nem_tcp_init (MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_tcp_finalize (void);

#ifdef ENABLE_CHECKPOINTING
int MPID_nem_tcp_ckpt_pause_send_vc(MPIDI_VC_t *vc);
int MPID_nem_tcp_ckpt_continue_vc(MPIDI_VC_t *vc);
int MPID_nem_tcp_ckpt_restart_vc(MPIDI_VC_t *vc);
int MPID_nem_tcp_ckpt_shutdown(void);
#endif

int MPID_nem_tcp_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_tcp_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc);
int MPID_nem_tcp_vc_init (MPIDI_VC_t *vc);
int MPID_nem_tcp_vc_destroy(MPIDI_VC_t *vc);
int MPID_nem_tcp_vc_terminate (MPIDI_VC_t *vc);

/* completion counter is atomically decremented when operation completes */
int MPID_nem_tcp_get (void *target_p, void *source_p, int source_node, int len, int *completion_ctr);
int MPID_nem_tcp_put (void *target_p, int target_node, void *source_p, int len, int *completion_ctr);

int MPID_nem_tcp_send_init(void);
int MPID_nem_tcp_connect(struct MPIDI_VC *const vc);
int MPID_nem_tcp_connpoll(int in_blocking_poll);
int MPID_nem_tcp_sm_init(void);
int MPID_nem_tcp_sm_finalize(void);
int MPID_nem_tcp_set_sockopts(int fd);
MPID_NEM_TCP_SOCK_STATUS_t MPID_nem_tcp_check_sock_status(const struct pollfd *const plfd);
int MPID_nem_tcp_send_finalize(void);
int MPID_nem_tcp_bind(int sockfd);
int MPID_nem_tcp_conn_est(MPIDI_VC_t *vc);
int MPID_nem_tcp_get_conninfo(struct MPIDI_VC *vc, struct sockaddr_in *addr, char **pg_id, int *pg_rank);
int MPID_nem_tcp_get_vc_from_conninfo(char *pg_id, int pg_rank, struct MPIDI_VC **vc);
int MPID_nem_tcp_is_sock_connected(int fd);
int MPID_nem_tcp_disconnect(struct MPIDI_VC *const vc);
int MPID_nem_tcp_cleanup (struct MPIDI_VC *const vc);
int MPID_nem_tcp_cleanup_on_error(MPIDI_VC_t *const vc, int req_errno);
int MPID_nem_tcp_error_out_send_queue(struct MPIDI_VC *const vc, int req_errno);
int MPID_nem_tcp_ckpt_cleanup(void);
int MPID_nem_tcp_state_listening_handler(struct pollfd *const l_plfd, sockconn_t *const l_sc);
int MPID_nem_tcp_send_queued(MPIDI_VC_t *vc, MPIDI_nem_tcp_request_queue_t *send_queue);

int MPID_nem_tcp_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz);
int MPID_nem_tcp_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                    MPID_Request **sreq_ptr);
int MPID_nem_tcp_iStartContigMsg_paused(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                        MPID_Request **sreq_ptr);
int MPID_nem_tcp_SendNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz);
int MPID_nem_tcp_get_addr_port_from_bc(const char *business_card, struct in_addr *addr, in_port_t *port);

void MPID_nem_tcp_vc_dbg_print_sendq(FILE *stream, MPIDI_VC_t *vc);

int MPID_nem_tcp_socksm_finalize(void);
int MPID_nem_tcp_socksm_init(void);
int MPID_nem_tcp_vc_terminated(MPIDI_VC_t *vc);


int MPID_nem_tcp_pkt_unpause_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **rreqp);


/* Macros */

/* system call wrapper -- This retries the syscall each time it is interrupted.  
   Example usage:  instead of writing "ret = write(fd, buf, len);" 
   use: "CHECK_EINTR(ret, write(fd, buf, len)); 
 Caution:
 (1) Some of the system calls have value-result parameters. Those system calls
 should not be used within CHECK_EINTR macro or should be used with CARE.
 For eg. accept, the last parameter (addrlen) is a value-result one. So, even if the
 system call is interrupted, addrlen should be initialized to appropriate value before
 calling it again.

 (2) connect should not be called within a loop. In case, the connect is interrupted after
 the TCP handshake is initiated, calling connect again will only fail. So, select/poll
 should be called to check the status of the socket.
 I don't know what will happen, if a connect is interrupted even before the system call
 tries to initiate TCP handshake. No book/manual doesn't seem to explain this scenario.
*/
#define CHECK_EINTR(var, func) do {             \
        (var) = (func);                         \
    } while ((var) == -1 && errno == EINTR)

#define Q_EMPTY(q) GENERIC_Q_EMPTY (q)
#define Q_HEAD(q) GENERIC_Q_HEAD (q)
#define Q_ENQUEUE_EMPTY(qp, ep) GENERIC_Q_ENQUEUE_EMPTY (qp, ep, next)
#define Q_ENQUEUE(qp, ep) GENERIC_Q_ENQUEUE (qp, ep, next)
#define Q_ENQUEUE_EMPTY_MULTIPLE(qp, ep0, ep1) GENERIC_Q_ENQUEUE_EMPTY_MULTIPLE (qp, ep0, ep1, next)
#define Q_ENQUEUE_MULTIPLE(qp, ep0, ep1) GENERIC_Q_ENQUEUE_MULTIPLE (qp, ep0, ep1, next)
#define Q_DEQUEUE(qp, ep) GENERIC_Q_DEQUEUE (qp, ep, next)
#define Q_REMOVE_ELEMENTS(qp, ep0, ep1) GENERIC_Q_REMOVE_ELEMENTS (qp, ep0, ep1, next)


/* VC list macros */
#define VC_L_EMPTY(q) GENERIC_L_EMPTY (q)
#define VC_L_HEAD(q) GENERIC_L_HEAD (q)
#define SET_PLFD(vc_tcp)   MPID_nem_tcp_plfd_tbl[(vc_tcp)->sc->index].events |= POLLOUT
#define UNSET_PLFD(vc_tcp) MPID_nem_tcp_plfd_tbl[(vc_tcp)->sc->index].events &= ~POLLOUT

/* stack macros */
#define S_EMPTY(s) GENERIC_S_EMPTY (s)
#define S_TOP(s) GENERIC_S_TOP (s)
#define S_PUSH(sp, ep) GENERIC_S_PUSH (sp, ep, next)
#define S_PUSH_MULTIPLE(sp, ep0, ep1) GENERIC_S_PUSH_MULTIPLE (sp, ep0, ep1, next)
#define S_POP(sp, ep) GENERIC_S_POP (sp, ep, next)

/* interface utilities */
/*S
  MPIDU_Sock_ifaddr_t - Structure to hold an Internet address.

+ len - Length of the address.  4 for IPv4, 16 for IPv6.
- ifaddr - Address bytes (as bytes, not characters)

S*/
typedef struct MPIDU_Sock_ifaddr_t {
    unsigned int len;
    int type;
    unsigned char ifaddr[16];
} MPIDU_Sock_ifaddr_t;
int MPIDI_GetIPInterface( MPIDU_Sock_ifaddr_t *ifaddr, int *found );
int MPIDI_Get_IP_for_iface(const char *ifname, MPIDU_Sock_ifaddr_t *ifaddr, int *found);

/* Keys for business cards */
#define MPIDI_CH3I_PORT_KEY "port"
#define MPIDI_CH3I_HOST_DESCRIPTION_KEY "description"
#define MPIDI_CH3I_IFNAME_KEY "ifname"



/* tcp-local packet types */

typedef enum MPIDI_nem_tcp_pkt_type {
#ifdef ENABLE_CHECKPOINTING
    MPIDI_NEM_TCP_PKT_UNPAUSE,
#endif
    MPIDI_NEM_TCP_PKT_NUM_TYPES,
    MPIDI_NEM_TCP_PKT_INVALID = -1 /* force signed, to avoid warnings */
} MPIDI_nem_tcp_pkt_type_t;

#ifdef ENABLE_CHECKPOINTING
typedef struct MPIDI_nem_tcp_pkt_unpause
{
    MPID_nem_pkt_type_t type;
    unsigned subtype;
} MPIDI_nem_tcp_pkt_unpause_t;
#endif



#endif /* TCP_IMPL_H */
