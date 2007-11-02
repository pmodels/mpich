/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDIMPL_H_INCLUDED)
#define MPICH_MPIDIMPL_H_INCLUDED

#if defined(HAVE_ASSERT_H)
#include <assert.h>
#endif

#include "mpiimpl.h"

#define MPIDI_IOV_DENSITY_MIN 128

#warning DARIUS CH3 is doing rendezvous
#define CH3_RNDV 1

typedef struct MPIDI_Process
{
    MPID_Request * recvq_posted_head;
    MPID_Request * recvq_posted_tail;
    MPID_Request * recvq_unexpected_head;
    MPID_Request * recvq_unexpected_tail;
#if !defined(MPICH_SINGLE_THREADED)
    MPID_Thread_lock_t recvq_mutex;
#endif
    char * processor_name;
}
MPIDI_Process_t;

extern MPIDI_Process_t MPIDI_Process;

/*
 * VC utility macros
 */
#if defined(MPID_USE_SEQUENCE_NUMBERS)
#define MPIDI_CH3U_VC_init_seqnum_send(_vc)	\
{						\
    (_vc)->seqnum_send = 0;			\
}
#else
#define MPIDI_CH3U_VC_init_seqnum_send(_vc)
#endif

#if defined(MPIDI_CH3_MSGS_UNORDERED)
#define MPIDI_CH3U_VC_init_seqnum_recv(_vc);	\
{						\
    (_vc)->seqnum_recv = 0;			\
    (_vc)->msg_reorder_queue = NULL;		\
}
#else
#define MPIDI_CH3U_VC_init_seqnum_recv(_vc);
#endif

#define MPIDI_CH3U_VC_init(_vc, _lpid)		\
{						\
    MPIU_Object_set_ref((_vc), 0);		\
    (_vc)->lpid = (_lpid);			\
    MPIDI_CH3U_VC_init_seqnum_send(_vc);	\
    MPIDI_CH3U_VC_init_seqnum_recv(_vc);	\
}


/*
 * Datatype Utility Macros (internal - do not use in MPID macros)
 */
#define MPIDI_CH3U_Datatype_get_info(_count, _datatype, _dt_contig_out, _data_sz_out, _dt_ptr)				\
{															\
    if (HANDLE_GET_KIND(_datatype) == HANDLE_KIND_BUILTIN)								\
    {															\
	(_dt_ptr) = NULL;												\
	(_dt_contig_out) = TRUE;											\
	(_data_sz_out) = (_count) * MPID_Datatype_get_basic_size(_datatype);						\
	MPIDI_DBG_PRINTF((15, FCNAME, "basic datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT,		\
			  (_dt_contig_out), MPID_Datatype_get_basic_size(_datatype), (_data_sz_out)));			\
    }															\
    else														\
    {															\
	MPID_Datatype_get_ptr((_datatype), (_dt_ptr));									\
	(_dt_contig_out) = (_dt_ptr)->is_contig;									\
	(_data_sz_out) = (_count) * (_dt_ptr)->size;									\
	MPIDI_DBG_PRINTF((15, FCNAME, "user defined datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT,	\
			  (_dt_contig_out), (_dt_ptr)->size, (_data_sz_out)));						\
    }															\
}

/*
 * Request utility macros (internal - do not use in MPID macros)
 */
#define MPIDI_CH3U_Request_create(_req)				\
{								\
    MPID_Request_construct(_req);				\
    MPIU_Object_set_ref((_req), 1);				\
    (_req)->cc = 1;						\
    (_req)->cc_ptr = &(_req)->cc;				\
    (_req)->status.MPI_SOURCE = MPI_UNDEFINED;			\
    (_req)->status.MPI_TAG = MPI_UNDEFINED;			\
    (_req)->status.MPI_ERROR = MPI_SUCCESS;			\
    (_req)->status.count = 0;					\
    (_req)->status.cancelled = FALSE;				\
    (_req)->comm = NULL;					\
    (_req)->dev.datatype_ptr = NULL;				\
    MPIDI_Request_state_init((_req));				\
    (_req)->dev.cancel_pending = FALSE;				\
    (_req)->dev.decr_ctr = NULL;				\
    (_req)->dev.dtype_info = NULL;				\
    (_req)->dev.dataloop = NULL;				\
}

#define MPIDI_CH3U_Request_destroy(_req)			\
{								\
    if ((_req)->comm != NULL)					\
    {								\
	MPIR_Comm_release((_req)->comm);			\
    }								\
								\
    if ((_req)->dev.datatype_ptr != NULL)			\
    {								\
	MPID_Datatype_release((_req)->dev.datatype_ptr);	\
    }								\
								\
    if (MPIDI_Request_get_srbuf_flag(_req))			\
    {								\
	MPIDI_CH3U_SRBuf_free(_req);				\
    }								\
								\
    MPID_Request_destruct(_req);				\
}

/* FIXME: MT: The reference count is normally set to two, one for the user and one for the device and channel.  The device and
 * channel should really be separated since the device may have completed the request and thus be done with it; but, the channel
 * may still need it (to check if dev.iov_count is zero).  Right now the request is being referenced by the progress engine after
 * the MPIDI_CH3U_Request_complete() is called, thus using the request after it may have been freed. */
#define MPIDI_CH3U_Request_complete(req_)			\
{								\
    int incomplete__;						\
								\
    MPIDI_CH3U_Request_decrement_cc((req_), &incomplete__);	\
    if (!incomplete__)						\
    {								\
	MPID_Request_release(req_);				\
	MPIDI_CH3_Progress_signal_completion();			\
    }								\
}

#define MPIDI_CH3M_create_sreq(_sreq, _mpi_errno, _FAIL)			\
{										\
    (_sreq) = MPIDI_CH3_Request_create();					\
    if ((_sreq) == NULL)							\
    {										\
	MPIDI_DBG_PRINTF((15, FCNAME, "send request allocation failed"));	\
	(_mpi_errno) = MPIR_ERR_MEMALLOCFAILED;					\
	_FAIL;									\
    }										\
    										\
    MPIU_Object_set_ref((_sreq), 2);						\
    (_sreq)->kind = MPID_REQUEST_SEND;						\
    (_sreq)->comm = comm;							\
    MPIR_Comm_add_ref(comm);							\
    (_sreq)->dev.match.rank = rank;						\
    (_sreq)->dev.match.tag = tag;						\
    (_sreq)->dev.match.context_id = comm->context_id + context_offset;		\
    (_sreq)->dev.user_buf = (void *) buf;					\
    (_sreq)->dev.user_count = count;						\
    (_sreq)->dev.datatype = datatype;						\
}

#define MPIDI_CH3M_create_psreq(_sreq, _mpi_errno, _FAIL)			\
{										\
    (_sreq) = MPIDI_CH3_Request_create();					\
    if ((_sreq) == NULL)							\
    {										\
	MPIDI_DBG_PRINTF((15, FCNAME, "send request allocation failed"));	\
	(_mpi_errno) = MPIR_ERR_MEMALLOCFAILED;					\
	_FAIL;									\
    }										\
										\
    MPIU_Object_set_ref((_sreq), 1);						\
    (_sreq)->kind = MPID_PREQUEST_SEND;						\
    (_sreq)->comm = comm;							\
    MPIR_Comm_add_ref(comm);							\
    (_sreq)->dev.match.rank = rank;						\
    (_sreq)->dev.match.tag = tag;						\
    (_sreq)->dev.match.context_id = comm->context_id + context_offset;		\
    (_sreq)->dev.user_buf = (void *) buf;					\
    (_sreq)->dev.user_count = count;						\
    (_sreq)->dev.datatype = datatype;						\
    (_sreq)->partner_request = NULL;						\
}

/* Masks and flags for channel device state in an MPID_Request */
#define MPIDI_Request_state_init(_req)		\
{						\
    (_req)->dev.state = 0;			\
}

#define MPIDI_REQUEST_MSG_MASK (0x3 << MPIDI_REQUEST_MSG_SHIFT)
#define MPIDI_REQUEST_MSG_SHIFT 0
#define MPIDI_REQUEST_NO_MSG 0
#define MPIDI_REQUEST_EAGER_MSG 1
#define MPIDI_REQUEST_RNDV_MSG 2
#define MPIDI_REQUEST_SELF_MSG 3

#define MPIDI_Request_get_msg_type(_req)					\
(((_req)->dev.state & MPIDI_REQUEST_MSG_MASK) >> MPIDI_REQUEST_MSG_SHIFT)

#define MPIDI_Request_set_msg_type(_req, _msgtype)						\
{												\
    (_req)->dev.state &= ~MPIDI_REQUEST_MSG_MASK;						\
    (_req)->dev.state |= ((_msgtype) << MPIDI_REQUEST_MSG_SHIFT) & MPIDI_REQUEST_MSG_MASK;	\
}

#define MPIDI_REQUEST_SRBUF_MASK (0x1 << MPIDI_REQUEST_SRBUF_SHIFT)
#define MPIDI_REQUEST_SRBUF_SHIFT 2

#define MPIDI_Request_get_srbuf_flag(_req)					\
(((_req)->dev.state & MPIDI_REQUEST_SRBUF_MASK) >> MPIDI_REQUEST_SRBUF_SHIFT)

#define MPIDI_Request_set_srbuf_flag(_req, _flag)						\
{												\
    (_req)->dev.state &= ~MPIDI_REQUEST_SRBUF_MASK;						\
    (_req)->dev.state |= ((_flag) << MPIDI_REQUEST_SRBUF_SHIFT) & MPIDI_REQUEST_SRBUF_MASK;	\
}

#define MPIDI_REQUEST_SYNC_SEND_MASK (0x1 << MPIDI_REQUEST_SYNC_SEND_SHIFT)
#define MPIDI_REQUEST_SYNC_SEND_SHIFT 3

#define MPIDI_Request_get_sync_send_flag(_req)						\
(((_req)->dev.state & MPIDI_REQUEST_SYNC_SEND_MASK) >> MPIDI_REQUEST_SYNC_SEND_SHIFT)

#define MPIDI_Request_set_sync_send_flag(_req, _flag)							\
{													\
    (_req)->dev.state &= ~MPIDI_REQUEST_SYNC_SEND_MASK;							\
    (_req)->dev.state |= ((_flag) << MPIDI_REQUEST_SYNC_SEND_SHIFT) & MPIDI_REQUEST_SYNC_SEND_MASK;	\
}

#define MPIDI_REQUEST_TYPE_MASK (0xF << MPIDI_REQUEST_TYPE_SHIFT)
#define MPIDI_REQUEST_TYPE_SHIFT 4
#define MPIDI_REQUEST_TYPE_RECV 0
#define MPIDI_REQUEST_TYPE_SEND 1
#define MPIDI_REQUEST_TYPE_RSEND 2
#define MPIDI_REQUEST_TYPE_SSEND 3
/* We need a BSEND type for persistent bsends (see mpid_startall.c) */
#define MPIDI_REQUEST_TYPE_BSEND 4
#define MPIDI_REQUEST_TYPE_PUT_RESP 5
#define MPIDI_REQUEST_TYPE_GET_RESP 6
#define MPIDI_REQUEST_TYPE_ACCUM_RESP 7
#define MPIDI_REQUEST_TYPE_PUT_RESP_DERIVED_DT 8
#define MPIDI_REQUEST_TYPE_GET_RESP_DERIVED_DT 9
#define MPIDI_REQUEST_TYPE_ACCUM_RESP_DERIVED_DT 10

#define MPIDI_Request_get_type(_req)						\
(((_req)->dev.state & MPIDI_REQUEST_TYPE_MASK) >> MPIDI_REQUEST_TYPE_SHIFT)

#define MPIDI_Request_set_type(_req, _type)							\
{												\
    (_req)->dev.state &= ~MPIDI_REQUEST_TYPE_MASK;						\
    (_req)->dev.state |= ((_type) << MPIDI_REQUEST_TYPE_SHIFT) & MPIDI_REQUEST_TYPE_MASK;	\
}

#if defined(MPICH_SINGLE_THREADED)
#define MPIDI_Request_cancel_pending(_req, _flag)	\
{							\
    *(_flag) = (_req)->dev.cancel_pending;		\
    (_req)->dev.cancel_pending = TRUE;			\
}
#else
/* MT: to make this code lock free, an atomic exchange can be used. */ 
#define MPIDI_Request_cancel_pending(_req, _flag)	\
{							\
    MPID_Request_thread_lock(_req);			\
    {							\
	*(_flag) = (_req)->dev.cancel_pending;		\
	(_req)->dev.cancel_pending = TRUE;		\
    }							\
    MPID_Request_thread_unlock(_req);			\
}
#endif

#if defined(MPICH_SINGLE_THREADED)
#define MPIDI_Request_recv_pending(req_, recv_pending_)		\
{								\
    *(recv_pending_) = --(req_)->dev.recv_pending_count;	\
}
#else
#if defined(USE_ATOMIC_UPDATES)
#define MPIDI_Request_recv_pending(req_, recv_pending_)				\
{										\
    int recv_pending__;								\
    										\
    MPID_Atomic_decr_flag(&(req_)->dev.recv_pending_count, recv_pending__);	\
    *(recv_pending_) = recv_pending__;						\
}
#else
#define MPIDI_Request_recv_pending(req_, recv_pending_)		\
{								\
    MPID_Request_thread_lock(req_);				\
    {								\
	*(recv_pending_) = --(req_)->dev.recv_pending_count;	\
    }								\
    MPID_Request_thread_unlock(req_);				\
}
#endif /* defined(USE_ATOMIC_UPDATES) */
#endif /* defined(MPICH_SINGLE_THREADED) */

/* MPIDI_Request_fetch_and_clear_rts_sreq() - atomically fetch current partner RTS sreq and nullify partner request */
#if defined(MPICH_SINGLE_THREADED)
#define MPIDI_Request_fetch_and_clear_rts_sreq(sreq_, rts_sreq_)	\
{									\
    *(rts_sreq_) = (sreq_)->partner_request;				\
    (sreq_)->partner_request = NULL;					\
}
#else
/* MT: to make this code lock free, an atomic exchange can be used. */
#define MPIDI_Request_fetch_and_clear_rts_sreq(sreq_, rts_sreq_)	\
{									\
    MPID_Request_thread_lock(sreq_);					\
    {									\
	*(rts_sreq_) = (sreq_)->partner_request;			\
	(sreq_)->partner_request = NULL;				\
    }									\
    MPID_Request_thread_unlock(sreq_);					\
}
#endif


/*
 * Send/Receive buffer macros
 */
#if !defined(MPIDI_CH3U_SRBuf_size)
#define MPIDI_CH3U_SRBuf_size (16384)
#endif

#if !defined(MPIDI_CH3U_SRBuf_alloc)
#define MPIDI_CH3U_SRBuf_alloc(_req, _size)			\
{								\
    (_req)->dev.tmpbuf = MPIU_Malloc(MPIDI_CH3U_SRBuf_size);	\
    if ((_req)->dev.tmpbuf != NULL)				\
    {								\
	(_req)->dev.tmpbuf_sz = MPIDI_CH3U_SRBuf_size;		\
	MPIDI_Request_set_srbuf_flag((_req), TRUE);		\
    }								\
    else							\
    {								\
	(_req)->dev.tmpbuf_sz = 0;				\
    }								\
}
#endif

#if !defined(MPIDI_CH3U_SRBuf_free)
#define MPIDI_CH3U_SRBuf_free(_req)			\
{							\
    assert(MPIDI_Request_get_srbuf_flag(_req));		\
    MPIDI_Request_set_srbuf_flag((_req), FALSE);	\
    MPIU_Free((_req)->dev.tmpbuf);			\
}
#endif


/*
 * Sequence number related macros (internal)
 */
#if defined(MPID_USE_SEQUENCE_NUMBERS)
#if defined(MPICH_SINGLE_THREADED)
#define MPIDI_CH3U_VC_FAI_send_seqnum(_vc, _seqnum_out)	\
{							\
    (_seqnum_out) = (_vc)->seqnum_send++;		\
}
#else
#if defined(USE_ATOMIC_UPDATES)
#define MPIDI_CH3U_VC_FAI_send_seqnum(_vc, _seqnum_out)			\
{									\
    MPID_Atomic_fetch_and_incr(&(_vc)->seqnum_send, (_seqnum_out));	\
}
#else
/* FIXME: a VC specific mutex could be used if contention is a problem. */
#define MPIDI_CH3U_VC_FAI_send_seqnum(_vc, _seqnum_out)	\
{							\
    MPID_Common_thread_lock();				\
    {							\
	(_seqnum_out) = (_vc)->seqnum_send++;		\
    }							\
    MPID_Common_thread_unlock();			\
}
#endif /* defined(USE_ATOMIC_UPDATES) */
#endif
#define MPIDI_CH3U_Request_set_seqnum(_req, _seqnum)	\
{							\
    (_req)->dev.seqnum = (_seqnum);			\
}
#define MPIDI_CH3U_Pkt_set_seqnum(_pkt, _seqnum)	\
{							\
    (_pkt)->seqnum = (_seqnum);				\
}
#else
#define MPIDI_CH3U_VC_FAI_send_seqnum(_vc, _seqnum_out)
#define MPIDI_CH3U_Request_set_seqnum(_req, _seqnum)
#define MPIDI_CH3U_Pkt_set_seqnum(_pkt, _seqnum)
#endif


/*
 * Debugging tools
 */
void MPIDI_dbg_printf(int, char *, char *, ...);
void MPIDI_err_printf(char *, char *, ...);

#if defined(MPICH_DBG_OUTPUT)
#define MPIDI_DBG_PRINTF(_e)				\
{                                               	\
    if (MPIUI_dbg_state != MPIU_DBG_STATE_NONE)		\
    {							\
	MPIDI_dbg_printf _e;				\
    }							\
}
#else
#define MPIDI_DBG_PRINTF(e)
#endif

#define MPIDI_ERR_PRINTF(e) MPIDI_err_printf e

#if defined(HAVE_CPP_VARARGS)
#define MPIDI_dbg_printf(level, func, fmt, args...)						\
{												\
    MPIU_dbglog_printf("[%d] %s(): " fmt "\n", MPIR_Process.comm_world->rank, func, ## args);	\
}
#define MPIDI_err_printf(func, fmt, args...)							\
{												\
    MPIU_Error_printf("[%d] ERROR - %s(): " fmt "\n", MPIR_Process.comm_world->rank, func, ## args);	\
    fflush(stdout);										\
}
#endif

#define MPIDI_QUOTE(A) MPIDI_QUOTE2(A)
#define MPIDI_QUOTE2(A) #A

/* Prototypes for internal device routines */
int MPIDI_Isend_self(const void *, int, MPI_Datatype, int, int, MPID_Comm *, int, int, MPID_Request **);


/* Prototypes for collective operations supplied by the device (or channel) */
int MPIDI_Barrier(MPID_Comm *);

#ifdef MPICH_DBG_OUTPUT
void MPIDI_DBG_Print_packet(MPIDI_CH3_Pkt_t *pkt);
#else
#define MPIDI_DBG_Print_packet(a)
#endif

/* ------------------------------------------------------------------------- */
/* mpirma.h (in src/mpi/rma?) */
/* ------------------------------------------------------------------------- */
typedef struct MPIDI_RMA_ops { 
/* for keeping track of puts and gets, which will be executed at fence */
    struct MPIDI_RMA_ops *next;  /* pointer to next element in list */
    int type;  /* MPIDI_RMA_PUT, MPID_REQUEST_GET,
                  MPIDI_RMA_ACCUMULATE */  
    void *origin_addr;
    int origin_count;
    MPI_Datatype origin_datatype;
    int target_rank;
    MPI_Aint target_disp;
    int target_count;
    MPI_Datatype target_datatype;
    MPI_Op op;  /* for accumulate */
    int lock_type;  /* for win_lock */
} MPIDI_RMA_ops;

#define MPIDI_RMA_PUT 23
#define MPIDI_RMA_GET 24
#define MPIDI_RMA_ACCUMULATE 25
#define MPIDI_RMA_LOCK 26
#define MPIDI_RMA_DATATYPE_BASIC 50
#define MPIDI_RMA_DATATYPE_DERIVED 51

extern MPIDI_RMA_ops *MPIDI_RMA_ops_list; /* list of outstanding RMA requests */

int MPIDI_CH3I_Send_rma_msg(MPIDI_RMA_ops *rma_op, MPID_Win *win_ptr,
                            int *decr_addr, MPIDI_RMA_dtype_info
                            *dtype_info, void **dataloop, MPID_Request
                            **request);

int MPIDI_CH3I_Recv_rma_msg(MPIDI_RMA_ops *rma_op, MPID_Win *win_ptr,
                            int *decr_addr, MPIDI_RMA_dtype_info
                            *dtype_info, void **dataloop, MPID_Request
                            **request); 

/* NOTE: Channel function prototypes are in mpidi_ch3_post.h since some of the macros require their declarations. */

#endif /* !defined(MPICH_MPIDIMPL_H_INCLUDED) */
