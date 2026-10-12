/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SOCK_TYPES_H_INCLUDED
#define SOCK_TYPES_H_INCLUDED

#include "mpidimpl.h"

#include <poll.h>

/* Every message on a connection starts with this header, followed by
 * am_hdr_sz bytes of active message header and data_sz bytes of data. */
typedef struct {
    uint16_t handler_id;
    uint8_t src_vci;
    uint8_t dst_vci;
    uint32_t am_hdr_sz;
    uint64_t data_sz;
} MPIDI_SOCK_am_header_t;

/* handler_id of the messages of MPIDI_SOCK_dynamic_{send,recv,sendrecv}. They
 * are not active messages. Their am header is MPIDI_SOCK_dynamic_hdr_t. */
#define MPIDI_SOCK_DYNAMIC_HANDLER_ID 0xffff

typedef struct {
    int context_id;
    int tag;
} MPIDI_SOCK_dynamic_hdr_t;

/* A received dynamic message that is waiting for its receive */
typedef struct MPIDI_SOCK_dynamic_msg {
    struct MPIDI_SOCK_dynamic_msg *next;
    MPIDI_SOCK_dynamic_hdr_t hdr;
    MPI_Aint data_sz;
    char *buf;                  /* the whole message body, to be freed */
    char *data;                 /* points into buf */
} MPIDI_SOCK_dynamic_msg_t;

/* A message that could not be written to the socket at once */
typedef struct MPIDI_SOCK_sendq_elem {
    struct MPIDI_SOCK_sendq_elem *next;
    MPIR_Request *sreq;         /* NULL if the message has no request (am_send_hdr) */
    int handler_id;
    void *hdr_buf;              /* copy of MPIDI_SOCK_am_header_t and the am header */
    MPI_Aint hdr_len;
    const void *data;           /* user buffer or pack_buf */
    MPI_Aint data_len;
    void *pack_buf;             /* to be freed when the message is sent, or NULL */
    MPI_Aint offset;            /* bytes already written, counting hdr_buf then data */
} MPIDI_SOCK_sendq_elem_t;

/* A message is received in one of two ways.
 *   - buffered: HDR -> BODY. The am header and the data are read into
 *     recv_buf and the handler gets both.
 *   - direct: HDR -> AM_HDR -> DATA. Used for messages larger than the eager
 *     size. The handler is called with the am header only and returns the
 *     receive request. The data is then read into the receive buffer. */
typedef enum {
    MPIDI_SOCK_RECV_HDR,        /* reading MPIDI_SOCK_am_header_t */
    MPIDI_SOCK_RECV_BODY,       /* reading the am header and data */
    MPIDI_SOCK_RECV_AM_HDR,     /* reading the am header */
    MPIDI_SOCK_RECV_DATA        /* reading the data */
} MPIDI_SOCK_recv_state_t;

/* size of the buffer for data that can't be read into the receive buffer directly */
#define MPIDI_SOCK_RECV_CHUNK_SIZE (64 * 1024)

/* A connection is used in one direction only. An outgoing connection is the
 * one we connected; we only send on it. An incoming connection is the one we
 * accepted; we only receive on it. */
typedef struct MPIDI_SOCK_conn {
    int fd;
    bool is_outgoing;
    int poll_idx;               /* index in MPIDI_SOCK_global.pollfds */

    /* outgoing: messages waiting to be written, in order */
    MPIDI_SOCK_sendq_elem_t *sendq_head;
    MPIDI_SOCK_sendq_elem_t *sendq_tail;

    /* incoming: the message being read */
    MPIDI_SOCK_recv_state_t recv_state;
    MPIDI_SOCK_am_header_t recv_hdr;
    char *recv_buf;             /* am header, followed by data if buffered */
    MPI_Aint recv_need;         /* bytes to read in the current state */
    MPI_Aint recv_got;          /* bytes read so far in the current state */

    /* incoming, in MPIDI_SOCK_RECV_DATA */
    MPIR_Request *recv_rreq;    /* NULL if the handler does not take the data */
    char *recv_direct;          /* read the data here if not NULL, otherwise into recv_chunk */
    char *recv_chunk;           /* MPIDI_SOCK_RECV_CHUNK_SIZE bytes */
    MPI_Aint recv_chunk_len;    /* bytes in recv_chunk not yet handed to the request */
} MPIDI_SOCK_conn_t;

typedef struct {
    int listen_fd;
    MPIDI_SOCK_name_t name;     /* address of this process */
    MPI_Aint eager_size;        /* max size of am header plus data in an eager message */

    /* pollfds[0] is the listening socket and conns[0] is NULL. pollfds[i] is
     * the socket of conns[i] for i > 0. */
    struct pollfd *pollfds;
    MPIDI_SOCK_conn_t **conns;
    int num_pollfds;
    int max_pollfds;

    /* received dynamic messages, in the order of arrival */
    MPIDI_SOCK_dynamic_msg_t *dynamic_head;
    MPIDI_SOCK_dynamic_msg_t *dynamic_tail;
} MPIDI_SOCK_global_t;

extern MPIDI_SOCK_global_t MPIDI_SOCK_global;

#endif /* SOCK_TYPES_H_INCLUDED */
