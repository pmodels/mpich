/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef NEWTCP_MODULE_IMPL_H
#define NEWTCP_MODULE_IMPL_H

#include "mpid_nem_impl.h"
/* #include "newtcp_module.h" */
#ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
#endif
#ifdef HAVE_ERRNO_H
    #include <errno.h>
#endif
#ifdef HAVE_NETINET_IN_H
    #include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
    #include <netinet/tcp.h>
#endif
#ifdef HAVE_NETDB_H
    #include <netdb.h>
#endif

#include "mpiu_ex.h"
#include "socksm.h"
#include "pmi.h"

/* globals */
extern MPID_nem_queue_ptr_t MPID_nem_newtcp_module_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;
extern int MPID_nem_newtcp_module_listen_fd;

typedef enum{
    MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED=0,
    MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTED
} MPID_Nem_newtcp_module_vc_state_t;

extern char *MPID_nem_newtcp_module_recv_buf;
#define MPID_NEM_NEWTCP_MODULE_RECV_MAX_PKT_LEN 2048
#define MPID_NEM_NEWTCP_MODULE_RCVBUF_SZ    (128*1024)
#define MPID_NEM_NEWTCP_MODULE_SNDBUF_SZ    (128*1024)

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct 
{
    struct sockaddr_in sock_id;
    struct MPID_nem_new_tcp_module_sockconn *sc;
    struct
    {
        struct MPID_Request *head;
        struct MPID_Request *tail;
    } send_queue;
    /* this is a count of how many sc objects refer to this vc */
    MPID_Nem_newtcp_module_vc_state_t state;
    int sc_ref_count;
} MPID_nem_newtcp_module_vc_area;

/* accessor macro to private fields in VC */
#define VC_FIELD(vc, field) (((MPID_nem_newtcp_module_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->field)

#define ASSIGN_SC_TO_VC(vc_, sc_) do {      \
        VC_FIELD((vc_), sc) = (sc_);        \
    } while (0)

/* functions */
int MPID_nem_newtcp_module_init (MPIDI_PG_t *pg_p, int pg_rank,
                                 char **bc_val_p, int *val_max_sz_p);
int MPID_nem_newtcp_module_finalize (void);
int MPID_nem_newtcp_module_ckpt_shutdown (void);
int MPID_nem_newtcp_module_poll(int);
int MPID_nem_newtcp_module_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_newtcp_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc);
int MPID_nem_newtcp_module_vc_init (MPIDI_VC_t *vc);
int MPID_nem_newtcp_module_vc_destroy(MPIDI_VC_t *vc);
int MPID_nem_newtcp_module_vc_terminate (MPIDI_VC_t *vc);

/* completion counter is atomically decremented when operation completes */
int MPID_nem_newtcp_module_get (void *target_p, void *source_p, int source_node, int len, int *completion_ctr);
int MPID_nem_newtcp_module_put (void *target_p, int target_node, void *source_p, int len, int *completion_ctr);

int MPID_nem_newtcp_module_send_init (void);
int MPID_nem_newtcp_module_handle_sendq_head_req(MPIDI_VC_t *vc, int *req_complete);
int MPID_nem_newtcp_module_send_queued (MPIDI_VC_t *vc);
int MPID_nem_newtcp_module_poll_init (void);
int MPID_nem_newtcp_module_connect (struct MPIDI_VC *const vc);
/* FIXME: WINTCP ASYNC
int MPID_nem_newtcp_module_conn_wr_enable (struct MPIDI_VC *const vc);
int MPID_nem_newtcp_module_conn_wr_disable (struct MPIDI_VC *const vc);
*/
int MPID_nem_newtcp_module_connpoll (int );
int MPID_nem_newtcp_module_sm_init (void);
int MPID_nem_newtcp_module_sm_finalize (void);
int MPID_nem_newtcp_module_set_sockopts (MPIU_SOCKW_Sockfd_t fd);
/* FIXME: WINTCP ASYNC
MPID_NEM_NEWTCP_MODULE_SOCK_STATUS_t MPID_nem_newtcp_module_check_sock_status(MPIU_SOCKW_Waitset_sock_hnd_t fd_ws_hnd);
*/
int MPID_nem_newtcp_module_poll_finalize (void);
int MPID_nem_newtcp_module_send_finalize (void);
int MPID_nem_newtcp_module_bind (MPIU_SOCKW_Sockfd_t sockfd);
/* FIXME: WINTCP ASYNC
int MPID_nem_newtcp_module_recv_handler (MPIU_SOCKW_Waitset_sock_hnd_t fd_ws_hnd, sockconn_t *sc);
*/
int MPID_nem_newtcp_module_recv_handler (MPIU_EXOVERLAPPED *ov);
int MPID_nem_newtcp_module_conn_est (MPIDI_VC_t *vc);
int MPID_nem_newtcp_module_get_conninfo (struct MPIDI_VC *vc, struct sockaddr_in *addr, char **pg_id, int *pg_rank);
int MPID_nem_newtcp_module_get_vc_from_conninfo (char *pg_id, int pg_rank, struct MPIDI_VC **vc);
int MPID_nem_newtcp_module_get_addr_port_from_bc (const char *business_card, struct in_addr *addr, in_port_t *port);
int MPID_nem_newtcp_module_is_sock_connected(int fd);
int MPID_nem_newtcp_module_disconnect (struct MPIDI_VC *const vc);
int MPID_nem_newtcp_module_cleanup (struct MPIDI_VC *const vc);
/* FIXME: WINTCP ASYNC
int MPID_nem_newtcp_module_state_listening_handler(MPIU_SOCKW_Waitset_sock_hnd_t fd_ws_hnd, sockconn_t *const l_sc);
*/
int MPID_nem_newtcp_module_state_listening_handler(MPIU_EXOVERLAPPED *ov);

int MPID_nem_newtcp_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz);
int MPID_nem_newtcp_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                    MPID_Request **sreq_ptr);
int MPID_nem_newtcp_SendNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz);

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
/* CHECK_EINTR is now deprecated. Use MPIU_OSW_RETRYON_EINTR() instead*/
#define CHECK_EINTR(var, func) do {             \
        (var) = (func);                         \
    } while ((var) == -1 && errno == EINTR)

/* Send queue macros */
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

/* stack macros */
#define S_EMPTY(s) GENERIC_S_EMPTY (s)
#define S_TOP(s) GENERIC_S_TOP (s)
#define S_PUSH(sp, ep) GENERIC_S_PUSH (sp, ep, next)
#define S_PUSH_MULTIPLE(sp, ep0, ep1) GENERIC_S_PUSH_MULTIPLE (sp, ep0, ep1, next)
#define S_POP(sp, ep) GENERIC_S_POP (sp, ep, next)

#endif /* NEWTCP_MODULE_IMPL_H */
