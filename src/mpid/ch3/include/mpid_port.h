/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2016 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */
#ifndef MPID_PORT_H_INCLUDED
#define MPID_PORT_H_INCLUDED

#include "utlist.h"

#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS

/* Header file only used by port connect/accept routines (ch3u_port.c) */

typedef enum {
    MPIDI_CH3I_PORT_CONNREQ_INITED,     /* Connection request initialized. */
    MPIDI_CH3I_PORT_CONNREQ_REVOKE,     /* Client started revoking a timed out connection request. */
    MPIDI_CH3I_PORT_CONNREQ_ACCEPT,     /* Server started accepting a connection request. */
    MPIDI_CH3I_PORT_CONNREQ_ACCEPTED,   /* Server successfully accepted this request (i.e.,
                                         * client does not revoke it). */
    MPIDI_CH3I_PORT_CONNREQ_ERR_CLOSE,  /* Error state -- Server closed unexpectedly.
                                         * (called close_port or finalize while client is
                                         * still waiting acceptance).
                                         * Note: revoke-state request will be directly
                                         * changed to free-state. */
    MPIDI_CH3I_PORT_CONNREQ_FREE,       /* Started freeing a connection request.
                                         * (VC has been locally closed and ready for blocking
                                         * wait on termination).
                                         * A connection request can be eventually freed when:
                                         * (1) connection normally completed;
                                         * (2) a timed out connection (revoke) request received
                                         *     accept packet;
                                         * (3) server closed unexpectedly.*/
} MPIDI_CH3I_Port_connreq_stat_t;

typedef struct MPIDI_CH3I_Port_connreq {
    MPIDI_VC_t *vc;
    MPIDI_CH3I_Port_connreq_stat_t stat;
    struct MPIDI_CH3I_Port_connreq *next;
} MPIDI_CH3I_Port_connreq_t;

typedef struct MPIDI_CH3I_Port_connreq_q {
    MPIDI_CH3I_Port_connreq_t *head;
    MPIDI_CH3I_Port_connreq_t *tail;
    int size;
} MPIDI_CH3I_Port_connreq_q_t;

typedef struct MPIDI_CH3I_Port {
    int port_name_tag;
    MPIDI_CH3I_Port_connreq_q_t accept_connreq_q;
    struct MPIDI_CH3I_Port *next;
} MPIDI_CH3I_Port_t;

typedef struct MPIDI_CH3I_Port_q {
    MPIDI_CH3I_Port_t *head;
    MPIDI_CH3I_Port_t *tail;
    int size;
} MPIDI_CH3I_Port_q_t;

#define MPIDI_CH3I_PORT_CONNREQ_SET_STAT(connreq, new_stat)     \
                (connreq->stat = MPIDI_CH3I_PORT_CONNREQ_##new_stat)


/* Start VC closing protocol -- locally close VC.
 * It is the 1st step of VC closing protocol (see ch3u_handle_connection.c):
 *      local_closed / remote_closed -> close_acked -> closed. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Port_local_close_vc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Port_local_close_vc(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;

    /* Note that this routine is usually called in comm_release after matched in
     * an accept call. Here we do not have comm for the VC, thus just close it
     * with a dummy rank (this parameter is only for debugging) */
    if (vc->state == MPIDI_VC_STATE_ACTIVE || vc->state == MPIDI_VC_STATE_REMOTE_CLOSE) {
        mpi_errno = MPIDI_CH3U_VC_SendClose(vc, 0 /* dummy rank */);
    }

    return mpi_errno;
}

/*** Utility routines for connection request queues ***/

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Port_connreq_q_enqueue
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_Port_connreq_q_enqueue(MPIDI_CH3I_Port_connreq_q_t * connreq_q,
                                                     MPIDI_CH3I_Port_connreq_t * connreq)
{
    LL_APPEND(connreq_q->head, connreq_q->tail, connreq);
    connreq_q->size++;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Port_connreq_q_dequeue
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_Port_connreq_q_dequeue(MPIDI_CH3I_Port_connreq_q_t * connreq_q,
                                                     MPIDI_CH3I_Port_connreq_t ** connreq_ptr)
{
    MPIDI_CH3I_Port_connreq_t *connreq = NULL;

    /* delete head */
    if (connreq_q->head != NULL) {
        connreq = connreq_q->head;

        LL_DELETE(connreq_q->head, connreq_q->tail, connreq);
        connreq_q->size--;
    }

    (*connreq_ptr) = connreq;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Port_connreq_q_delete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_Port_connreq_q_delete(MPIDI_CH3I_Port_connreq_q_t * connreq_q,
                                                    MPIDI_CH3I_Port_connreq_t * connreq)
{
    LL_DELETE(connreq_q->head, connreq_q->tail, connreq);
    connreq_q->size--;
}



/*** Utility routines for issuing ACK packets in accept/connect routine ***/

/* Reply ACK packet to server accept routine. Client replies this packet after
 * received an accept packet from server. Handled in MPIDI_CH3_PktHandler_AcceptAck.
 * ACK - True means client matched with server acceptance;
 * ACK - False means client already started revoking, thus acceptance fails. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Port_issue_accept_ack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Port_issue_accept_ack(MPIDI_VC_t * vc, int ack)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req_ptr = NULL;
    MPIDI_CH3_Pkt_t spkt;
    MPIDI_CH3_Pkt_accept_ack_t *ack_pkt = &spkt.accept_ack;

    MPIDI_Pkt_init(ack_pkt, MPIDI_CH3_PKT_ACCEPT_ACK);
    ack_pkt->ack = ack;

    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                    (MPL_DBG_FDEST, "issuing accpet_ack packet to vc %p, ack=%d", vc, ack));

    mpi_errno = MPIDI_CH3_iStartMsg(vc, ack_pkt, sizeof(MPIDI_CH3_Pkt_t), &req_ptr);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;

    if (req_ptr != NULL)
        MPIR_Request_free(req_ptr);     /* Only reduce reference, released after pkt sent. */

    return mpi_errno;
}

/* Reply ACK packet to client connect routine. Server replies this packet in
 * accept call for dequeued connection request, or replies immediately when
 * received a request for non-existing port. Handled in MPIDI_CH3_PktHandler_ConnAck
 * on client.
 * ACK - True means server started accepting this connection request;
 * ACK - False means port does not exist or being closed on server, thus connection
 *       fails. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Port_issue_conn_ack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Port_issue_conn_ack(MPIDI_VC_t * vc, int ack)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req_ptr = NULL;
    MPIDI_CH3_Pkt_t spkt;
    MPIDI_CH3_Pkt_conn_ack_t *ack_pkt = &spkt.conn_ack;

    MPIDI_Pkt_init(ack_pkt, MPIDI_CH3_PKT_CONN_ACK);
    ack_pkt->ack = ack;

    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE,
                    (MPL_DBG_FDEST, "issuing conn_ack packet to vc %p, ack=%d", vc, ack));

    mpi_errno = MPIDI_CH3_iStartMsg(vc, ack_pkt, sizeof(MPIDI_CH3_Pkt_t), &req_ptr);
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;

    if (req_ptr != NULL)
        MPIR_Request_free(req_ptr);     /* Only reduce reference, released after pkt sent. */

    return mpi_errno;
}

#endif /* MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS */

#endif /* MPID_PORT_H_INCLUDED */
