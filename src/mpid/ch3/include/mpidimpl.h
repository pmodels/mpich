/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * WARNING: Functions and macros in this file are for internal use only.  
 * As such, they are only visible to the device and
 * channel.  Do not include them in the MPID macros.
 */

#if !defined(MPICH_MPIDIMPL_H_INCLUDED)
#define MPICH_MPIDIMPL_H_INCLUDED

#include "mpichconf.h"

#if defined(HAVE_ASSERT_H)
#include <assert.h>
#endif

#include "mpiimpl.h"

#if !defined(MPICH_MPIDPRE_H_INCLUDED)
#include "mpidpre.h"
#endif

#include "mpidftb.h"

/* Add the ch3 packet definitions */
#include "mpidpkt.h"

/* We need to match the size of MPIR_Pint to the relevant Format control
 */
#define MPIDI_MSG_SZ_FMT MPIR_PINT_FMT_DEC_SPEC

#if !defined(MPIDI_IOV_DENSITY_MIN)
#   define MPIDI_IOV_DENSITY_MIN (16 * 1024)
#endif

#if defined(HAVE_GETHOSTNAME) && defined(NEEDS_GETHOSTNAME_DECL) && \
   !defined(gethostname)
int gethostname(char *name, size_t len);
# endif

extern int MPIDI_Use_pmi2_api;

#define MPIDI_CHANGE_VC_STATE(vc, new_state) do {               \
        MPIU_DBG_VCSTATECHANGE(vc, VC_STATE_##new_state);       \
        (vc)->state = MPIDI_VC_STATE_##new_state;               \
    } while (0)

/*S
  MPIDI_PG_t - Process group description

  Notes:
  Every 'MPI_COMM_WORLD' known to this process has an associated process 
  group.  
  S*/
typedef struct MPIDI_PG
{
    /* MPIU_Object field.  MPIDI_PG_t objects are not allocated using the 
       MPIU_Object system, but we do use the associated reference counting 
       routines.  Therefore, handle must be present, but is not used 
       except by debugging routines */
    MPIU_OBJECT_HEADER; /* adds handle and ref_count fields */

    /* Next pointer used to maintain a list of all process groups known to 
       this process */
    struct MPIDI_PG * next;

    /* Number of processes in the process group */
    int size;

    /* VC table.  At present this is a pointer to an array of VC structures. 
       Someday we may want make this a pointer to an array
       of VC references.  Thus, it is important to use MPIDI_PG_Get_vc() 
       instead of directly referencing this field. */
    struct MPIDI_VC * vct;

    /* Pointer to the process group ID.  The actual ID is defined and 
       allocated by the process group.  The pointer is kept in the
       device space because it is necessary for the device to be able to 
       find a particular process group. */
    void * id;

    /* Replacement abstraction for connection information */
    /* Connection information needed to access processes in this process 
       group and to share the data with other processes.  The items are
       connData - pointer for data used to implement these functions 
                  (e.g., a pointer to an array of process group info)
       getConnInfo( rank, buf, bufsize, self ) - function to store into
                  buf the connection information for rank in this process 
                  group
       connInfoToString( buf_p, size, self ) - return in buf_p a string
                  that can be sent to another process to recreate the
                  connection information (the info needed to support
                  getConnInfo)
       connInfoFromString( buf, self ) - setup the information needed
                  to implement getConnInfo
       freeConnInfo( self ) - free any storage or resources associated
                  with the connection information.

       See ch3/src/mpidi_pg.c 
    */
    void *connData;
    int  (*getConnInfo)( int, char *, int, struct MPIDI_PG * );
    int  (*connInfoToString)( char **, int *, struct MPIDI_PG * );
    int  (*connInfoFromString)( const char *,  struct MPIDI_PG * );
    int  (*freeConnInfo)( struct MPIDI_PG * );

    /* Rather than have each channel define its own fields for the 
       channel-specific data, we provide a fixed-sized scratchpad.  Currently,
       this has a very generous size, though this may shrink later (a channel
       can always allocate storage and hang it off of the end).  This 
       is necessary to allow dynamic loading of channels at MPI_Init time. */
#define MPIDI_CH3_PG_SIZE 48
    int32_t channel_private[MPIDI_CH3_PG_SIZE];
#if defined(MPIDI_CH3_PG_DECL)
    MPIDI_CH3_PG_DECL
#endif    
}
MPIDI_PG_t;


/*S
  MPIDI_Process_t - The information required about this process by the CH3 
  device.

  S*/
typedef struct MPIDI_Process
{
    MPIDI_PG_t * my_pg;
    int my_pg_rank;
}
MPIDI_Process_t;

extern MPIDI_Process_t MPIDI_Process;

/*----------------------
  BEGIN DATATYPE SECTION
  ----------------------*/
/* FIXME: We want to avoid even storing information about the builtins
   if we can */
#define MPIDI_Datatype_get_info(count_, datatype_, dt_contig_out_, data_sz_out_, dt_ptr_, dt_true_lb_)\
{									\
    if (HANDLE_GET_KIND(datatype_) == HANDLE_KIND_BUILTIN)		\
    {									\
	(dt_ptr_) = NULL;						\
	(dt_contig_out_) = TRUE;					\
        (dt_true_lb_)    = 0;                                           \
	(data_sz_out_) = (MPIDI_msg_sz_t) (count_) * MPID_Datatype_get_basic_size(datatype_); \
	MPIDI_DBG_PRINTF((15, FCNAME, "basic datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT,\
			  (dt_contig_out_), MPID_Datatype_get_basic_size(datatype_), (data_sz_out_)));\
    }									\
    else								\
    {									\
	MPID_Datatype_get_ptr((datatype_), (dt_ptr_));			\
	(dt_contig_out_) = (dt_ptr_)->is_contig;			\
	(data_sz_out_) = (MPIDI_msg_sz_t) (count_) * (dt_ptr_)->size;	\
        (dt_true_lb_)    = (dt_ptr_)->true_lb;                          \
	MPIDI_DBG_PRINTF((15, FCNAME, "user defined datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT,\
			  (dt_contig_out_), (dt_ptr_)->size, (data_sz_out_)));\
    }									\
}
/*--------------------
  END DATATYPE SECTION
  --------------------*/


/*---------------------
  BEGIN REQUEST SECTION
  ---------------------*/

/*
 * MPID_Requests
 *
 * MPI Requests are handles to MPID_Request structures.  These are used
 * for most communication operations to provide a uniform way in which to
 * define pending operations.  As such, they contain many fields that are 
 * only used by some operations (logically, an MPID_Request is a union type).
 *
 * There are several kinds of requests.  They are
 *    Send, Receive, RMA, User, Persistent
 * In addition, send and RMA requests may be "incomplete"; this means that
 * they have not sent their initial packet, and they may store additional
 * data about the operation that will be used when the initial packet 
 * can be sent.
 *
 * Also, requests that are used internally within blocking MPI routines
 * (only Send and Receive requests) do not require references to
 * (or increments of the reference counts) communicators or datatypes.
 * Thus, freeing these requests also does not require testing or 
 * decrementing these fields.
 *
 * Finally, we want to avoid multiple tests for a failure to allocate
 * a request.  Thus, the request allocation macros will jump to fn_fail
 * if there is an error.  This is akin to using a "throw" in C++.
 * 
 * For example, a posted (unmatched) receive queue entry needs only:
 *     match info
 *     buffer info (address, count, datatype)
 *     if nonblocking, communicator (used for finding error handler)
 *     if nonblocking, cancelled state
 * Once matched, a receive queue entry also needs
 *     actual match info
 *     message type (eager, rndv, eager-sync)
 *     completion state (is all data available)
 *        If destination datatype is non-contiguous, it also needs
 *        current unpack state.
 * An unexpected message (in the unexpected receive queue) needs only:
 *     match info
 *     message type (eager, rndv, eager-sync)
 *     if (eager, eager-sync), data
 *     completion state (is all data available?)
 * A send request requires only
 *     message type (eager, rndv, eager-sync)
 *     completion state (has all data been sent?)
 *     canceled state
 *     if nonblocking, communicator (used for finding error handler)
 *     if the initial envelope is still pending (e.g., could not write yet)
 *         match info
 *     if the data is still pending (rndv or would not send eager)
 *         buffer info (address, count, datatype)
 * RMA requests require (what)?
 * User (generalized) requests require
 *     function pointers for operations
 *     completion state
 *     cancelled state
 */

/* FIXME XXX DJG for TLS hack */
#define MPID_REQUEST_TLS_MAX 128

#if MPIU_HANDLE_ALLOCATION_METHOD == MPIU_HANDLE_ALLOCATION_THREAD_LOCAL
#  define MPIDI_Request_tls_alloc(req) \
    do { \
        int i;                                                         \
        MPIU_THREADPRIV_DECL;                                          \
        MPIU_THREADPRIV_GET;                                           \
        if (!MPIU_THREADPRIV_FIELD(request_handles)) {                 \
            MPID_Request *prev, *cur;                                  \
            /* batch allocate a linked list of requests */             \
            MPIU_THREAD_CS_ENTER(HANDLEALLOC,);                        \
            prev = MPIU_Handle_obj_alloc_unsafe(&MPID_Request_mem);    \
            prev->next = NULL;                                         \
            assert(prev);                                              \
            for (i = 1; i < MPID_REQUEST_TLS_MAX; ++i) {               \
                cur = MPIU_Handle_obj_alloc_unsafe(&MPID_Request_mem); \
                assert(cur);                                           \
                cur->next = prev;                                      \
                prev = cur;                                            \
            }                                                          \
            MPIU_THREAD_CS_EXIT(HANDLEALLOC,);                         \
            MPIU_THREADPRIV_FIELD(request_handles) = cur;              \
            MPIU_THREADPRIV_FIELD(request_handle_count) += MPID_REQUEST_TLS_MAX;    \
        }                                                              \
        (req) = MPIU_THREADPRIV_FIELD(request_handles);                \
        MPIU_THREADPRIV_FIELD(request_handles) = req->next;            \
        MPIU_THREADPRIV_FIELD(request_handle_count) -= 1;              \
    } while (0)
#elif MPIU_HANDLE_ALLOCATION_METHOD == MPIU_HANDLE_ALLOCATION_MUTEX
#  define MPIDI_Request_tls_alloc(req_) \
    do { \
	(req_) = MPIU_Handle_obj_alloc(&MPID_Request_mem); \
        MPIU_DBG_MSG_P(CH3_CHANNEL,VERBOSE,		\
	       "allocated request, handle=0x%08x", req_);\
    } while (0)
#else
#  error MPIU_HANDLE_ALLOCATION_METHOD not defined
#endif

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


/* If the channel doesn't initialize anything in the request, 
   provide a dummy */
#ifndef MPIDI_CH3_REQUEST_INIT
#define MPIDI_CH3_REQUEST_INIT(a_)
#endif

/* FIXME: Why does a send request need the match information?
   Is that for debugging information?  In case the initial envelope
   cannot be sent? Ditto for the dev.user_buf, count, and datatype 
   fields when the data is sent eagerly.  

   The following fields needed to be set:
   datatype_ptr
   status.MPI_ERROR

   Note that this macro requires that rank, tag, context_offset,
   comm, buf, datatype, and count all be available with those names
   (they are not arguments to the routine)
*/
#define MPIDI_Request_create_sreq(sreq_, mpi_errno_, FAIL_)	\
{								\
    (sreq_) = MPIU_Handle_obj_alloc(&MPID_Request_mem);         \
    if ((sreq_) == NULL)					\
    {								\
	MPIU_DBG_MSG(CH3_CHANNEL,TYPICAL,"unable to allocate a request");\
	(mpi_errno_) = MPIR_ERR_MEMALLOCFAILED;			\
	FAIL_;							\
    }								\
    MPIU_DBG_MSG_P(CH3_CHANNEL,VERBOSE,                         \
	       "allocated request, handle=0x%08x", (sreq_)->handle);\
    								\
    MPIU_Object_set_ref((sreq_), 2);				\
    (sreq_)->kind = MPID_REQUEST_SEND;				\
    (sreq_)->comm = comm;					\
    MPID_cc_set(&(sreq_)->cc, 1);                               \
    (sreq_)->cc_ptr		   = &(sreq_)->cc;              \
    (sreq_)->partner_request   = NULL;                          \
    MPIR_Comm_add_ref(comm);					\
    (sreq_)->status.MPI_ERROR	   = MPI_SUCCESS;               \
    (sreq_)->status.cancelled	   = FALSE;		        \
    (sreq_)->dev.state = 0;                                     \
    (sreq_)->dev.cancel_pending = FALSE;                        \
    (sreq_)->dev.match.parts.rank = rank;			\
    (sreq_)->dev.match.parts.tag = tag;				\
    (sreq_)->dev.match.parts.context_id = comm->context_id + context_offset;	\
    (sreq_)->dev.user_buf = (void *) buf;			\
    (sreq_)->dev.user_count = count;				\
    (sreq_)->dev.datatype = datatype;				\
    (sreq_)->dev.datatype_ptr	   = NULL;                      \
    (sreq_)->dev.segment_ptr	   = NULL;                      \
    (sreq_)->dev.OnDataAvail	   = NULL;                      \
    (sreq_)->dev.OnFinal	   = NULL;                      \
    (sreq_)->dev.iov_count	   = 0;                         \
    (sreq_)->dev.iov_offset	   = 0;                         \
}

/* This is the receive request version of MPIDI_Request_create_sreq */
#define MPIDI_Request_create_rreq(rreq_, mpi_errno_, FAIL_)	\
{								\
    (rreq_) = MPIU_Handle_obj_alloc(&MPID_Request_mem);         \
    if ((rreq_) == NULL)					\
    {								\
	MPIU_DBG_MSG(CH3_CHANNEL,TYPICAL,"unable to allocate a request");\
	(mpi_errno_) = MPIR_ERR_MEMALLOCFAILED;			\
	FAIL_;							\
    }								\
    MPIU_DBG_MSG_P(CH3_CHANNEL,VERBOSE,                         \
	       "allocated request, handle=0x%08x", (rreq_)->handle);\
    								\
    MPIU_Object_set_ref((rreq_), 2);				\
    (rreq_)->kind = MPID_REQUEST_RECV;				\
    (rreq_)->comm = NULL;					\
    MPID_cc_set(&(rreq_)->cc, 1);                               \
    (rreq_)->cc_ptr		   = &(rreq_)->cc;              \
    (rreq_)->status.MPI_ERROR	   = MPI_SUCCESS;               \
    (rreq_)->status.cancelled	   = FALSE;                     \
    (rreq_)->partner_request   = NULL;                          \
    (rreq_)->dev.state = 0;                                     \
    (rreq_)->dev.cancel_pending = FALSE;                        \
    (rreq_)->dev.datatype_ptr = NULL;                           \
    (rreq_)->dev.segment_ptr = NULL;                            \
    (rreq_)->dev.iov_offset   = 0;                              \
    (rreq_)->dev.OnDataAvail	   = NULL;                      \
    (rreq_)->dev.OnFinal	   = NULL;                      \
     MPIDI_CH3_REQUEST_INIT(rreq_);\
}

#define MPIDI_REQUEST_MSG_MASK (0x3 << MPIDI_REQUEST_MSG_SHIFT)
#define MPIDI_REQUEST_MSG_SHIFT 0
#define MPIDI_REQUEST_NO_MSG 0
#define MPIDI_REQUEST_EAGER_MSG 1
#define MPIDI_REQUEST_RNDV_MSG 2
#define MPIDI_REQUEST_SELF_MSG 3

#define MPIDI_Request_get_msg_type(req_)				\
(((req_)->dev.state & MPIDI_REQUEST_MSG_MASK) >> MPIDI_REQUEST_MSG_SHIFT)

#define MPIDI_Request_set_msg_type(req_, msgtype_)			\
{									\
    (req_)->dev.state &= ~MPIDI_REQUEST_MSG_MASK;			\
    (req_)->dev.state |= ((msgtype_) << MPIDI_REQUEST_MSG_SHIFT) & MPIDI_REQUEST_MSG_MASK;\
}

#define MPIDI_REQUEST_SRBUF_MASK (0x1 << MPIDI_REQUEST_SRBUF_SHIFT)
#define MPIDI_REQUEST_SRBUF_SHIFT 2

#define MPIDI_Request_get_srbuf_flag(req_)					\
(((req_)->dev.state & MPIDI_REQUEST_SRBUF_MASK) >> MPIDI_REQUEST_SRBUF_SHIFT)

#define MPIDI_Request_set_srbuf_flag(req_, flag_)			\
{									\
    (req_)->dev.state &= ~MPIDI_REQUEST_SRBUF_MASK;			\
    (req_)->dev.state |= ((flag_) << MPIDI_REQUEST_SRBUF_SHIFT) & MPIDI_REQUEST_SRBUF_MASK;	\
}

#define MPIDI_REQUEST_SYNC_SEND_MASK (0x1 << MPIDI_REQUEST_SYNC_SEND_SHIFT)
#define MPIDI_REQUEST_SYNC_SEND_SHIFT 3

#define MPIDI_Request_get_sync_send_flag(req_)						\
(((req_)->dev.state & MPIDI_REQUEST_SYNC_SEND_MASK) >> MPIDI_REQUEST_SYNC_SEND_SHIFT)

#define MPIDI_Request_set_sync_send_flag(req_, flag_)			\
{									\
    (req_)->dev.state &= ~MPIDI_REQUEST_SYNC_SEND_MASK;			\
    (req_)->dev.state |= ((flag_) << MPIDI_REQUEST_SYNC_SEND_SHIFT) & MPIDI_REQUEST_SYNC_SEND_MASK;\
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
#define MPIDI_REQUEST_TYPE_PT_SINGLE_PUT 11
#define MPIDI_REQUEST_TYPE_PT_SINGLE_ACCUM 12


#define MPIDI_Request_get_type(req_)						\
(((req_)->dev.state & MPIDI_REQUEST_TYPE_MASK) >> MPIDI_REQUEST_TYPE_SHIFT)

#define MPIDI_Request_set_type(req_, type_)				\
{									\
    (req_)->dev.state &= ~MPIDI_REQUEST_TYPE_MASK;			\
    (req_)->dev.state |= ((type_) << MPIDI_REQUEST_TYPE_SHIFT) & MPIDI_REQUEST_TYPE_MASK;\
}

/* NOTE: Request updates may require atomic ops (critical sections) if
   a fine-grain thread-sync model is used. */
#define MPIDI_Request_cancel_pending(req_, flag_)	\
{							\
    *(flag_) = (req_)->dev.cancel_pending;		\
    (req_)->dev.cancel_pending = TRUE;			\
}

/* the following two macros were formerly a single confusing macro with side
   effects named MPIDI_Request_recv_pending() */
#define MPIDI_Request_check_pending(req_, recv_pending_)   \
    do {                                                   \
        *(recv_pending_) = (req_)->dev.recv_pending_count; \
    } while (0)

#define MPIDI_Request_decr_pending(req_)    \
    do {                                    \
        --(req_)->dev.recv_pending_count;   \
    } while (0)

/* MPIDI_Request_fetch_and_clear_rts_sreq() - atomically fetch current 
   partner RTS sreq and nullify partner request */
#define MPIDI_Request_fetch_and_clear_rts_sreq(sreq_, rts_sreq_)	\
    {									\
    	*(rts_sreq_) = (sreq_)->partner_request;			\
    	(sreq_)->partner_request = NULL;				\
    }

/* FIXME: We've moved to allow finer-grain critical sections... */
/* Note: In the current implementation, the mpid_xsend.c routines that
   make use of MPIDI_VC_FAI_send_seqnum are all protected by the 
   SINGLE_CS_ENTER/EXIT macros, so all uses of this macro are 
   alreay within a critical section when needed.  If/when we move to
   a finer-grain model, we'll need to examine whether this requires
   a separate lock. */
#if defined(MPID_USE_SEQUENCE_NUMBERS)
#   define MPIDI_Request_set_seqnum(req_, seqnum_)	\
    {							\
    	(req_)->dev.seqnum = (seqnum_);			\
    }
#   define MPIDI_VC_FAI_send_seqnum(vc_, seqnum_out_)	\
    {							\
	(seqnum_out_) = (vc_)->seqnum_send++;		\
    }
#   define MPIDI_Pkt_set_seqnum(pkt_, seqnum_)	\
    {						\
    	(pkt_)->seqnum = (seqnum_);		\
    }
#   define MPIDI_VC_Init_seqnum_send(vc_)	\
    {						\
    	(vc_)->seqnum_send = 0;			\
    }
#else
#   define MPIDI_Request_set_seqnum(req_, seqnum_)
#   define MPIDI_VC_FAI_send_seqnum(vc_, seqnum_out_)
#   define MPIDI_Pkt_set_seqnum(pkt_, seqnum_)
#   define MPIDI_VC_Init_seqnum_send(vc_)
#endif


/*-------------------
  END REQUEST SECTION
  -------------------*/


/*------------------
  BEGIN COMM SECTION
  ------------------*/
#define MPIDI_Comm_get_vc(comm_, rank_, vcp_) *(vcp_) = (comm_)->vcr[(rank_)]

#define MPIDI_Comm_get_vc_set_active(comm_, rank_, vcp_) do {           \
        *(vcp_) = (comm_)->vcr[(rank_)];                                \
        if ((*(vcp_))->state == MPIDI_VC_STATE_INACTIVE)                \
        {                                                               \
            MPIU_DBG_PrintVCState2(*(vcp_), MPIDI_VC_STATE_ACTIVE);     \
            MPIDI_CHANGE_VC_STATE((*(vcp_)), ACTIVE);                   \
        }                                                               \
    } while(0)

/*----------------
  END COMM SECTION
  ----------------*/


/*--------------------
  BEGIN PACKET SECTION
  --------------------*/
#if !defined(MPICH_DEBUG_MEMINIT)
#   define MPIDI_Pkt_init(pkt_, type_)		\
    {						\
	(pkt_)->type = (type_);			\
    }
#else
#   define MPIDI_Pkt_init(pkt_, type_)				\
    {								\
	memset((void *) (pkt_), 0xfc, sizeof(MPIDI_CH3_Pkt_t));	\
	(pkt_)->type = (type_);					\
    }
#endif

/*------------------
  END PACKET SECTION
  ------------------*/


/*---------------------------
  BEGIN PROCESS GROUP SECTION
  ---------------------------*/
/* FIXME: Determine which of these functions should be exported to all of 
   the MPICH routines and which are internal to the device implementation */
typedef int (*MPIDI_PG_Compare_ids_fn_t)(void * id1, void * id2);
typedef int (*MPIDI_PG_Destroy_fn_t)(MPIDI_PG_t * pg);

int MPIDI_PG_Init( int *, char ***, 
		   MPIDI_PG_Compare_ids_fn_t, MPIDI_PG_Destroy_fn_t);
int MPIDI_PG_Finalize(void);
int MPIDI_PG_Create(int vct_sz, void * pg_id, MPIDI_PG_t ** ppg);
int MPIDI_PG_Destroy(MPIDI_PG_t * pg);
int MPIDI_PG_Find(void * id, MPIDI_PG_t ** pgp);
int MPIDI_PG_Id_compare(void *id1, void *id2);

/* Always use the MPIDI_PG_iterator type, never its expansion.  Otherwise it
   will be difficult to make any changes later. */
typedef MPIDI_PG_t * MPIDI_PG_iterator;
/* 'iter' is similar to 'saveptr' in strtok_r */
int MPIDI_PG_Get_iterator(MPIDI_PG_iterator *iter);
int MPIDI_PG_Has_next(MPIDI_PG_iterator *iter);
int MPIDI_PG_Get_next(MPIDI_PG_iterator *iter, MPIDI_PG_t **pgp);

int MPIDI_PG_Close_VCs( void );

int MPIDI_PG_InitConnKVS( MPIDI_PG_t * );
int MPIDI_PG_GetConnKVSname( char ** );
int MPIDI_PG_InitConnString( MPIDI_PG_t * );
int MPIDI_PG_GetConnString( MPIDI_PG_t *, int, char *, int );
int MPIDI_PG_Dup_vcr( MPIDI_PG_t *, int, struct MPIDI_VC ** );
int MPIDI_PG_Get_size(MPIDI_PG_t * pg);
void MPIDI_PG_IdToNum( MPIDI_PG_t *, int * );
int MPIU_PG_Printall( FILE * );
int MPIDI_PG_CheckForSingleton( void );

/* CH3_PG_Init allows the channel to pre-initialize the process group */
int MPIDI_CH3_PG_Init( MPIDI_PG_t * );

#define MPIDI_PG_add_ref(pg_)			\
do {                                            \
    MPIU_Object_add_ref(pg_);			\
} while (0)
#define MPIDI_PG_release_ref(pg_, inuse_)	\
do {                                            \
    MPIU_Object_release_ref(pg_, inuse_);	\
} while (0)

#define MPIDI_PG_Get_vc(pg_, rank_, vcp_) *(vcp_) = &(pg_)->vct[rank_]

#define MPIDI_PG_Get_vc_set_active(pg_, rank_, vcp_)  do {              \
        *(vcp_) = &(pg_)->vct[rank_];                                   \
        if ((*(vcp_))->state == MPIDI_VC_STATE_INACTIVE)                \
        {                                                               \
            MPIU_DBG_PrintVCState2(*(vcp_), MPIDI_VC_STATE_ACTIVE);     \
            MPIDI_CHANGE_VC_STATE((*(vcp_)), ACTIVE);                   \
        }                                                               \
    } while(0)

#define MPIDI_PG_Get_size(pg_) ((pg_)->size)

#ifdef MPIDI_DEV_IMPLEMENTS_KVS
int MPIDI_PG_To_string(MPIDI_PG_t *pg_ptr, char **str_ptr, int *);
int MPIDI_PG_Create_from_string(const char * str, MPIDI_PG_t ** pg_pptr, 
				int *flag);
#endif
/*-------------------------
  END PROCESS GROUP SECTION
  -------------------------*/


/*--------------------------------
  BEGIN VIRTUAL CONNECTION SECTION
  --------------------------------*/
/*E
  MPIDI_VC_State - States for a virtual connection.
 
  Notes:
  A closed connection is placed into 'STATE_INACTIVE'. (is this true?)
 E*/
typedef enum MPIDI_VC_State
{
    MPIDI_VC_STATE_INACTIVE=1,      /* Comm either hasn't started or has completed. */
    MPIDI_VC_STATE_ACTIVE,          /* Comm has started and hasn't completed */
    MPIDI_VC_STATE_LOCAL_CLOSE,     /* Local side has initiated close protocol */
    MPIDI_VC_STATE_REMOTE_CLOSE,    /* Remote side has initiated close protocol */
    MPIDI_VC_STATE_CLOSE_ACKED,     /* Both have initiated close, we have acknowledged remote side */
    MPIDI_VC_STATE_CLOSED,          /* Both have initiated close, both have acked */
    MPIDI_VC_STATE_INACTIVE_CLOSED, /* INACTIVE VCs are moved to this state in Finalize */
    MPIDI_VC_STATE_MORIBUND         /* Abnormally terminated, there may be unsent/unreceived msgs */
} MPIDI_VC_State_t;

struct MPID_Comm;

#ifdef ENABLE_COMM_OVERRIDES
typedef struct MPIDI_Comm_ops
{
    /* Overriding calls in case of matching-capable interfaces */
    int (*recv_posted)(struct MPIDI_VC *vc, struct MPID_Request *req);
    
    int (*send)(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
		int dest, int tag, MPID_Comm *comm, int context_offset,
		struct MPID_Request **request);
    int (*rsend)(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
		 int dest, int tag, MPID_Comm *comm, int context_offset,
		 struct MPID_Request **request);
    int (*ssend)(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
		 int dest, int tag, MPID_Comm *comm, int context_offset,
		 struct MPID_Request **request );
    int (*isend)(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
		 int dest, int tag, MPID_Comm *comm, int context_offset,
		 struct MPID_Request **request );
    int (*irsend)(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
		  int dest, int tag, MPID_Comm *comm, int context_offset,
		  struct MPID_Request **request );
    int (*issend)(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
		  int dest, int tag, MPID_Comm *comm, int context_offset,
		  struct MPID_Request **request );
    
    int (*send_init)(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
		     int dest, int tag, MPID_Comm *comm, int context_offset,
		     struct MPID_Request **request );
    int (*bsend_init)(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
		      int dest, int tag, MPID_Comm *comm, int context_offset,
		      struct MPID_Request **request);
    int (*rsend_init)(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
		      int dest, int tag, MPID_Comm *comm, int context_offset,
		      struct MPID_Request **request );
    int (*ssend_init)(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
		      int dest, int tag, MPID_Comm *comm, int context_offset,
		      struct MPID_Request **request );
    int (*startall)(struct MPIDI_VC *vc, int count,  struct MPID_Request *requests[]);
    
    int (*cancel_send)(struct MPIDI_VC *vc,  struct MPID_Request *sreq);
    int (*cancel_recv)(struct MPIDI_VC *vc,  struct MPID_Request *rreq);
    
    int (*probe)(struct MPIDI_VC *vc,  int source, int tag, MPID_Comm *comm, int context_offset,
		                  MPI_Status *status);
    int (*iprobe)(struct MPIDI_VC *vc,  int source, int tag, MPID_Comm *comm, int context_offset,
		  int *flag, MPI_Status *status);
   
} MPIDI_Comm_ops_t;

extern int (*MPIDI_Anysource_iprobe_fn)(int tag, MPID_Comm * comm, int context_offset, int *flag,
                                        MPI_Status * status);
#endif

typedef struct MPIDI_VC
{
    /* XXX - need better comment */
    /* MPIU_Object fields.  MPIDI_VC_t objects are not allocated using the 
       MPIU_Object system, but we do use the associated
       reference counting routines.  The handle value is required 
       when debugging objects (the handle kind is used in reporting
       on changes to the object).
    */
    MPIU_OBJECT_HEADER; /* adds handle and ref_count fields */

    /* state of the VC */
    MPIDI_VC_State_t state;

    /* Process group to which this VC belongs */
    struct MPIDI_PG * pg;

    /* Rank of the process in that process group associated with this VC */
    int pg_rank;

    /* Local process ID */
    int lpid;

    /* The node id of this process, used for topologically aware collectives. */
    MPID_Node_id_t node_id;

    /* port name tag */ 
    int port_name_tag; /* added to handle dynamic process mgmt */
    
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    /* Sequence number of the next packet to be sent */
    MPID_Seqnum_t seqnum_send;
#endif
    
#if defined(MPIDI_CH3_MSGS_UNORDERED)
    /* Sequence number of the next packet we expect to receive */
    MPID_Seqnum_t seqnum_recv;

    /* Queue for holding packets received out of order.  NOTE: the CH3 device 
       only orders packets.  Handling of out-of-order data
       is the responsibility of the channel. */
    MPIDI_CH3_Pkt_send_container_t * msg_reorder_queue;
#endif

    /* rendezvous function pointers.  Called to send a rendevous
       message or when one is matched */
    int (* rndvSend_fn)( struct MPID_Request **sreq_p, const void * buf, int count, 
                         MPI_Datatype datatype, int dt_contig, MPIDI_msg_sz_t data_sz, 
                         MPI_Aint dt_true_lb, int rank, int tag,
                         struct MPID_Comm * comm, int context_offset );
    int (* rndvRecv_fn)( struct MPIDI_VC * vc, struct MPID_Request *rreq );

    /* eager message threshold */
    int eager_max_msg_sz;
    
    /* noncontiguous send function pointer.  Called to send a
       noncontiguous message.  Caller must initialize
       sreq->dev.segment, _first and _size.  Contiguous messages are
       called directly from CH3 and cannot be overridden. */
    int (* sendNoncontig_fn)( struct MPIDI_VC *vc, struct MPID_Request *sreq,
			      void *header, MPIDI_msg_sz_t hdr_sz );

#ifdef ENABLE_COMM_OVERRIDES
    MPIDI_Comm_ops_t *comm_ops;    
#endif

    /* Rather than have each channel define its own fields for the 
       channel-specific data, we provide a fixed-sized scratchpad.  Currently,
       this has a very generous size, though this may shrink later (a channel
       can always allocate storage and hang it off of the end).  This 
       is necessary to allow dynamic loading of channels at MPI_Init time. */
    /* The ssm channel needed a *huge* space for the VC.  But now that it's
       gone, we need to determine the appropriate smaller size to use instead. */
    /* Note also that for dynamically-loaded channels, the VCs must all be the
       same size, so MPIDI_CH3_VC_SIZE should not be overridden when building
       multiple channels that will be used together */
#ifndef MPIDI_CH3_VC_SIZE
#define MPIDI_CH3_VC_SIZE 256
#endif
    int32_t channel_private[MPIDI_CH3_VC_SIZE];
# if defined(MPIDI_CH3_VC_DECL)
    MPIDI_CH3_VC_DECL
# endif
}
MPIDI_VC_t;

typedef enum MPIDI_VC_Event
{
    MPIDI_VC_EVENT_TERMINATED
}
MPIDI_VC_Event_t;

#ifndef HAVE_MPIDI_VCRT
#define HAVE_MPIDI_VCRT
typedef struct MPIDI_VCRT * MPID_VCRT;
typedef struct MPIDI_VC * MPID_VCR;
#endif

/* number of VCs that are in MORIBUND state */
extern int MPIDI_Failed_vc_count;

/* Initialize a new VC */
int MPIDI_VC_Init( MPIDI_VC_t *, MPIDI_PG_t *, int );

#if defined(MPIDI_CH3_MSGS_UNORDERED)
#   define MPIDI_VC_Init_seqnum_recv(vc_);	\
    {						\
    	(vc_)->seqnum_recv = 0;			\
    	(vc_)->msg_reorder_queue = NULL;	\
    }
#else
#   define MPIDI_VC_Init_seqnum_recv(vc_);
#endif


#define MPIDI_VC_add_ref( _vc )                                 \
    do { MPIU_Object_add_ref( _vc ); } while (0)

#define MPIDI_VC_release_ref( _vc, _inuse ) \
    do { MPIU_Object_release_ref( _vc, _inuse ); } while (0)

/*------------------------------
  END VIRTUAL CONNECTION SECTION
  ------------------------------*/


/*---------------------------------
  BEGIN SEND/RECEIVE BUFFER SECTION
  ---------------------------------*/
#if !defined(MPIDI_CH3U_Offsetof)
#    define MPIDI_CH3U_Offsetof(struct_, field_) ((MPI_Aint) &((struct_*)0)->field_)
#endif

#if !defined(MPIDI_CH3U_SRBuf_size)
#    define MPIDI_CH3U_SRBuf_size (256 * 1024)
#endif

typedef struct __MPIDI_CH3U_SRBuf_element {
    /* Keep the buffer at the top to help keep the memory alignment */
    char   buf[MPIDI_CH3U_SRBuf_size];
    struct __MPIDI_CH3U_SRBuf_element * next;
} MPIDI_CH3U_SRBuf_element_t;

extern MPIDI_CH3U_SRBuf_element_t * MPIDI_CH3U_SRBuf_pool;

#if !defined (MPIDI_CH3U_SRBuf_get)
#   define MPIDI_CH3U_SRBuf_get(req_)                                   \
    {                                                                   \
        MPIDI_CH3U_SRBuf_element_t * tmp;                               \
        if (!MPIDI_CH3U_SRBuf_pool) {                                   \
             MPIDI_CH3U_SRBuf_pool =                                    \
                MPIU_Malloc(sizeof(MPIDI_CH3U_SRBuf_element_t));        \
            MPIDI_CH3U_SRBuf_pool->next = NULL;                         \
        }                                                               \
        tmp = MPIDI_CH3U_SRBuf_pool;                                    \
        MPIDI_CH3U_SRBuf_pool = MPIDI_CH3U_SRBuf_pool->next;            \
        tmp->next = NULL;                                               \
        (req_)->dev.tmpbuf = tmp->buf;                                  \
    }
#endif

#if !defined (MPIDI_CH3U_SRBuf_free)
#   define MPIDI_CH3U_SRBuf_free(req_)                                  \
    {                                                                   \
        MPIDI_CH3U_SRBuf_element_t * tmp;                               \
        MPIU_Assert(MPIDI_Request_get_srbuf_flag(req_));                \
        MPIDI_Request_set_srbuf_flag((req_), FALSE);                    \
        tmp = (MPIDI_CH3U_SRBuf_element_t *) (((MPI_Aint) ((req_)->dev.tmpbuf)) - \
               ((MPI_Aint) MPIDI_CH3U_Offsetof(MPIDI_CH3U_SRBuf_element_t, buf))); \
        tmp->next = MPIDI_CH3U_SRBuf_pool;                              \
        MPIDI_CH3U_SRBuf_pool = tmp;                                    \
    }
#endif

#if !defined(MPIDI_CH3U_SRBuf_alloc)
#   define MPIDI_CH3U_SRBuf_alloc(req_, size_)				\
    {									\
        MPIDI_CH3U_SRBuf_get(req_);                                     \
 	if ((req_)->dev.tmpbuf != NULL)					\
 	{								\
 	    (req_)->dev.tmpbuf_sz = MPIDI_CH3U_SRBuf_size;		\
 	    MPIDI_Request_set_srbuf_flag((req_), TRUE);			\
 	}								\
 	else								\
 	{								\
 	    (req_)->dev.tmpbuf_sz = 0;					\
 	}								\
    }
#endif
/*-------------------------------
  END SEND/RECEIVE BUFFER SECTION
  -------------------------------*/

/*----------------------------
  BEGIN DEBUGGING TOOL SECTION
  ----------------------------*/

/* If there is no support for dynamic processes, there will be no
   channel-specific connection state */
#ifdef USE_DBG_LOGGING

#ifdef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
#define MPIDI_CH3_VC_GetStateString( _c ) "none"
#else
/* FIXME: This duplicates a value in util/sock/ch3usock.h */
const char *MPIDI_CH3_VC_GetStateString(struct MPIDI_VC *);
const char *MPIDI_CH3_VC_SockGetStateString(struct MPIDI_VC *);
#endif

/* These tw routines are in mpidi_pg.c and are used to print the 
   connection string (which is attached to a process group) */
int MPIDI_PrintConnStr( const char *file, int line, 
			const char *label, const char *str );
int MPIDI_PrintConnStrToFile( FILE *fd, const char *file, int line, 
			      const char *label, const char *str );
#endif

/* These macros simplify and unify the debugging of changes in the
   connection state 

   MPIU_DBG_VCSTATECHANGE(vc,newstate) - use when changing the state
   of a VC

   MPIU_DBG_VCCHSTATECHANGE(vc,newstate) - use when changing the state
   of the channel-specific part of the vc (e.g., vc->ch.state)

   MPIU_DBG_CONNSTATECHANGE(vc,conn,newstate ) - use when changing the
   state of a conn.  vc may be null

   MPIU_DBG_CONNSTATECHANGEMSG(vc,conn,newstate,msg ) - use when changing the
   state of a conn.  vc may be null.  Like CONNSTATECHANGE, but allows
   an additional message

   MPIU_DBG_PKT(conn,pkt,msg) - print out a short description of an
   packet being sent/received on the designated connection, prefixed with
   msg.

*/
#define MPIU_DBG_VCSTATECHANGE(_vc,_newstate) do { \
     MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST, \
     "vc=%p: Setting state (vc) from %s to %s, vcchstate is %s", \
                 _vc, MPIDI_VC_GetStateString((_vc)->state), \
                 #_newstate, MPIU_CALL(MPIDI_CH3,VC_GetStateString( (_vc) ))) );\
} while (0)

#define MPIU_DBG_VCCHSTATECHANGE(_vc,_newstate) \
     MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST, \
     "vc=%p: Setting state (ch) from %s to %s, vc state is %s", \
	   _vc, MPIU_CALL(MPIDI_CH3,VC_GetStateString((_vc))), \
           #_newstate, MPIDI_VC_GetStateString( (_vc)->state )) )

#define MPIU_DBG_CONNSTATECHANGE(_vc,_conn,_newstate) \
     MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST, \
     "vc=%p,conn=%p: Setting state (conn) from %s to %s, vcstate = %s", \
             _vc, _conn, \
             MPIDI_Conn_GetStateString((_conn)->state), #_newstate, \
             _vc ? MPIDI_VC_GetStateString((_vc)->state) : "<no vc>" ))

#define MPIU_DBG_CONNSTATECHANGE_MSG(_vc,_conn,_newstate,_msg) \
     MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST, \
     "vc=%p,conn=%p: Setting conn state from %s to %s, vcstate = %s %s", \
             _vc, _conn, \
             MPIDI_Conn_GetStateString((_conn)->state), #_newstate, \
             _vc ? MPIDI_VC_GetStateString((_vc)->state) : "<no vc>", _msg ))
#define MPIU_DBG_VCUSE(_vc,_msg) \
     MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST,\
      "vc=%p: Using vc for %s", _vc, _msg ))
#define MPIU_DBG_PKT(_conn,_pkt,_msg) \
     MPIU_DBG_MSG_FMT(CH3_OTHER,TYPICAL,(MPIU_DBG_FDEST,\
     "conn=%p: %s %s", _conn, _msg, MPIDI_Pkt_GetDescString( _pkt ) ))

const char *MPIDI_Pkt_GetDescString( MPIDI_CH3_Pkt_t *pkt );

/* These macros help trace communication headers */
#define MPIU_DBG_MSGPKT(_vc,_tag,_contextid,_dest,_size,_kind)	\
    MPIU_DBG_MSG_FMT(CH3_MSG,TYPICAL,(MPIU_DBG_FDEST,\
		      "%s: vc=%p, tag=%d, context=%d, dest=%d, datasz=" MPIDI_MSG_SZ_FMT,\
		      _kind,_vc,_tag,_contextid,_dest,_size) )

/* FIXME: Switch this to use the common debug code */
void MPIDI_dbg_printf(int, char *, char *, ...);
void MPIDI_err_printf(char *, char *, ...);

/* FIXME: This does not belong here */
#ifdef USE_MPIU_DBG_PRINT_VC
extern char *MPIU_DBG_parent_str;
#endif

#if defined(MPICH_DBG_OUTPUT)
#define MPIDI_DBG_PRINTF(e_)				\
{                                               	\
    if (MPIU_dbg_state != MPIU_DBG_STATE_NONE)		\
    {							\
	MPIDI_dbg_printf e_;				\
    }							\
}
#else
#   define MPIDI_DBG_PRINTF(e)
#endif

#define MPIDI_ERR_PRINTF(e) MPIDI_err_printf e

#if defined(HAVE_CPP_VARARGS)
#   define MPIDI_dbg_printf(level, func, fmt, args...)			\
    {									\
    	MPIU_dbglog_printf("[%d] %s(): " fmt "\n", MPIR_Process.comm_world->rank, func, ## args);	\
    }
#   define MPIDI_err_printf(func, fmt, args...)				\
    {									\
    	MPIU_Error_printf("[%d] ERROR - %s(): " fmt "\n", MPIR_Process.comm_world->rank, func, ## args);	\
    	fflush(stdout);							\
    }
#endif

/* This is used to quote a name in a definition (see FUNCNAME/FCNAME below) */
#define MPIDI_QUOTE(A) MPIDI_QUOTE2(A)
#define MPIDI_QUOTE2(A) #A

#ifdef MPICH_DBG_OUTPUT
    void MPIDI_DBG_Print_packet(MPIDI_CH3_Pkt_t *pkt);
#else
#   define MPIDI_DBG_Print_packet(a)
#endif

/* Given a state, return the string for this state (VC's and connections) */
const char * MPIDI_VC_GetStateString(int);
/*--------------------------
  END DEBUGGING TOOL SECTION
  --------------------------*/


/* Prototypes for internal device routines */
int MPIDI_Isend_self(const void *, int, MPI_Datatype, int, int, MPID_Comm *, 
		     int, int, MPID_Request **);

/*--------------------------
  BEGIN MPI PORT SECTION 
  --------------------------*/
/* These are the default functions */
int MPIDI_Comm_connect(const char *, MPID_Info *, int, MPID_Comm *, MPID_Comm **);
int MPIDI_Comm_accept(const char *, MPID_Info *, int, MPID_Comm *, MPID_Comm **);

int MPIDI_Comm_spawn_multiple(int, char **, char ***, int *, MPID_Info **, 
			      int, MPID_Comm *, MPID_Comm **, int *);


/* This structure defines a module that handles the routines that 
   work with MPI port names */
typedef struct MPIDI_Port_Ops {
    int (*OpenPort)( MPID_Info *, char * );
    int (*ClosePort)( const char * );
    int (*CommAccept)( const char *, MPID_Info *, int, MPID_Comm *, 
		       MPID_Comm ** );
    int (*CommConnect)( const char *, MPID_Info *, int, MPID_Comm *, 
			MPID_Comm ** );
} MPIDI_PortFns;
#define MPIDI_PORTFNS_VERSION 1
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns * );

/* Utility routines provided in src/ch3u_port.c for working with connection
   queues */
int MPIDI_CH3I_Acceptq_enqueue(MPIDI_VC_t * vc, int port_name_tag);
int MPIDI_CH3I_Acceptq_dequeue(MPIDI_VC_t ** vc, int port_name_tag);
#ifdef MPIDI_CH3_CHANNEL_AVOIDS_SELECT
int MPIDI_CH3_Complete_Acceptq_dequeue(MPIDI_VC_t * vc);
#else
#define MPIDI_CH3_Complete_Acceptq_dequeue(vc)  MPI_SUCCESS
#endif
/*--------------------------
  END MPI PORT SECTION 
  --------------------------*/

/* part of mpid_vc.c, this routine completes any pending operations 
   on a communicator */
int MPIDI_CH3U_Comm_FinishPending( MPID_Comm * );

#define MPIDI_MAX_KVS_VALUE_LEN    4096

/* ------------------------------------------------------------------------- */
/* mpirma.h (in src/mpi/rma?) */
/* ------------------------------------------------------------------------- */

/* This structure defines a module that handles the routines that 
   work with MPI-2 RMA ops */
typedef struct MPIDI_RMA_Ops {
    int (*Win_create)(void *, MPI_Aint, int, MPID_Info *, MPID_Comm *,
		      MPID_Win **, struct MPIDI_RMA_Ops *);
    int (*Win_free)(MPID_Win **);
    int (*Put)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, 
		MPID_Win *);
    int (*Get)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, 
		MPID_Win *);
    int (*Accumulate)(void *, int, MPI_Datatype, int, MPI_Aint, int, 
		       MPI_Datatype, MPI_Op, MPID_Win *);
    int (*Win_fence)(int, MPID_Win *);
    int (*Win_post)(MPID_Group *, int, MPID_Win *);
    int (*Win_start)(MPID_Group *, int, MPID_Win *);
    int (*Win_complete)(MPID_Win *);
    int (*Win_wait)(MPID_Win *);
    int (*Win_lock)(int, int, int, MPID_Win *);
    int (*Win_unlock)(int, MPID_Win *);
    void * (*Alloc_mem)(size_t, MPID_Info *);
    int (*Free_mem)(void *);
} MPIDI_RMAFns;
#define MPIDI_RMAFNS_VERSION 1
int MPIDI_CH3_RMAFnsInit( MPIDI_RMAFns * );

/* FIXME: These are specific to the RMA code and should be in the RMA 
   header file. */
#define MPIDI_RMA_PUT 23
#define MPIDI_RMA_GET 24
#define MPIDI_RMA_ACCUMULATE 25
#define MPIDI_RMA_LOCK 26

/* Special case RMA operations */
#define MPIDI_RMA_ACC_CONTIG 27

#define MPIDI_RMA_DATATYPE_BASIC 50
#define MPIDI_RMA_DATATYPE_DERIVED 51

#define MPID_LOCK_NONE 0

int MPIDI_Win_create(void *, MPI_Aint, int, MPID_Info *, MPID_Comm *,
                    MPID_Win ** );
int MPIDI_Win_fence(int, MPID_Win *);
int MPIDI_Put(void *, int, MPI_Datatype, int, MPI_Aint, int,
            MPI_Datatype, MPID_Win *); 
int MPIDI_Get(void *, int, MPI_Datatype, int, MPI_Aint, int,
            MPI_Datatype, MPID_Win *);
int MPIDI_Accumulate(void *, int, MPI_Datatype, int, MPI_Aint, int, 
		   MPI_Datatype,  MPI_Op, MPID_Win *);
int MPIDI_Win_free(MPID_Win **); 
int MPIDI_Win_wait(MPID_Win *win_ptr);
int MPIDI_Win_test(MPID_Win *win_ptr, int *);
int MPIDI_Win_complete(MPID_Win *win_ptr);
int MPIDI_Win_post(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr);
int MPIDI_Win_start(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr);
int MPIDI_Win_lock(int lock_type, int dest, int assert, MPID_Win *win_ptr);
int MPIDI_Win_unlock(int dest, MPID_Win *win_ptr);
void *MPIDI_Alloc_mem(size_t size, MPID_Info *info_ptr);
int MPIDI_Free_mem(void *ptr);

/* optional channel-specific */
void *MPIDI_CH3_Alloc_mem(size_t size, MPID_Info *info_ptr);
int MPIDI_CH3_Win_create(void *base, MPI_Aint size, int disp_unit, 
			 MPID_Info *info, MPID_Comm *comm_ptr, 
			 MPID_Win **win_ptr, MPIDI_RMAFns *RMAFns);
int MPIDI_CH3_Free_mem(void *ptr);
void MPIDI_CH3_Cleanup_mem(void);
int MPIDI_CH3_Win_free(MPID_Win **win_ptr);
int MPIDI_CH3_Put(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr);
int MPIDI_CH3_Get(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr);
int MPIDI_CH3_Accumulate(void *origin_addr, int origin_count, MPI_Datatype
                    origin_datatype, int target_rank, MPI_Aint target_disp,
                    int target_count, MPI_Datatype target_datatype, MPI_Op op,
                    MPID_Win *win_ptr);
int MPIDI_CH3_Win_fence(int assert, MPID_Win *win_ptr);
int MPIDI_CH3_Win_lock(int lock_type, int dest, int assert, MPID_Win *win_ptr);
int MPIDI_CH3_Win_unlock(int dest, MPID_Win *win_ptr);
int MPIDI_CH3_Win_wait(MPID_Win *win_ptr);
int MPIDI_CH3_Win_complete(MPID_Win *win_ptr);
int MPIDI_CH3_Win_post(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr);
int MPIDI_CH3_Win_start(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr);

/* internal */
int MPIDI_CH3I_Release_lock(MPID_Win * win_ptr);
int MPIDI_CH3I_Try_acquire_win_lock(MPID_Win * win_ptr, int requested_lock);
int MPIDI_CH3I_Send_lock_granted_pkt(MPIDI_VC_t * vc, int source_win_ptr);
int MPIDI_CH3I_Send_pt_rma_done_pkt(MPIDI_VC_t * vc, int source_win_ptr);

#define MPIDI_CH3I_DATATYPE_IS_PREDEFINED(type, predefined) \
    if ((HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) || \
        (type == MPI_FLOAT_INT) || (type == MPI_DOUBLE_INT) || \
        (type == MPI_LONG_INT) || (type == MPI_SHORT_INT) || \
	(type == MPI_LONG_DOUBLE_INT)) \
        predefined = 1; \
    else predefined = 0;

int MPIDI_CH3I_Progress_finalize(void);

/*@
  MPIDI_CH3_Progress_signal_completion - Inform the progress engine that a 
  pending request has completed.

  IMPLEMENTORS:
  In a single-threaded environment, this routine can be implemented by
  incrementing a request completion counter.  In a
  multi-threaded environment, the request completion counter must be atomically
  incremented, and any threaded blocking in the
  progress engine must be woken up when a request is completed.

  Notes on the implementation:

  This code is designed to support one particular model of thread-safety.
  It is common to many of the channels and was moved into this file because
  the MPIDI_CH3_Progress_signal_completion reference is used by the 
  macro the implements MPID_Request_set_completed.  Note that there is 
  a function version of MPID_Request_set_completed for use by greq_complete.c

@*/

/*
 * MPIDI_CH3_Progress_signal_completion() is used to notify the progress
 * engine that a completion has occurred.  The multi-threaded version will need
 * to wake up any (and all) threads blocking in MPIDI_CH3_Progress().
 */

/* This allows the channel to define an alternate to the 
   completion counter.  The dllchannel uses this to access the 
   counter through a pointer.  Others could use this to provide
   more complex operations */
#ifndef MPIDI_CH3I_INCR_PROGRESS_COMPLETION_COUNT
#define MPIDI_CH3I_INCR_PROGRESS_COMPLETION_COUNT                                \
    do {                                                                         \
        MPIU_THREAD_CS_ENTER(COMPLETION,);                                       \
        ++MPIDI_CH3I_progress_completion_count;                                  \
        MPIU_DBG_MSG_D(CH3_PROGRESS,VERBOSE,                                     \
                     "just incremented MPIDI_CH3I_progress_completion_count=%d", \
                     MPIDI_CH3I_progress_completion_count);                      \
        MPIU_THREAD_CS_EXIT(COMPLETION,);                                        \
    } while (0)
#endif


/* The following is part of an implementation of a control of a 
   resource shared among threads - it needs to be managed more 
   explicitly as such as shared resource */
#ifndef MPICH_IS_THREADED
#   define MPIDI_CH3_Progress_signal_completion()	\
    {							\
       MPIDI_CH3I_INCR_PROGRESS_COMPLETION_COUNT;		\
    }
#else
    /* TODO these decls should probably move into each channel as appropriate */
    extern volatile int MPIDI_CH3I_progress_blocked;
    extern volatile int MPIDI_CH3I_progress_wakeup_signalled;

/* This allows the channel to hook the MPIDI_CH3_Progress_signal_completion
 * macro when it is necessary to wake up some part of the progress engine from a
 * blocking operation.  Currently ch3:sock uses it, ch3:nemesis does not. */
/* MT alternative implementations of this macro are responsible for providing any
 * synchronization (acquiring MPIDCOMM, etc) */
#ifndef MPIDI_CH3I_PROGRESS_WAKEUP
# define MPIDI_CH3I_PROGRESS_WAKEUP do {/*do nothing*/} while(0)
#endif

    void MPIDI_CH3I_Progress_wakeup(void);
    /* MT TODO profiling is needed here.  We currently protect the completion
     * counter with the COMPLETION critical section, which could be a source of
     * contention.  It should be possible to peform these updates atomically via
     * OPA instead, but the additional complexity should be justified by
     * profiling evidence.  [goodell@ 2010-06-29] */
#   define MPIDI_CH3_Progress_signal_completion()			\
    do {                                                                \
        MPIDI_CH3I_INCR_PROGRESS_COMPLETION_COUNT;                      \
        MPIDI_CH3I_PROGRESS_WAKEUP;                                     \
    } while (0)
#endif

/* Function that may be used to provide business card info */
int MPIDI_CH3I_BCInit( char **bc_val_p, int *val_max_sz_p);
/* Function to free the storage allocated by MPIDI_CH3I_BCInit */
int MPIDI_CH3I_BCFree( char *publish_bc );

/* Inform the process group of our connection information string (business
   card) */
int MPIDI_PG_SetConnInfo( int rank, const char *connString );

/* Fill in the node_id information for each VC in the given PG. */
int MPIDI_Populate_vc_node_ids(MPIDI_PG_t *pg, int our_pg_rank);

/* NOTE: Channel function prototypes are in mpidi_ch3_post.h since some of the 
   macros require their declarations. */

/* FIXME: These should be defined only when these particular utility
   packages are used.  Best would be to keep these prototypes in the
   related util/xxx directories, and either copy them into an include
   directory used only for builds or add (yet another) include path */
/* from util/sock */
int MPIDI_VC_InitSock( MPIDI_VC_t *);
int MPIDI_CH3I_Connect_to_root_sock(const char *, MPIDI_VC_t **);


int MPIDI_CH3I_VC_post_sockconnect(MPIDI_VC_t * );

/* FIXME: Where should this go? */

/* Used internally to broadcast process groups belonging to peercomm to
 all processes in comm*/
int MPID_PG_BCast( MPID_Comm *peercomm_p, MPID_Comm *comm_p, int root );

/* Channel defintitions */
/*@
  MPIDI_CH3_iStartMsg - A non-blocking request to send a CH3 packet.  A r
  equest object is allocated only if the send could not be completed 
  immediately.

  Input Parameters:
+ vc - virtual connection to send the message over
. pkt - pointer to a MPIDI_CH3_Pkt_t structure containing the substructure to 
  be sent
- pkt_sz - size of the packet substucture

  Output Parameters:
. sreq_ptr - send request or NULL if the send completed immediately

  Return value:
  An mpi error code.
  
  NOTE:
  The packet structure may be allocated on the stack.

  IMPLEMETORS:
  If the send can not be completed immediately, the CH3 packet structure must 
  be stored internally until the request is complete.
  
  If the send completes immediately, the channel implementation should return 
  NULL.
@*/
int MPIDI_CH3_iStartMsg(MPIDI_VC_t * vc, void * pkt, MPIDI_msg_sz_t pkt_sz, 
			MPID_Request **sreq_ptr);


/*@
  MPIDI_CH3_iStartMsgv - A non-blocking request to send a CH3 packet and 
  associated data.  A request object is allocated only if
  the send could not be completed immediately.

  Input Parameters:
+ vc - virtual connection to send the message over
. iov - a vector of a structure contains a buffer pointer and length
- iov_n - number of elements in the vector

  Output Parameters:
. sreq_ptr - send request or NULL if the send completed immediately

  Return value:
  An mpi error code.
  
  NOTE:
  The first element in the vector must point to the packet structure.   The 
  packet structure and the vector may be allocated on
  the stack.

  IMPLEMENTORS:
  If the send can not be completed immediately, the CH3 packet structure and 
  the vector must be stored internally until the
  request is complete.
  
  If the send completes immediately, the channel implementation should return 
  NULL.
@*/
int MPIDI_CH3_iStartMsgv(MPIDI_VC_t * vc, MPID_IOV * iov, int iov_n, 
			 MPID_Request **sreq_ptr);


/*@
  MPIDI_CH3_iSend - A non-blocking request to send a CH3 packet using an 
  existing request object.  When the send is complete
  the channel implementation will call the OnDataAvail routine in the
  request, if any (if not, the channel implementation will mark the 
  request as complete).

  Input Parameters:
+ vc - virtual connection over which to send the CH3 packet
. sreq - pointer to the send request object
. pkt - pointer to a MPIDI_CH3_Pkt_t structure containing the substructure to 
  be sent
- pkt_sz - size of the packet substucture

  Return value:
  An mpi error code.
  
  NOTE:
  The packet structure may be allocated on the stack.

  IMPLEMETORS:
  If the send can not be completed immediately, the packet structure must be 
  stored internally until the request is complete.

  If the send completes immediately, the channel implementation still must 
  invoke the OnDataAvail routine in the request, if any; otherwise, is 
  must set the request as complete.
@*/
int MPIDI_CH3_iSend(MPIDI_VC_t * vc, MPID_Request * sreq, void * pkt, 
		    MPIDI_msg_sz_t pkt_sz);


/*@
  MPIDI_CH3_iSendv - A non-blocking request to send a CH3 packet and 
  associated data using an existing request object.  When
  the send is complete the channel implementation will call the
  OnDataAvail routine in the request, if any.

  Input Parameters:
+ vc - virtual connection over which to send the CH3 packet and data
. sreq - pointer to the send request object
. iov - a vector of a structure contains a buffer pointer and length
- iov_n - number of elements in the vector

  Return value:
  An mpi error code.
  
  NOTE:
  The first element in the vector must point to the packet structure.   The 
  packet structure and the vector may be allocated on
  the stack.

  IMPLEMENTORS:
  If the send can not be completed immediately, the packet structure and the 
  vector must be stored internally until the request is
  complete.

  If the send completes immediately, the channel implementation still must 
  call the OnDataAvail routine in the request, if any.
@*/
int MPIDI_CH3_iSendv(MPIDI_VC_t * vc, MPID_Request * sreq, MPID_IOV * iov, 
		     int iov_n);

/*@
  MPIDI_CH3_Connection_terminate - terminate the underlying connection 
  associated with the specified VC

  Input Parameters:
. vc - virtual connection

  Return value:
  An MPI error code
@*/
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc);

/* MPIDI_CH3_Connect_to_root (really connect to peer) - channel routine
   for connecting to a process through a port, used in implementing
   MPID_Comm_connect and accept */
int MPIDI_CH3_Connect_to_root(const char *, MPIDI_VC_t **);

/* keyval for COMM_WORLD attribute holding list of failed processes */
extern int MPICH_ATTR_FAILED_PROCESSES;


/*
 * Channel utility prototypes
 */
int MPIDI_CH3U_Recvq_FU(int, int, int, MPI_Status * );
MPID_Request * MPIDI_CH3U_Recvq_FDU(MPI_Request, MPIDI_Message_match *);
MPID_Request * MPIDI_CH3U_Recvq_FDU_or_AEP(int source, int tag, 
                                          int context_id, MPID_Comm *comm, void *user_buf,
                                           int user_count, MPI_Datatype datatype, int * foundp);
int MPIDI_CH3U_Recvq_DP(MPID_Request * rreq);
MPID_Request * MPIDI_CH3U_Recvq_FDP_or_AEU(MPIDI_Message_match * match, 
					   int * found);
int MPIDI_CH3U_Recvq_count_unexp(void);
int MPIDI_CH3U_Complete_posted_with_error(MPIDI_VC_t *vc);


int MPIDI_CH3U_Request_load_send_iov(MPID_Request * const sreq, 
				     MPID_IOV * const iov, int * const iov_n);
int MPIDI_CH3U_Request_load_recv_iov(MPID_Request * const rreq);
int MPIDI_CH3U_Request_unpack_uebuf(MPID_Request * rreq);
int MPIDI_CH3U_Request_unpack_srbuf(MPID_Request * rreq);

void MPIDI_CH3U_Buffer_copy(const void * const sbuf, int scount, 
			    MPI_Datatype sdt, int * smpi_errno,
			    void * const rbuf, int rcount, MPI_Datatype rdt, 
			    MPIDI_msg_sz_t * rdata_sz, int * rmpi_errno);
int MPIDI_CH3U_Post_data_receive(int found, MPID_Request ** rreqp);
int MPIDI_CH3U_Post_data_receive_found(MPID_Request * rreqp);
int MPIDI_CH3U_Post_data_receive_unexpected(MPID_Request * rreqp);
int MPIDI_CH3U_Receive_data_found(MPID_Request *rreq, char *buf, MPIDI_msg_sz_t *buflen, int *complete);
int MPIDI_CH3U_Receive_data_unexpected(MPID_Request * rreq, char *buf, MPIDI_msg_sz_t *buflen, int *complete);

/* FIXME: This is a macro! */
#ifndef MPIDI_CH3_Request_add_ref
/*@
  MPIDI_CH3_Request_add_ref - Increment the reference count associated with a
  request object

  Input Parameters:
. req - pointer to the request object
@*/
void MPIDI_CH3_Request_add_ref(MPID_Request * req);
#endif

/*@
  MPIDI_CH3_GetParentPort - obtain the port name associated with the parent

  Output Parameters:
.  parent_port_name - the port name associated with the parent communicator

  Return value:
  A MPI error code.
  
  NOTE:
  'MPIDI_CH3_GetParentPort' should only be called if the initialization
  (in the current implementation, done with the static function 
  'InitPGFromPMI' in 'mpid_init.c') has determined that this process
  in fact has a parent.
@*/
int MPIDI_CH3_GetParentPort(char ** parent_port_name);

/*@
   MPIDI_CH3_FreeParentPort - This routine frees the storage associated with
   a parent port (allocted with MPIDH_CH3_GetParentPort).

  @*/
void MPIDI_CH3_FreeParentPort( void );

/*E
  MPIDI_CH3_Abort - Abort this process.

  Input Parameters:
+ exit_code - exit code to be returned by the process
- error_msg - error message to print

  Return value:
  This function should not return.

  Notes:
  This routine is used only if the channel defines
  'MPIDI_CH3_IMPLEMENTS_ABORT'.  This allows the channel to handle 
  aborting processes, particularly when the channel does not use the standard
  PMI interface.
E*/
int MPIDI_CH3_Abort(int exit_code, char * error_msg);

/* FIXME: Move these prototypes into header files in the appropriate 
   util directories  */
/* added by brad.  upcalls for MPIDI_CH3_Init that contain code which could be 
   executed by two or more channels */
int MPIDI_CH3U_Init_sock(int has_parent, MPIDI_PG_t * pg_p, int pg_rank,
                         char **bc_val_p, int *val_max_sz_p);

/* added by brad.  business card related global and functions */
/* FIXME: Make these part of the channel support headers */
#define MAX_HOST_DESCRIPTION_LEN 256
int MPIDI_CH3U_Get_business_card_sock(int myRank, 
				      char **bc_val_p, int *val_max_sz_p);

int MPIDI_CH3_Get_business_card(int myRank, char *value, int length);

/*
 * Channel upcall prototypes
 */

/*E
  MPIDI_CH3U_Handle_recv_pkt- Handle a freshly received CH3 packet.

  Input Parameters:
+ vc - virtual connection over which the packet was received
- pkt - pointer to the CH3 packet

  Output Parameter:
. rreqp - receive request defining data to be received; may be NULL

  NOTE:
  Multiple threads may not simultaneously call this routine with the same 
  virtual connection.  This constraint eliminates the
  need to lock the VC and thus improves performance.  If simultaneous upcalls 
  for a single VC are a possible, then the calling
  routine must serialize the calls (perhaps by locking the VC).  Special 
  consideration may need to be given to packet ordering
  if the channel has made guarantees about ordering.
E*/
int MPIDI_CH3U_Handle_recv_pkt(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt, 
			       MPIDI_msg_sz_t *buflen, MPID_Request ** rreqp);

/*@
  MPIDI_CH3U_Handle_recv_req - Process a receive request for which all of the 
  data has been received (and copied) into the
  buffers described by the request's IOV.

  Input Parameters:
+ vc - virtual connection over which the data was received
- rreq - pointer to the receive request object

  Output Parameter:
. complete - data transfer for the request has completed
@*/
int MPIDI_CH3U_Handle_recv_req(MPIDI_VC_t * vc, MPID_Request * rreq, 
			       int * complete);

/* Handle_send_req invokes the action (method/function) when data 
   becomes available.  It is an obsolete routine; the completion 
   function should be invoked directly.  */
int MPIDI_CH3U_Handle_send_req(MPIDI_VC_t * vc, MPID_Request * sreq, 
			       int *complete);

int MPIDI_CH3U_Handle_connection(MPIDI_VC_t * vc, MPIDI_VC_Event_t event);

int MPIDI_CH3U_VC_SendClose( MPIDI_VC_t *vc, int rank );
int MPIDI_CH3U_VC_WaitForClose( void );
#ifdef MPIDI_CH3_HAS_CHANNEL_CLOSE
int MPIDI_CH3_Channel_close( void );
#else
#define MPIDI_CH3_Channel_close( )   MPI_SUCCESS
#endif
/* MPIDI_CH3U_Check_for_failed_procs() reads PMI_dead_processes key
   and marks VCs to those processes as failed */
int MPIDI_CH3U_Check_for_failed_procs(void);

/*@
  MPIDI_CH3_Pre_init - Allows the channel to initialize before PMI_init is 
  called, and allows the
  channel to optionally set the rank, size, and whether this process has a 
  parent.

  Output Parameters:
+ setvals - boolean value that is true if this function set has_parent, rank, 
  and size
. has_parent - boolean value that is true if this MPI job was spawned by 
  another set of MPI processes
. rank - rank of this process in the process group
- size - number of processes in the process group

  Return value:
  A MPI error code.

  Notes:
  This function is optional, and is used only when HAVE_CH3_PRE_INIT is 
  defined.  It is called by CH3 before PMI_Init.  If the function sets setvals 
  to TRUE, CH3 will not use PMI to get the rank,  size, etc.
@*/
int MPIDI_CH3_Pre_init (int *setvals, int *has_parent, int *rank, int *size);

/*@
  MPIDI_CH3_Init - Initialize the channel implementation.

  Input Parameters:
+ has_parent - boolean value that is true if this MPI job was spawned by 
  another set of MPI processes
. pg_ptr - the new process group representing MPI_COMM_WORLD
- pg_rank - my rank in the process group

  Return value:
  A MPI error code.

  Notes:
  MPID_Init has called 'PMI_Init' and created the process group structure 
  before this routine is called.
@*/
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t *pg_ptr, int pg_rank );

/*@
  MPIDI_CH3_PreLoad - Setup a channel before calling MPIDI_CH3_Init

  Notes:
  This routine is called only if the channel defines 'HAVE_CH3_PRELOAD' .
  It may be used to perform any initialization that is required before any
  of the channel routines may be used.

  @*/
int MPIDI_CH3_PreLoad( void );

/*@
  MPIDI_CH3_Finalize - Shutdown the channel implementation.

  Return value:
  A MPI error class.
@*/
int MPIDI_CH3_Finalize(void);

/*@
  MPIDI_CH3_VC_Init - Perform channel-specific initialization of a VC

  Input Parameter:
. vc - Virtual connection to initialize
  @*/
int MPIDI_CH3_VC_Init( struct MPIDI_VC *vc );

/*@
   MPIDI_CH3_PG_Destroy - Perform any channel-specific actions when freeing 
   a process group 

    Input Parameter:
.   pg - Process group on which to act
@*/
int MPIDI_CH3_PG_Destroy( struct MPIDI_PG *pg );

/*@ MPIDI_CH3_VC_Destroy - Perform and channel-specific actions when freeing a
    virtual connection.

    Input Parameter:
.   vc - Virtual connection on which to act
@*/
int MPIDI_CH3_VC_Destroy( struct MPIDI_VC *vc );

/*@ MPIDI_CH3_InitComplete - Perform any channel-specific initialization 
  actions after MPID_Init but before MPI_Init (or MPI_Initthread) returns
  @*/
int MPIDI_CH3_InitCompleted( void );

#ifdef MPIDI_CH3_HASIMPL_HEADER
#include "mpidi_ch3_mpid.h"
#endif
/* Routines in support of ch3 */

/* Routine to return the tag associated with a port */
int MPIDI_GetTagFromPort( const char *, int * );

/* Here are the packet handlers */
int MPIDI_CH3_PktHandler_EagerSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				   MPIDI_msg_sz_t *, MPID_Request ** );
#ifdef USE_EAGER_SHORT
int MPIDI_CH3_PktHandler_EagerShortSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					 MPIDI_msg_sz_t *, MPID_Request ** );
#endif
int MPIDI_CH3_PktHandler_ReadySend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				    MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_EagerSyncSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_EagerSyncAck( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				       MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_RndvReqToSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_RndvClrToSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_RndvSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				   MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_CancelSendReq( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_CancelSendResp( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					 MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_Put( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
			      MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_Accumulate( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				     MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_Accumulate_Immed( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				     MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_Get( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
			      MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_GetResp( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				 MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_Lock( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
			      MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_LockGranted( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				      MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_PtRMADone( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				    MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_LockPutUnlock( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_LockAccumUnlock( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					  MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_LockGetUnlock( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_FlowCntlUpdate( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
					 MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_Close( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				MPIDI_msg_sz_t *, MPID_Request ** );
int MPIDI_CH3_PktHandler_EndCH3( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *,
				 MPIDI_msg_sz_t *, MPID_Request ** );

/* PktHandler function:
   vc  (INPUT) -- vc on which the packet was received
   pkt (INPUT) -- pointer to packet header at beginning of receive buffer
   buflen (I/O) -- IN: number of bytes received into receive buffer
                   OUT: number of bytes processed by the handler function
   req (OUTPUT) -- NULL, if the whole message has been processed by the handler
                   function, otherwise, pointer to the receive request for this
                   message.  The IOV will be set describing where the rest of the
                   message should be received.
*/
typedef int MPIDI_CH3_PktHandler_Fcn(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
				     MPIDI_msg_sz_t *buflen, MPID_Request **req );
int MPIDI_CH3_PktHandler_Init( MPIDI_CH3_PktHandler_Fcn *[], int );

#ifdef MPICH_DBG_OUTPUT
int MPIDI_CH3_PktPrint_CancelSendReq( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_CancelSendResp( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_EagerSend( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_ReadySend( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_RndvReqToSend( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_RndvClrToSend( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_RndvSend( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_EagerSyncSend( FILE *fp, MPIDI_CH3_Pkt_t *pkt );
int MPIDI_CH3_PktPrint_EagerSyncAck( FILE *fp, MPIDI_CH3_Pkt_t *pkt );
#endif

/* Routines to create packets (used in implementing MPI communications */
int MPIDI_CH3_EagerNoncontigSend( MPID_Request **, MPIDI_CH3_Pkt_type_t, 
				  const void *, int, 
				  MPI_Datatype, MPIDI_msg_sz_t, int, int, MPID_Comm *, 
				  int );
int MPIDI_CH3_EagerContigSend( MPID_Request **, MPIDI_CH3_Pkt_type_t, 
			       const void *, MPIDI_msg_sz_t, int, 
			       int, MPID_Comm *, int );
int MPIDI_CH3_EagerContigShortSend( MPID_Request **, MPIDI_CH3_Pkt_type_t, 
				    const void *, MPIDI_msg_sz_t, 
				    int, int, MPID_Comm *, int );
int MPIDI_CH3_EagerContigIsend( MPID_Request **, MPIDI_CH3_Pkt_type_t, 
				const void *, MPIDI_msg_sz_t, int, 
				int, MPID_Comm *, int );


int MPIDI_CH3_RndvSend( MPID_Request **, const void *, int, MPI_Datatype, 
			int, MPIDI_msg_sz_t, MPI_Aint, int, int, MPID_Comm *, int );

int MPIDI_CH3_EagerSyncNoncontigSend( MPID_Request **, const void *, int, 
				      MPI_Datatype, MPIDI_msg_sz_t, int, MPI_Aint,
				      int, int, MPID_Comm *, int );
int MPIDI_CH3_EagerSyncZero(MPID_Request **, int, int, MPID_Comm *, int );

int MPIDI_CH3_SendNoncontig_iov( struct MPIDI_VC *vc, struct MPID_Request *sreq,
                                 void *header, MPIDI_msg_sz_t hdr_sz );

/* Routines to ack packets, called in the receive routines when a 
   message is matched */
int MPIDI_CH3_EagerSyncAck( MPIDI_VC_t *, MPID_Request * );
int MPIDI_CH3_RecvFromSelf( MPID_Request *, void *, int, MPI_Datatype );
int MPIDI_CH3_RecvRndv( MPIDI_VC_t *, MPID_Request * );

/* Handler routines to continuing after an IOV is processed (assigned to the
   OnDataAvail field in the device part of a request) */
int MPIDI_CH3_ReqHandler_RecvComplete( MPIDI_VC_t *, MPID_Request *, int * );
int MPIDI_CH3_ReqHandler_UnpackUEBufComplete( MPIDI_VC_t *, MPID_Request *,
					      int * );
int MPIDI_CH3_ReqHandler_ReloadIOV( MPIDI_VC_t *, MPID_Request *, int * );

int MPIDI_CH3_ReqHandler_UnpackSRBufReloadIOV( MPIDI_VC_t *, MPID_Request *,
					       int * );
int MPIDI_CH3_ReqHandler_UnpackSRBufComplete( MPIDI_VC_t *, MPID_Request *,
					      int * );
int MPIDI_CH3_ReqHandler_PutRespDerivedDTComplete( MPIDI_VC_t *, 
						   MPID_Request *, int * );
int MPIDI_CH3_ReqHandler_PutAccumRespComplete( MPIDI_VC_t *, MPID_Request *,
					       int * );
int MPIDI_CH3_ReqHandler_AccumRespDerivedDTComplete( MPIDI_VC_t *, 
						     MPID_Request *,
						     int * );
int MPIDI_CH3_ReqHandler_SinglePutAccumComplete( MPIDI_VC_t *, MPID_Request *,
						 int * );
int MPIDI_CH3_ReqHandler_GetRespDerivedDTComplete( MPIDI_VC_t *, 
						   MPID_Request *, int * );

/* Send Handlers */
int MPIDI_CH3_ReqHandler_SendReloadIOV( MPIDI_VC_t *vc, MPID_Request *sreq, 
					int *complete );
int MPIDI_CH3_ReqHandler_GetSendRespComplete( MPIDI_VC_t *, MPID_Request *,
					      int * );
/* Thread Support */
#ifdef MPICH_IS_THREADED
#if MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_GLOBAL
/* There is a single, global lock, held for the duration of an MPI call */
#define MPIU_THREAD_CS_ENTER_CH3COMM(context_)
#define MPIU_THREAD_CS_EXIT_CH3COMM(context_)

#define MPIU_THREAD_CS_ENTER_LMT(_context)
#define MPIU_THREAD_CS_EXIT_LMT(_context)

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_PER_OBJECT

#define MPIU_THREAD_CS_ENTER_POBJ_MUTEX(mutex_p_)                                                           \
    do {                                                                                                    \
        MPIU_DBG_MSG_P(THREAD,VERBOSE,"attempting to ENTER per-object CS, mutex=%s", MPIU_QUOTE(mutex_p_)); \
        /* FIXME do we need nest checking here? the existing macros won't work unmodified...*/              \
        MPID_Thread_mutex_lock(mutex_p_);                                                                   \
    } while (0)
#define MPIU_THREAD_CS_EXIT_POBJ_MUTEX(mutex_p_)                                                            \
    do {                                                                                                    \
        MPIU_DBG_MSG_P(THREAD,VERBOSE,"attempting to EXIT per-object CS, mutex=%s", MPIU_QUOTE(mutex_p_));  \
        /* FIXME do we need nest checking here? the existing macros won't work unmodified...*/              \
        MPID_Thread_mutex_unlock(mutex_p_);                                                                 \
    } while (0)

/* There is a per object lock */
#define MPIU_THREAD_CS_ENTER_CH3COMM(context_)                      \
    do {                                                            \
        if (MPIU_ISTHREADED)                                        \
            MPIU_THREAD_CS_ENTER_POBJ_MUTEX(&context_->pobj_mutex); \
    } while (0)
#define MPIU_THREAD_CS_EXIT_CH3COMM(context_)                       \
    do {                                                            \
        if (MPIU_ISTHREADED)                                        \
            MPIU_THREAD_CS_EXIT_POBJ_MUTEX(&context_->pobj_mutex);  \
    } while (0)

/* MT FIXME making LMT into MPIDCOMM for now because of overwhelming deadlock
 * issues */
#define MPIU_THREAD_CS_ENTER_LMT(context_) MPIU_THREAD_CS_ENTER_MPIDCOMM(context_)
#define MPIU_THREAD_CS_EXIT_LMT(context_)  MPIU_THREAD_CS_EXIT_MPIDCOMM(context_)

#elif MPIU_THREAD_GRANULARITY == MPIU_THREAD_GRANULARITY_SINGLE
/* No thread support, make all operations a no-op */
/* FIXME incomplete? or already handled by the upper level? */
/* FIXME does it make sense to have (MPICH_IS_THREADED && _GRANULARITY==_SINGLE) ? */

#else
#error Unrecognized thread granularity
#endif
#else

#endif /* MPICH_IS_THREADED */

#endif /* !defined(MPICH_MPIDIMPL_H_INCLUDED) */

