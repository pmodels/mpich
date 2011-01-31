/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

#include "sctp_common.h"

#define MPIDI_CH3_HAS_CHANNEL_CLOSE
#define MPIDI_CH3_CHANNEL_AVOIDS_SELECT

/* These macros unlock shared code */
/* #define MPIDI_CH3_USES_SOCK */
#define MPIDI_CH3_USES_ACCEPTQ


/*
 * Features needed or implemented by the channel
 */
#undef MPID_USE_SEQUENCE_NUMBERS

/* FIXME: These should be removed */
#define MPIDI_DEV_IMPLEMENTS_KVS
#define MPIDI_DEV_IMPLEMENTS_ABORT

/* FIXME: Are the following packet extensions?  Can the socket connect/accept
   packets be made part of the util/sock support? */
#define MPIDI_CH3_PKT_ENUM			\
MPIDI_CH3I_PKT_SC_OPEN_REQ,			\
MPIDI_CH3I_PKT_SC_CONN_ACCEPT,		        \
MPIDI_CH3I_PKT_SC_CLOSE

/* FIXME - We need a little security here to avoid having a random port scan 
   crash the process.  Perhaps a "secret" value for each process could be 
   published in the key-val space and subsequently sent in the open pkt. */
#define MPIDI_CH3_PKT_DEFS	\
typedef struct			\
{				\
    MPIDI_CH3_Pkt_type_t type;	\
    int pg_id_len;	/* ??? this is in the len field of the IOV... */	\
    int pg_rank;		\
}				\
MPIDI_CH3I_Pkt_sc_open_req_t;	\
                                \
typedef struct			\
{				\
    MPIDI_CH3_Pkt_type_t type;	\
    int ack;			\
}				\
MPIDI_CH3I_Pkt_sc_open_resp_t;	\
				\
typedef struct			\
{				\
    MPIDI_CH3_Pkt_type_t type;	\
}				\
MPIDI_CH3I_Pkt_sc_close_t;      \
                                \
typedef struct			\
{				\
    MPIDI_CH3_Pkt_type_t type;	\
    int bizcard_len;	        \
    int port_name_tag; 		\
    int ack;                    \
}				\
MPIDI_CH3I_Pkt_sc_conn_accept_t;

#define MPIDI_CH3_PKT_DECL			\
MPIDI_CH3I_Pkt_sc_open_req_t sc_open_req;	\
MPIDI_CH3I_Pkt_sc_conn_accept_t sc_conn_accept;	\
MPIDI_CH3I_Pkt_sc_open_resp_t sc_open_resp;	\
MPIDI_CH3I_Pkt_sc_close_t sc_close;


typedef struct MPIDI_CH3I_PG
{
    char * kvs_name;
}
MPIDI_CH3I_PG;


typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTING,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;


/* stream state */
typedef enum SCTP_Stream_state {
    SCTP_Stream_state_recv_pkt,
    SCTP_Stream_state_recv_data
} 
SCTP_Stream_state_t; 

/* Actively progress this number of requests for read and this number for
 *  write at the same time, taking advantage of SCTP's multistreaming feature.
 */
#define MPICH_SCTP_NUM_REQS_ACTIVE 10

/* We want to allocated one more so that stream zero is used as a control stream
 *  here in ch3:sctp.  We define a new constant in case we want to ever use
 *  more than one control stream.
 */

#define MPICH_SCTP_NUM_REQS_ACTIVE_TO_INIT MPICH_SCTP_NUM_REQS_ACTIVE + 1

/* This is the number of streams setup on an association.  The additional ones could
 *  be used as a control streams.  The number below is the number of currently used
 *  control streams.
 */

#define MPICH_SCTP_NUM_STREAMS MPICH_SCTP_NUM_REQS_ACTIVE_TO_INIT + 1

#define MPICH_SCTP_CTL_STREAM 0

#define MPIDU_SCTP_INFINITE_TIME -1

/* We want to allocated one more so that stream zero isn't a special case since
 *  it is used as a control stream.  We define a new constant in case we want to ever use
 *  more than one control stream.
 */
#define MPICH_SCTP_NUM_STREAMS_TO_INIT MPICH_SCTP_NUM_STREAMS + 1

typedef struct sctp_sndrcvinfo sctp_rcvinfo;

typedef struct my_iov {
    char write_iov_flag;

    union {
	struct {
	    struct iovec* ptr;
	    int iov_count;
	    int offset;
	} iov;
	struct {
	    char* ptr;
	    MPIU_Size_t min;
	    MPIU_Size_t max;
	} buf;
    } write;

} SCTP_IOV;

/* stream struct */
typedef struct MPIDI_CH3I_SCTP_Stream {
    struct MPIDI_VC* vc;                       /* 1-to-1 relationship, probably not needed */
    char have_sent_pg_id;
    struct MPID_Request* send_active;
    struct MPID_Request* recv_active;
    struct MPID_Request* sendQ_head;
    struct MPID_Request* sendQ_tail;
    MPID_IOV iov[2];                           /* general purpose iov for stream, use for connection packet */
} MPIDI_CH3I_SCTP_Stream_t;


#define MPIDI_CH3_PG_DECL MPIDI_CH3I_PG ch;
typedef struct MPIDI_CH3I_VC
{
    /* NOTE: if the control streams are ever used, these will change to NUM_STREAMS_TO_INIT.
     */    
    MPIDI_CH3I_VC_state_t state;

    int port_name_tag;
 
    /* stream table, each stream is independent */
    struct MPIDI_CH3I_SCTP_Stream stream_table[MPICH_SCTP_NUM_REQS_ACTIVE_TO_INIT];
    
    /* file descriptor for one-to-many SCTP socket.  Will be identical to listener
    *   this field may be unneccessary.
    */
    int fd;

    /* process group ID, formerly in MPIDI_CH3I_Connection.  */
    char * pg_id;   /* do we need this since pg_id is in the id field in the PG?
                     *  SCTP requires no exlicit connect/accept (sock channel
                     *  doesn't know where a new connection is from until it reads
                     *  the first message AFTER it does an accept)
                     */

    /* target address */
    struct sockaddr_in to_address;

    /* notion of POST */
    SCTP_IOV posted_iov[MPICH_SCTP_NUM_REQS_ACTIVE_TO_INIT];

    /* connection packet (MPIDI_CH3_Pkt_t) */
    union MPIDI_CH3_Pkt* pkt;

    unsigned short send_init_count;

    sctp_assoc_t sinfo_assoc_id;

    void * sendq_head; /* FIXME this is called in mpid_vc.c */
}
MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC ch;


/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
/*  pkt is used to temporarily store a packet header associated with this 
    request */	
#define MPIDI_CH3_REQUEST_DECL	\
struct MPIDI_CH3I_Request	\
{				\
    struct MPIDI_VC* vc;                    \
    int stream;                        \
} ch;


/*
 * MPID_Progress_state - device/channel dependent state to be passed between 
 * MPID_Progress_{start,wait,end}
 *
 */
typedef struct MPIDI_CH3I_Progress_state
{
    int completion_count;
}
MPIDI_CH3I_Progress_state;

#define MPIDI_CH3_PROGRESS_STATE_DECL MPIDI_CH3I_Progress_state ch;


/* This variable is used in the definitions of the MPID_Progress_xxx macros,
   and must be available to the routines in src/mpi */
extern volatile unsigned int MPIDI_CH3I_progress_completion_count;

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */
