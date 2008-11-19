/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

#include "mpidi_ch3i_sctp_conf.h"
#include "mpidi_ch3_conf.h"
#include "mpidimpl.h"

#include "all_hash.h"

typedef int (* MPIDU_Sock_progress_update_func_t)(MPIU_Size_t num_bytes, void * user_ptr);

typedef struct MPIDU_Sock_ifaddr_t {
    int len, type;
    unsigned char ifaddr[16];
} MPIDU_Sock_ifaddr_t;

#define MPIDU_SOCK_ERR_NOMEM -1
#define MPIDU_SOCK_ERR_TIMEOUT -2

/*  hash table entry */
typedef struct hash_entry
{
    sctp_assoc_t assoc_id;
    MPIDI_VC_t * vc;
} MPIDI_CH3I_Hash_entry;

/* global hash table */
extern HASH* MPIDI_CH3I_assocID_table;

/* one_to_many socket */
extern int MPIDI_CH3I_onetomany_fd;

/* number of items in the sendq's */
extern int sendq_total;

extern int MPIDI_CH3I_listener_port;

/* used for dynamic processes */
extern MPIDI_VC_t * MPIDI_CH3I_dynamic_tmp_vc;
extern int MPIDI_CH3I_dynamic_tmp_fd;

/* determine the stream # of a req */
/*  want to avoid stream zero so we can use it as a control stream.
 *    also, we want to use context so that collectives don't cause head-of-line blocking
 *    for p2p...
 */
/*#define Req_Stream_from_match(match) (match.tag)%MPICH_SCTP_NUM_STREAMS*/
/* the following keeps stream s.t. 1 <= stream <= MPICH_SCTP_NUM_REQS_ACTIVE */
#define Req_Stream_from_match(match) (abs((match.parts.tag) + (match.parts.context_id))% MPICH_SCTP_NUM_REQS_ACTIVE)+1
#define REQ_Stream(req) Req_Stream_from_match(req->dev.match)
int Req_Stream_from_pkt_and_req(MPIDI_CH3_Pkt_t * pkt, MPID_Request * sreq);

/* initialize stream */
#define STREAM_INIT(x) \
{\
    x-> vc = NULL;\
    x-> have_sent_pg_id = MPIDI_CH3I_VC_STATE_UNCONNECTED;\
    x-> send_active = NULL;\
    x-> recv_active = NULL;\
    x-> sendQ_head = NULL;\
    x-> sendQ_tail = NULL;\
}

/* use of STREAM macros to allow change of logic in future */
#define SEND_CONNECTED(vc, x) vc->ch.stream_table[x].have_sent_pg_id
#define SEND_ACTIVE(vc, x) vc->ch.stream_table[x].send_active
#define RECV_ACTIVE(vc, x) vc->ch.stream_table[x].recv_active
#define VC_SENDQ(vc, x) vc->ch.stream_table[x].sendQ_head
#define VC_SENDQ_TAIL(vc, x) vc->ch.stream_table[x].sendQ_tail
#define VC_IOV(vc, x) vc->ch.stream_table[x].iov

/* MT - not thread safe! */
/*  enqueue to a specific stream */
#define MPIDI_CH3I_SendQ_enqueue_x(vc, req, x)                \
{                                                           \
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue vc=%p req=0x%08x", vc, req->handle));  \
    sendq_total++; \
    req->dev.next = NULL;						\
    if (VC_SENDQ_TAIL(vc, x) != NULL)					\
    {									\
	VC_SENDQ_TAIL(vc, x)->dev.next = req;				\
    }									\
    else								\
    {									\
	VC_SENDQ(vc, x) = req;					\
    }									\
    VC_SENDQ_TAIL(vc, x) = req;						\
}

/* MT - not thread safe! */
/*  enqueue to a particular stream */
#define MPIDI_CH3I_SendQ_enqueue_head_x(vc, req, x)				\
{									\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue_head vc=%p req=0x%08x", vc, req->handle));\
    sendq_total++; \
    req->dev.next = VC_SENDQ(vc, x);					\
    if (VC_SENDQ_TAIL(vc, x) == NULL)					\
    {									\
	VC_SENDQ_TAIL(vc, x) = req;					\
    }									\
    VC_SENDQ(vc, x) = req;						\
}


/* MT - not thread safe! */
#define MPIDI_CH3I_SendQ_dequeue_x(vc, x)					\
{									\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_dequeue vc=%p req=0x%08x", vc, VC_SENDQ(vc,x)->handle));\
    MPIU_Assert(sendq_total > 0);\
    sendq_total--; \
    VC_SENDQ(vc, x) = VC_SENDQ(vc, x)->dev.next;\
    if (VC_SENDQ(vc, x) == NULL)					\
    {									\
	VC_SENDQ_TAIL(vc, x) = NULL;					\
    }									\
}

/*  sendQ head for specific stream */
#define MPIDI_CH3I_SendQ_head_x(vc, x) (VC_SENDQ(vc, x))
#define MPIDI_CH3I_SendQ_empty_x(vc, x) (VC_SENDQ(vc, x) == NULL)

/*  posted IOV macros */
#define POST_IOV(x) (x->write.iov.ptr)
#define POST_IOV_CNT(x) (x->write.iov.iov_count)
#define POST_IOV_OFFSET(x) (x->write.iov.offset)
#define POST_IOV_FLAG(x) (x->write_iov_flag) 
#define POST_BUF(x) (x->write.buf.ptr)
#define POST_BUF_MIN(x) (x->write.buf.min)
#define POST_BUF_MAX(x) (x->write.buf.max)
#define POST_UPDATE_FN(x) (x)

/*  determine if SCTP_WAIT should keep spinning */
#define SPIN(x) (x == 0)? FALSE : TRUE

/*  global event queue */

#define MPIDU_SCTP_EVENTQ_POOL_SIZE 20

typedef enum MPIDU_Sctp_op
{
    MPIDU_SCTP_OP_READ,
    MPIDU_SCTP_OP_WRITE,
    MPIDU_SCTP_OP_CLOSE,
    MPIDU_SCTP_OP_WAKEUP,
    MPIDU_SCTP_OP_ACCEPT
} MPIDU_Sctp_op_t;

typedef struct MPIDU_Sctp_event
{
    MPIDU_Sctp_op_t op_type;
    MPIU_Size_t num_bytes;
    int fd; /* in case we ever use more than one socket */
    sctp_rcvinfo sri;  /* this points to sctp_sndrcvinfo always for now on read */
    /* int stream_no; */  /* this is in the sri for read but no sri is provided for write */
    void * user_ptr;  /* this points to sctp_advbuf always for reads, and to vc for writes */
    void * user_ptr2;
    int user_value; /* can differ from flags in sri */
    int error;

} MPIDU_Sctp_event_t;

struct MPIDU_Sctp_eventq_elem {
    MPIDU_Sctp_event_t event[MPIDU_SCTP_EVENTQ_POOL_SIZE];
    int size;
    int head;
    int tail;
    struct MPIDU_Sctp_eventq_elem* next;
};

/* need global event queues, because we no longer have sock_set */
extern struct MPIDU_Sctp_eventq_elem* eventq_head;
extern struct MPIDU_Sctp_eventq_elem* eventq_tail;


int MPIDU_Sctp_event_enqueue(MPIDU_Sctp_op_t op, MPIU_Size_t num_bytes, 
				    sctp_rcvinfo* sri, int fd, 
				    void * user_ptr, void * user_ptr2, int msg_flags, int error);
int MPIDU_Sctp_event_dequeue(struct MPIDU_Sctp_event * eventp);

void MPIDU_Sctp_free_eventq_mem(void);

void print_SCTP_event(struct MPIDU_Sctp_event * eventp);

/*  send queue */
void MPIDU_Sctp_stream_init(MPIDI_VC_t* vc, MPID_Request* req, int stream);

int MPIDU_Sctp_writev_fd(int fd, struct sockaddr_in * to, struct iovec* ldata,
                         int iovcnt, int stream, int ppid, MPIU_Size_t* nb);
int MPIDU_Sctp_writev(MPIDI_VC_t* vc, struct iovec* data,int iovcnt, 
		      int stream, int ppid, MPIU_Size_t* nb);
int MPIDU_Sctp_write(MPIDI_VC_t* vc, void* buf, MPIU_Size_t len, 
		     int stream_no, int ppid, MPIU_Size_t* num_written);

int readv_from_advbuf(MPID_Request* req, char* from, int bytes_read);

inline int MPIDU_Sctp_post_writev(MPIDI_VC_t* vc, MPID_Request* sreq, int offset,
				  MPIDU_Sock_progress_update_func_t fn, int stream_no);

inline int MPIDU_Sctp_post_write(MPIDI_VC_t* vc, MPID_Request* sreq, MPIU_Size_t minlen, 
			  MPIU_Size_t maxlen, MPIDU_Sock_progress_update_func_t fn, int stream_no);

/* duplicate static existed so made this non-static */
int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb);
/* used in ch3_init.c and ch3_progress.c . sock equivalents in util/sock */
int MPIDU_Sctp_wait(int fd, int timeout, MPIDU_Sctp_event_t * event);
int MPIDI_CH3I_Progress_handle_sctp_event(MPIDU_Sctp_event_t * event);
int MPIDI_CH3U_Get_business_card_sctp(char **value, int *length);
int MPIDU_Sctp_get_conninfo_from_bc( const char *bc, 
				     char *host_description, int maxlen,
				     int *port, MPIDU_Sock_ifaddr_t *ifaddr, 
				     int *hasIfaddr );


/* BUFFER management routines/structs */

#define READ_AMOUNT 262144

typedef struct my_buffer {
  int free_space; /* free space */
  char buffer[READ_AMOUNT];
  char* buf_ptr;
  struct my_buffer* next; /* if we ever chain bufferNode */
  char dynamic;
} BufferNode_t;

inline void BufferList_init(BufferNode_t* node);
int inline buf_init(BufferNode_t* node);
int inline buf_clean(BufferNode_t* node);
inline char* request_buffer(int size, BufferNode_t** node);
void inline update_size(BufferNode_t* node, int size);
#define remain_size(x) x-> free_space


/* END BUFFER management */

/* FIXME: Any of these used in the ch3->channel interface should be
   defined in a header file in ch3/include that defines the 
   channel interface */
int MPIDI_CH3I_Progress_init(int pg_size);
int MPIDI_CH3I_Progress_finalize(void);
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t *);

/*  global sendQ stuff */
typedef struct gb_sendQ_head {
    struct MPID_Request* head;
    struct MPID_Request* tail;
    int count;
} GLB_SendQ_Head;

extern GLB_SendQ_Head Global_SendQ;

/*  global sendQ enqueue */

/*  malloc is not needed, because ch portion of the REQ has all the info 
 *   we need: vc, stream */

#define Global_SendQ_enqueue(vc, req, x) {               \
        req->ch.vc = vc;                        \
        req->ch.stream = x;                \
        SEND_ACTIVE(vc, x) = req;\
        req->dev.next = NULL;\
	if(Global_SendQ.head == NULL) {	\
		Global_SendQ.head = req;	\
                Global_SendQ.tail = req;  \
	} else {				\
		Global_SendQ.tail->dev.next = req;	\
		Global_SendQ.tail = req;		\
	}					\
	Global_SendQ.count++;			\
}

/*  init global sendQ */
#define Global_SendQ_init() {                   \
	Global_SendQ.count = 0;			\
	Global_SendQ.head = NULL;		\
	Global_SendQ.tail = Global_SendQ.head;	\
}

#define Global_SendQ_head() (Global_SendQ.head)
#define Global_SendQ_count() (Global_SendQ.count)

/*  dequeue + pop */
#define Global_SendQ_dequeue(x) {                                       \
	if(Global_SendQ.count) {					\
	        x = Global_SendQ.head;                               \
		Global_SendQ.head = Global_SendQ.head->dev.next;	\
		Global_SendQ.count--;					\
	}                                                               \
	if(!Global_SendQ.count) {					\
		Global_SendQ.tail = Global_SendQ.head;	         	\
	}								\
}

/* PERFORMANCE stuff */

#define PERF_MEASURE

typedef struct perf_struct {
    int wait_count;
    int event_q_size;
} Performance_t;

/* END PERFORMANCE stuff */


/* Connection setup OPT */
#define CONNECT_THRESHOLD 2


/* Memory copy optimization */

/* Nemesis memcpy opt. can go here */
#define MEMCPY(to, from, nb) memcpy(to, from, nb)

#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */
