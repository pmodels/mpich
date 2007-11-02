/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDPRE_H_INCLUDED)
#define MPICH_MPIDPRE_H_INCLUDED

#include "mpidi_ch3_conf.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#include "segment_states.h"
#include "mpid_dataloop.h"
struct MPID_Datatype;

typedef MPI_Aint MPIDI_msg_sz_t;
#define MPIDI_MSG_SZ_FMT "%d"

/* Include definitions from the channel which must exist before items in this file (mpidimpl.h) or the file it includes
   (mpiimpl.h) can be defined. */
#include "mpidi_ch3_pre.h"

#if defined (MPIDI_CH3_MSGS_UNORDERED)
#define MPID_USE_SEQUENCE_NUMBERS
#endif

#if defined(MPID_USE_SEQUENCE_NUMBERS)
typedef unsigned long MPID_Seqnum_t;
#endif

#include "mpichconf.h"
typedef struct MPIDI_Message_match
{
    int32_t tag;
    int16_t rank;
    int16_t context_id;
}
MPIDI_Message_match;

/*
 * MPIDI_CH3_Pkt_type_t
 *
 */
typedef enum MPIDI_CH3_Pkt_type
{
    MPIDI_CH3_PKT_EAGER_SEND = 0,
    MPIDI_CH3_PKT_EAGER_SYNC_SEND,
    MPIDI_CH3_PKT_EAGER_SYNC_ACK,
    MPIDI_CH3_PKT_READY_SEND,
    MPIDI_CH3_PKT_RNDV_REQ_TO_SEND,
    MPIDI_CH3_PKT_RNDV_CLR_TO_SEND,
    MPIDI_CH3_PKT_RNDV_SEND,
    MPIDI_CH3_PKT_CANCEL_SEND_REQ,
    MPIDI_CH3_PKT_CANCEL_SEND_RESP,
    MPIDI_CH3_PKT_PUT,
    MPIDI_CH3_PKT_GET,
    MPIDI_CH3_PKT_GET_RESP,
    MPIDI_CH3_PKT_ACCUMULATE,
    MPIDI_CH3_PKT_FLOW_CNTL_UPDATE,
    MPIDI_CH3_PKT_END_CH3
# if defined(MPIDI_CH3_PKT_ENUM)
    , MPIDI_CH3_PKT_ENUM
# endif    
}
MPIDI_CH3_Pkt_type_t;

typedef struct MPIDI_CH3_Pkt_send
{
    MPIDI_CH3_Pkt_type_t type;  /* XXX - uint8_t to conserve space ??? */
    MPIDI_Message_match match;
    MPI_Request sender_req_id;	/* needed for ssend and send cancel */
    MPIDI_msg_sz_t data_sz;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
}
MPIDI_CH3_Pkt_send_t;

/* NOTE: Normal and synchronous eager sends, as well as all ready-mode sends, use the same structure but have a different type
   value. */
typedef MPIDI_CH3_Pkt_send_t MPIDI_CH3_Pkt_eager_send_t;
typedef MPIDI_CH3_Pkt_send_t MPIDI_CH3_Pkt_eager_sync_send_t;
typedef MPIDI_CH3_Pkt_send_t MPIDI_CH3_Pkt_ready_send_t;

typedef struct MPIDI_CH3_Pkt_eager_sync_ack
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request sender_req_id;
}
MPIDI_CH3_Pkt_eager_sync_ack_t;

typedef MPIDI_CH3_Pkt_send_t MPIDI_CH3_Pkt_rndv_req_to_send_t;

typedef struct MPIDI_CH3_Pkt_rndv_clr_to_send
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request sender_req_id;
    MPI_Request receiver_req_id;
}
MPIDI_CH3_Pkt_rndv_clr_to_send_t;

typedef struct MPIDI_CH3_Pkt_rndv_send
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request receiver_req_id;
}
MPIDI_CH3_Pkt_rndv_send_t;

typedef struct MPIDI_CH3_Pkt_cancel_send_req
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_Message_match match;
    MPI_Request sender_req_id;
}
MPIDI_CH3_Pkt_cancel_send_req_t;

typedef struct MPIDI_CH3_Pkt_cancel_send_resp
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request sender_req_id;
    int ack;
}
MPIDI_CH3_Pkt_cancel_send_resp_t;

#if defined(MPIDI_CH3_PKT_DEFS)
MPIDI_CH3_PKT_DEFS
#endif

typedef struct MPIDI_CH3_Pkt_put
{
    MPIDI_CH3_Pkt_type_t type;
    void *addr;
    int count;
    MPI_Datatype datatype;
    int dataloop_size;   /* for derived datatypes */
    int *decr_ctr; /* address of counter to be decremented on remote
                       side. could be NULL */ 
}
MPIDI_CH3_Pkt_put_t;

typedef struct MPIDI_CH3_Pkt_get
{
    MPIDI_CH3_Pkt_type_t type;
    void *addr;
    int count;
    MPI_Datatype datatype;
    int dataloop_size;   /* for derived datatypes */
    struct MPID_Request *request;
    int *decr_ctr; /* address of counter to be decremented on remote
                       side. could be NULL */ 
}
MPIDI_CH3_Pkt_get_t;

typedef struct MPIDI_CH3_Pkt_get_resp
{
    MPIDI_CH3_Pkt_type_t type;
    struct MPID_Request *request;
}
MPIDI_CH3_Pkt_get_resp_t;

typedef struct MPIDI_CH3_Pkt_accum
{
    MPIDI_CH3_Pkt_type_t type;
    void *addr;
    int count;
    MPI_Datatype datatype;
    int dataloop_size;   /* for derived datatypes */
    MPI_Op op;
    int *decr_ctr; /* address of counter to be decremented on remote
                       side. could be NULL */ 
}
MPIDI_CH3_Pkt_accum_t;

typedef union MPIDI_CH3_Pkt
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_eager_send_t eager_send;
    MPIDI_CH3_Pkt_eager_sync_send_t eager_sync_send;
    MPIDI_CH3_Pkt_eager_sync_ack_t eager_sync_ack;
    MPIDI_CH3_Pkt_eager_send_t ready_send;
    MPIDI_CH3_Pkt_rndv_req_to_send_t rndv_req_to_send;
    MPIDI_CH3_Pkt_rndv_clr_to_send_t rndv_clr_to_send;
    MPIDI_CH3_Pkt_rndv_send_t rndv_send;
    MPIDI_CH3_Pkt_cancel_send_req_t cancel_send_req;
    MPIDI_CH3_Pkt_cancel_send_resp_t cancel_send_resp;
    MPIDI_CH3_Pkt_put_t put;
    MPIDI_CH3_Pkt_get_t get;
    MPIDI_CH3_Pkt_get_resp_t get_resp;
    MPIDI_CH3_Pkt_accum_t accum;
# if defined(MPIDI_CH3_PKT_DECL)
    MPIDI_CH3_PKT_DECL
# endif
}
MPIDI_CH3_Pkt_t;

#if defined(MPID_USE_SEQUENCE_NUMBERS)
typedef struct MPIDI_CH3_Pkt_send_container
{
    MPIDI_CH3_Pkt_send_t pkt;
    struct MPIDI_CH3_Pkt_send_container_s * next;
}
MPIDI_CH3_Pkt_send_container_t;
#endif

/*
 * MPIDI_CH3_CA_t
 *
 * An enumeration of the actions to perform when the requested I/O operation has completed.
 *
 * MPIDI_CH3_CA_COMPLETE - The last operation for this request has completed.  The completion counter should be decremented.  If
 * it has reached zero, then the request should be released by calling MPID_Request_release().
 *
 * MPIDI_CH3_CA_UNPACK_UEBUF_AND_COMPLETE - This is a special case of the MPIDI_CH3_CA_COMPLETE.  The data for an unexpected
 * eager messaage has been stored into a temporary buffer and needs to be copied/unpacked into the user buffer before the
 * completion counter can be decremented, etc.
 *
 * MPIDI_CH3_CA_UNPACK_SRBUF_AND_COMPLETE - This is a special case of the MPIDI_CH3_CA_COMPLETE.  The data from the completing
 * read has been stored into a temporary send/receive buffer and needs to be copied/unpacked into the user buffer before the
 * completion counter can be decremented, etc.
 *
 * MPIDI_CH3_CA_RELOAD_IOV - This request contains more segments of data than the IOV or buffer space allow.  Since the
 * previously request operation has completed, the IOV in the request should be reload at this time.
 *
 * MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV - This is a special case of the MPIDI_CH3_CA_RELOAD_IOV.  The data from the
 * completing read operation has been stored into a temporary send/receive buffer and needs to be copied/unpacked into the user
 * buffer before the IOV is reloaded.
 *
 * MPIDI_CH3_CA_END_CH3 - This not a real action, but rather a marker.  All actions numerically less than MPID_CA_END are defined
 * by channel device.  Any actions numerically greater than MPIDI_CA_END are internal to the channel instance and must be handled
 * by the channel instance.
 */
typedef enum MPIDI_CA
{
    MPIDI_CH3_CA_COMPLETE,
    MPIDI_CH3_CA_UNPACK_SRBUF_AND_COMPLETE,
    MPIDI_CH3_CA_UNPACK_UEBUF_AND_COMPLETE,
    MPIDI_CH3_CA_RELOAD_IOV,
    MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV,
    MPIDI_CH3_CA_END_CH3
# if defined(MPIDI_CH3_CA_ENUM)
    , MPIDI_CH3_CA_ENUM
# endif
}
MPIDI_CA_t;


typedef struct MPIDI_VC
{
    int handle;  /* not used; exists so that we may use the MPIU_Object routines for reference counting */
    volatile int ref_count;
    int lpid;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum_send;
#endif
#if defined(MPIDI_CH3_MSGS_UNORDERED)
    MPID_Seqnum_t seqnum_recv;
    MPIDI_CH3_Pkt_send_container_t * msg_reorder_queue;
#endif
# if defined(MPIDI_CH3_VC_DECL)
    MPIDI_CH3_VC_DECL
# endif
}
MPIDI_VC;

/* to send derived datatype across in RMA ops */
typedef struct MPIDI_RMA_dtype_info { /* for derived datatypes */
    int           is_contig; 
    int           n_contig_blocks;
    int           size;     
    MPI_Aint      extent;   
    int           dataloop_size; /* not needed because this info is sent in packet header. remove it after lock/unlock is implemented in the device */
    void          *dataloop;  /* pointer needed to update pointers
                                 within dataloop on remote side */
    int           dataloop_depth; 
    int           eltype;
    MPI_Aint ub, lb, true_ub, true_lb;
    int has_sticky_ub, has_sticky_lb;
} MPIDI_RMA_dtype_info;


#if defined(MPID_USE_SEQUENCE_NUMBERS)
#   define MPIDI_REQUEST_SEQNUM	\
        MPID_Seqnum_t seqnum;
#else
#   define MPIDI_REQUEST_SEQNUM
#endif

#define MPID_REQUEST_DECL													\
struct MPIDI_Request														\
{																\
    MPIDI_Message_match match;													\
																\
    /* TODO - user_buf, user_count, and datatype needed to process rendezvous messages. */					\
    void * user_buf;														\
    int user_count;														\
    MPI_Datatype datatype;													\
																\
    /* segment, segment_first, and segment_size are used when processing non-contiguous datatypes */				\
    MPID_Segment segment;													\
    MPIDI_msg_sz_t segment_first;												\
    MPIDI_msg_sz_t segment_size;												\
																\
    /* Pointer to datatype for reference counting purposes */									\
    struct MPID_Datatype * datatype_ptr;											\
																\
    /* iov and iov_count define the data to be transferred/received */								\
    MPID_IOV iov[MPID_IOV_LIMIT];												\
    int iov_count;														\
																\
    /* ca (completion action) identifies the action to take once the operation described by the iov has completed */		\
    MPIDI_CA_t ca;														\
																\
    /* tmpbuf and tmpbuf_sz describe temporary storage used for things like unexpected eager messages and packing/unpacking	\
       buffers.  tmpuf_off is the current offset into the temporary buffer. */							\
    void * tmpbuf;														\
    int tmpbuf_off;														\
    MPIDI_msg_sz_t tmpbuf_sz;													\
																\
    MPIDI_msg_sz_t recv_data_sz;												\
    MPI_Request sender_req_id;													\
																\
    unsigned state;														\
    int cancel_pending;													        \
    int recv_pending_count;												        \
																\
    /* The next 6 are for RMA */                                                                                                \
    MPI_Op op;												                        \
    int *decr_ctr;														\
    /* For accumulate, since data is first read into a tmp_buf */								\
    void *real_user_buf;													\
    /* For derived datatypes at target */								                        \
    MPIDI_RMA_dtype_info *dtype_info;												\
    void *dataloop;													        \
    /* req. handle needed to implement derived datatype gets  */					                        \
    struct MPID_Request *request;												        \
																\
    MPIDI_REQUEST_SEQNUM													\
																\
    struct MPID_Request * next;													\
} dev;

#if defined(MPIDI_CH3_REQUEST_DECL)
#define MPID_DEV_REQUEST_DECL			\
MPID_REQUEST_DECL				\
MPIDI_CH3_REQUEST_DECL
#else
#define MPID_DEV_REQUEST_DECL			\
MPID_REQUEST_DECL
#endif

/* prototype for the channel state description function */
/* This function is called from MPIDU_Describe_timer_states() in mpid_describe_states.c */
int CH3U_Describe_timer_states(void);

/* define the device state list to be the concatenation of the mpid
 * list and the channel list */
#define MPID_STATE_LIST_MPID MPID_STATE_LIST_MPIDI MPID_STATE_LIST_CH3

/* define all the states used in the ch3/src directory */
#define MPID_STATE_LIST_MPIDI \
MPID_STATE_MPID_IRSEND, \
MPID_STATE_MPID_ISEND, \
MPID_STATE_MPID_ISSEND, \
MPID_STATE_MPID_PROBE, \
MPID_STATE_MPID_PUT, \
MPID_STATE_MPID_RECV, \
MPID_STATE_MPID_RSEND, \
MPID_STATE_MPID_SEND, \
MPID_STATE_MPID_SSEND, \
MPID_STATE_MPID_WIN_CREATE, \
MPID_STATE_MPID_WIN_COMPLETE, \
MPID_STATE_MPID_WIN_FENCE, \
MPID_STATE_MPID_WIN_FREE, \
MPID_STATE_MPID_WIN_POST, \
MPID_STATE_MPID_WIN_WAIT, \
MPID_STATE_MPID_WIN_LOCK, \
MPID_STATE_MPID_WIN_UNLOCK, \
MPID_STATE_CREATE_REQUEST, \
MPID_STATE_MPID_ABORT, \
MPID_STATE_MPID_CANCEL_RECV, \
MPID_STATE_MPID_CANCEL_SEND, \
MPID_STATE_MPID_IPROBE, \
MPID_STATE_MPID_IRECV, \
MPID_STATE_MPID_SEND_INIT, \
MPID_STATE_MPID_BSEND_INIT, \
MPID_STATE_MPID_RSEND_INIT, \
MPID_STATE_MPID_SSEND_INIT, \
MPID_STATE_MPID_RECV_INIT, \
MPID_STATE_MPID_STARTALL, \
MPID_STATE_MPIDI_BARRIER, \
MPID_STATE_MPIDI_CH3U_HANDLE_UNORDERED_RECV_PKT, \
MPID_STATE_MPIDI_CH3U_HANDLE_ORDERED_RECV_PKT, \
MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ, \
MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ, \
MPID_STATE_MPIDI_CH3U_RECVQ_DP, \
MPID_STATE_MPIDI_CH3U_RECVQ_FDP, \
MPID_STATE_MPIDI_CH3U_RECVQ_FDP_OR_AEU, \
MPID_STATE_MPIDI_CH3U_RECVQ_FDU, \
MPID_STATE_MPIDI_CH3U_RECVQ_FDU_OR_AEP, \
MPID_STATE_MPIDI_CH3U_RECVQ_FU, \
MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_RECV_IOV, \
MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV, \
MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF, \
MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF, \
MPID_STATE_MPID_VCRT_CREATE, \
MPID_STATE_MPID_VCRT_ADD_REF, \
MPID_STATE_MPID_VCRT_RELEASE, \
MPID_STATE_MPID_VCRT_GET_PTR, \
MPID_STATE_MPID_VCR_DUP, \
MPID_STATE_MPID_VCR_RELEASE, \
MPID_STATE_MPID_VCR_GET_LPID, \
MPID_STATE_MPID_COMM_SPAWN, \
MPID_STATE_MPID_COMM_SPAWN_MULTIPLE, \
MPID_STATE_MPID_CLOSE_PORT, \
MPID_STATE_MPID_OPEN_PORT, \
MPID_STATE_MPID_COMM_ACCEPT, \
MPID_STATE_MPID_COMM_CONNECT, \
MPID_STATE_MPID_COMM_DISCONNECT,

#endif /* !defined(MPICH_MPIDPRE_H_INCLUDED) */
