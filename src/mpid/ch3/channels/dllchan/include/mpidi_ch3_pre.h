/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

/* These macros unlock shared code */
#define MPIDI_CH3_USES_SOCK
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
MPIDI_CH3I_PKT_SC_OPEN_RESP,			\
MPIDI_CH3I_PKT_SC_CLOSE

/* FIXME - We need a little security here to avoid having a random port scan 
   crash the process.  Perhaps a "secret" value for each process could be 
   published in the key-val space and subsequently sent in the open pkt. */
#define MPIDI_CH3_PKT_DEFS	\
typedef struct			\
{				\
    MPIDI_CH3_Pkt_type_t type;	\
    int pg_id_len;		\
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
    int port_name_tag; 		\
}				\
MPIDI_CH3I_Pkt_sc_conn_accept_t;

#define MPIDI_CH3_PKT_DECL			\
MPIDI_CH3I_Pkt_sc_open_req_t sc_open_req;	\
MPIDI_CH3I_Pkt_sc_conn_accept_t sc_conn_accept;	\
MPIDI_CH3I_Pkt_sc_open_resp_t sc_open_resp;	\
MPIDI_CH3I_Pkt_sc_close_t sc_close;


/* This channel has no special channel data for the process group structure */

/* FIXME: Explain these; why is this separate from the VC state? */
typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTING,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;

typedef struct MPIDI_CH3I_VC
{
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    MPIDI_CH3I_VC_state_t state;
    /*MPIDU_Sock_t sock;*/
    struct MPIDU_Sock *sock;
    struct MPIDI_CH3I_Connection * conn;
}
MPIDI_CH3I_VC;

/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 * The DLL channel makes no additions
 */

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

extern int *MPIDI_CH3I_progress_completion_count_ptr;
#define MPIDI_CH3I_INCR_PROGRESS_COMPLETION_COUNT \
        *MPIDI_CH3I_progress_completion_count_ptr += 1

#define MPIDI_CH3_HASIMPL_HEADER
#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */
