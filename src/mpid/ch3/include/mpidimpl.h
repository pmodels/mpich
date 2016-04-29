/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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

#include "mpidftb.h"

/* Add the ch3 packet definitions */
#include "mpidpkt.h"

#if !defined(MPIDI_IOV_DENSITY_MIN)
#   define MPIDI_IOV_DENSITY_MIN (16 * 1024)
#endif

#if defined(HAVE_GETHOSTNAME) && defined(NEEDS_GETHOSTNAME_DECL) && \
   !defined(gethostname)
int gethostname(char *name, size_t len);
# endif

/* Default PMI version to use */
#define MPIDI_CH3I_DEFAULT_PMI_VERSION 1
#define MPIDI_CH3I_DEFAULT_PMI_SUBVERSION 1

/* group of processes detected to have failed.  This is a subset of
   comm_world group. */
extern MPIR_Group *MPIDI_Failed_procs_group;
extern int MPIDI_last_known_failed;
extern char *MPIDI_failed_procs_string;

extern int MPIDI_Use_pmi2_api;

#if defined(MPL_USE_DBG_LOGGING)
extern MPL_dbg_class MPIDI_CH3_DBG_CONNECT;
extern MPL_dbg_class MPIDI_CH3_DBG_DISCONNECT;
extern MPL_dbg_class MPIDI_CH3_DBG_PROGRESS;
extern MPL_dbg_class MPIDI_CH3_DBG_CHANNEL;
extern MPL_dbg_class MPIDI_CH3_DBG_OTHER;
extern MPL_dbg_class MPIDI_CH3_DBG_MSG;
extern MPL_dbg_class MPIDI_CH3_DBG_VC;
extern MPL_dbg_class MPIDI_CH3_DBG_REFCOUNT;
#endif /* MPL_USE_DBG_LOGGING */

#define MPIDI_CHANGE_VC_STATE(vc, new_state) do {               \
        MPL_DBG_VCSTATECHANGE(vc, VC_STATE_##new_state);       \
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
    MPIR_OBJECT_HEADER; /* adds handle and ref_count fields */

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

    /* Flag to mark a procress group which is finalizing. This means thay
       the VCs for this process group are closing, (normally becuase
       MPI_Finalize was called). This is required to avoid a reconnection
       of the VCs when the PG is closed due to unused elements in the event
       queue  */
    int finalize;

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
	(data_sz_out_) = (intptr_t) (count_) * MPIR_Datatype_get_basic_size(datatype_); \
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER, TERSE, (MPL_DBG_FDEST,"basic datatype: dt_contig=%d, dt_sz=%d, data_sz=%" PRIdPTR, \
			  (dt_contig_out_), MPIR_Datatype_get_basic_size(datatype_), (data_sz_out_)));\
    }									\
    else								\
    {									\
	MPIR_Datatype_get_ptr((datatype_), (dt_ptr_));			\
	MPIR_Datatype_is_contig((datatype_), (&dt_contig_out_));	\
	(data_sz_out_) = (intptr_t) (count_) * (dt_ptr_)->size;	\
        (dt_true_lb_)    = (dt_ptr_)->true_lb;                          \
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER, TERSE, (MPL_DBG_FDEST, "user defined datatype: dt_contig=%d, dt_sz=" MPI_AINT_FMT_DEC_SPEC ", data_sz=%" PRIdPTR, \
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
 * MPIR_Requests
 *
 * MPI Requests are handles to MPIR_Request structures.  These are used
 * for most communication operations to provide a uniform way in which to
 * define pending operations.  As such, they contain many fields that are 
 * only used by some operations (logically, an MPIR_Request is a union type).
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
#define MPIR_REQUEST_TLS_MAX 128

#  define MPIDI_Request_tls_alloc(req_) \
    do { \
	(req_) = MPIR_Handle_obj_alloc(&MPIR_Request_mem); \
        MPL_DBG_MSG_P(MPIDI_CH3_DBG_CHANNEL,VERBOSE,		\
	       "allocated request, handle=0x%08x", req_);\
    } while (0)


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
    (sreq_) = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);     \
    MPIR_Object_set_ref((sreq_), 2);				\
    (sreq_)->comm = comm;					\
    (sreq_)->dev.partner_request   = NULL;                         \
    MPIR_Comm_add_ref(comm);					\
    (sreq_)->dev.match.parts.rank = rank;			\
    (sreq_)->dev.match.parts.tag = tag;				\
    (sreq_)->dev.match.parts.context_id = comm->context_id + context_offset;	\
    (sreq_)->dev.user_buf = (void *) buf;			\
    (sreq_)->dev.user_count = count;				\
    (sreq_)->dev.datatype = datatype;				\
    (sreq_)->dev.iov_count	   = 0;                         \
}

/* This is the receive request version of MPIDI_Request_create_sreq */
#define MPIDI_Request_create_rreq(rreq_, mpi_errno_, FAIL_)	\
{								\
    (rreq_) = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);           \
    MPIR_Object_set_ref((rreq_), 2);				\
    (rreq_)->kind = MPIR_REQUEST_KIND__RECV;				\
    (rreq_)->dev.partner_request   = NULL;                         \
}

/* creates a new, trivially complete recv request that is suitable for
 * returning when a user passed MPI_PROC_NULL */
#define MPIDI_Request_create_null_rreq(rreq_, mpi_errno_, FAIL_)           \
    do {                                                                   \
        (rreq_) = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);               \
        if ((rreq_) != NULL) {                                             \
            MPIR_Object_set_ref((rreq_), 1);                               \
            /* MT FIXME should these be handled by MPIR_Request_create? */ \
            MPIR_cc_set(&(rreq_)->cc, 0);                                  \
            (rreq_)->kind = MPIR_REQUEST_KIND__RECV;                             \
            MPIR_Status_set_procnull(&(rreq_)->status);                    \
        }                                                                  \
        else {                                                             \
            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,TYPICAL,"unable to allocate a request");\
            (mpi_errno_) = MPIR_ERR_MEMALLOCFAILED;                        \
            FAIL_;                                                         \
        }                                                                  \
    } while (0)

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
#define MPIDI_REQUEST_TYPE_PUT_RECV 5                    /* target is receiving PUT data */
#define MPIDI_REQUEST_TYPE_GET_RESP 6                    /* target is sending GET response data */
#define MPIDI_REQUEST_TYPE_ACCUM_RECV 7                  /* target is receiving ACC data */
#define MPIDI_REQUEST_TYPE_PUT_RECV_DERIVED_DT 8         /* target is receiving derived DT info for PUT data */
#define MPIDI_REQUEST_TYPE_GET_RECV_DERIVED_DT 9         /* target is receiving derived DT info for GET data */
#define MPIDI_REQUEST_TYPE_ACCUM_RECV_DERIVED_DT 10      /* target is receiving derived DT info for ACC data */
#define MPIDI_REQUEST_TYPE_GET_ACCUM_RECV 11             /* target is receiving GACC data */
#define MPIDI_REQUEST_TYPE_GET_ACCUM_RECV_DERIVED_DT 12  /* target is receiving derived DT info for GACC data */
#define MPIDI_REQUEST_TYPE_GET_ACCUM_RESP 13             /* target is sending GACC response data */
#define MPIDI_REQUEST_TYPE_FOP_RECV 14                   /* target is receiving FOP data */
#define MPIDI_REQUEST_TYPE_FOP_RESP 15                   /* target is sending FOP response data */


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
        *(rts_sreq_) = (sreq_)->dev.partner_request;			\
        (sreq_)->dev.partner_request = NULL;				\
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
#define MPIDI_Comm_get_vc(comm_, rank_, vcp_) *(vcp_) = (comm_)->dev.vcrt->vcr_table[(rank_)]

#ifdef USE_MPIDI_DBG_PRINT_VC
void MPIDI_DBG_PrintVC(MPIDI_VC_t *vc);
void MPIDI_DBG_PrintVCState2(MPIDI_VC_t *vc, MPIDI_VC_State_t new_state);
void MPIDI_DBG_PrintVCState(MPIDI_VC_t *vc);
#else
#define MPIDI_DBG_PrintVC(vc)
#define MPIDI_DBG_PrintVCState2(vc, new_state)
#define MPIDI_DBG_PrintVCState(vc)
#endif

#define MPIDI_Comm_get_vc_set_active(comm_, rank_, vcp_) do {           \
        *(vcp_) = (comm_)->dev.vcrt->vcr_table[(rank_)];                \
        if ((*(vcp_))->state == MPIDI_VC_STATE_INACTIVE)                \
        {                                                               \
            MPIDI_DBG_PrintVCState2(*(vcp_), MPIDI_VC_STATE_ACTIVE);     \
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

int MPIDI_VCRT_Create(int size, struct MPIDI_VCRT **vcrt_ptr);
int MPIDI_VCRT_Add_ref(struct MPIDI_VCRT *vcrt);
int MPIDI_VCRT_Release(struct MPIDI_VCRT *vcrt, int isDisconnect);
int MPIDI_VCR_Dup(MPIDI_VCR orig_vcr, MPIDI_VCR * new_vcr);

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
    MPIR_Object_add_ref(pg_);			\
} while (0)
#define MPIDI_PG_release_ref(pg_, inuse_)	\
do {                                            \
    MPIR_Object_release_ref(pg_, inuse_);	\
} while (0)

#define MPIDI_PG_Get_vc(pg_, rank_, vcp_) *(vcp_) = &(pg_)->vct[rank_]

#define MPIDI_PG_Get_vc_set_active(pg_, rank_, vcp_)  do {              \
        *(vcp_) = &(pg_)->vct[rank_];                                   \
        if ((*(vcp_))->state == MPIDI_VC_STATE_INACTIVE)                \
        {                                                               \
            MPIDI_DBG_PrintVCState2(*(vcp_), MPIDI_VC_STATE_ACTIVE);     \
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

struct MPIR_Comm;

#ifdef ENABLE_COMM_OVERRIDES
typedef struct MPIDI_Comm_ops
{
    /* Overriding calls in case of matching-capable interfaces */
    int (*recv_posted)(struct MPIDI_VC *vc, struct MPIR_Request *req);
    
    int (*send)(struct MPIDI_VC *vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
		int dest, int tag, MPIR_Comm *comm, int context_offset,
		struct MPIR_Request **request);
    int (*rsend)(struct MPIDI_VC *vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
		 int dest, int tag, MPIR_Comm *comm, int context_offset,
		 struct MPIR_Request **request);
    int (*ssend)(struct MPIDI_VC *vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
		 int dest, int tag, MPIR_Comm *comm, int context_offset,
		 struct MPIR_Request **request );
    int (*isend)(struct MPIDI_VC *vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
		 int dest, int tag, MPIR_Comm *comm, int context_offset,
		 struct MPIR_Request **request );
    int (*irsend)(struct MPIDI_VC *vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
		  int dest, int tag, MPIR_Comm *comm, int context_offset,
		  struct MPIR_Request **request );
    int (*issend)(struct MPIDI_VC *vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
		  int dest, int tag, MPIR_Comm *comm, int context_offset,
		  struct MPIR_Request **request );
    
    int (*send_init)(struct MPIDI_VC *vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
		     int dest, int tag, MPIR_Comm *comm, int context_offset,
		     struct MPIR_Request **request );
    int (*bsend_init)(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype,
		      int dest, int tag, MPIR_Comm *comm, int context_offset,
		      struct MPIR_Request **request);
    int (*rsend_init)(struct MPIDI_VC *vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
		      int dest, int tag, MPIR_Comm *comm, int context_offset,
		      struct MPIR_Request **request );
    int (*ssend_init)(struct MPIDI_VC *vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
		      int dest, int tag, MPIR_Comm *comm, int context_offset,
		      struct MPIR_Request **request );
    int (*startall)(struct MPIDI_VC *vc, int count,  struct MPIR_Request *requests[]);
    
    int (*cancel_send)(struct MPIDI_VC *vc,  struct MPIR_Request *sreq);
    int (*cancel_recv)(struct MPIDI_VC *vc,  struct MPIR_Request *rreq);
    
    int (*probe)(struct MPIDI_VC *vc,  int source, int tag, MPIR_Comm *comm, int context_offset,
		                  MPI_Status *status);
    int (*iprobe)(struct MPIDI_VC *vc,  int source, int tag, MPIR_Comm *comm, int context_offset,
		  int *flag, MPI_Status *status);
    int (*improbe)(struct MPIDI_VC *vc,  int source, int tag, MPIR_Comm *comm, int context_offset,
                   int *flag, MPIR_Request **message, MPI_Status *status);
    int (*imrecv)(struct MPIDI_VC *vc, struct MPIR_Request *req);
} MPIDI_Comm_ops_t;

extern int (*MPIDI_Anysource_iprobe_fn)(int tag, MPIR_Comm * comm, int context_offset, int *flag,
                                        MPI_Status * status);
extern int (*MPIDI_Anysource_improbe_fn)(int tag, MPIR_Comm * comm, int context_offset,
                                         int *flag, MPIR_Request **message,
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
    MPIR_OBJECT_HEADER; /* adds handle and ref_count fields */

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
    
#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
    void *connreq_obj;  /* pointer to dynamic connection mgmt object */
#endif

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
    int (* rndvSend_fn)( struct MPIR_Request **sreq_p, const void * buf, MPI_Aint count,
                         MPI_Datatype datatype, int dt_contig, intptr_t data_sz,
                         MPI_Aint dt_true_lb, int rank, int tag,
                         struct MPIR_Comm * comm, int context_offset );
    int (* rndvRecv_fn)( struct MPIDI_VC * vc, struct MPIR_Request *rreq );

    /* eager message threshold */
    int eager_max_msg_sz;
    /* eager message threshold for ready sends.  -1 means there's no limit */
    int ready_eager_max_msg_sz;
 
    /* noncontiguous send function pointer.  Called to send a
       noncontiguous message.  Caller must initialize
       sreq->dev.segment, _first and _size.  Contiguous messages are
       called directly from CH3 and cannot be overridden. */
    int (* sendNoncontig_fn)( struct MPIDI_VC *vc, struct MPIR_Request *sreq,
			      void *header, intptr_t hdr_sz );

#ifdef ENABLE_COMM_OVERRIDES
    MPIDI_Comm_ops_t *comm_ops;
#endif

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

/*S
 * MPIDI_VCRT - virtual connection reference table
 *
 * handle - this element is not used, but exists so that we may use the
 * MPIU_Object routines for reference counting
 *
 * ref_count - number of references to this table
 *
 * vcr_table - array of virtual connection references
 S*/
typedef struct MPIDI_VCRT
{
    MPIR_OBJECT_HEADER; /* adds handle and ref_count fields */
    int size;
    MPIDI_VC_t * vcr_table[1];
}
MPIDI_VCRT_t;

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
    do { MPIR_Object_add_ref( _vc ); } while (0)

#define MPIDI_VC_release_ref( _vc, _inuse ) \
    do { MPIR_Object_release_ref( _vc, _inuse ); } while (0)

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
                MPL_malloc(sizeof(MPIDI_CH3U_SRBuf_element_t));        \
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
        MPIR_Assert(MPIDI_Request_get_srbuf_flag(req_));                \
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

/* define ACC stream size as the SRBuf size */
#if !defined(MPIDI_CH3U_Acc_stream_size)
#define MPIDI_CH3U_Acc_stream_size MPIDI_CH3U_SRBuf_size
#endif

/*----------------------------
  BEGIN DEBUGGING TOOL SECTION
  ----------------------------*/

/* If there is no support for dynamic processes, there will be no
   channel-specific connection state */
#ifdef MPL_USE_DBG_LOGGING

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

   MPL_DBG_VCSTATECHANGE(vc,newstate) - use when changing the state
   of a VC

   MPL_DBG_VCCHSTATECHANGE(vc,newstate) - use when changing the state
   of the channel-specific part of the vc (e.g., vc->ch.state)

   MPL_DBG_CONNSTATECHANGE(vc,conn,newstate ) - use when changing the
   state of a conn.  vc may be null

   MPL_DBG_CONNSTATECHANGEMSG(vc,conn,newstate,msg ) - use when changing the
   state of a conn.  vc may be null.  Like CONNSTATECHANGE, but allows
   an additional message

   MPL_DBG_PKT(conn,pkt,msg) - print out a short description of an
   packet being sent/received on the designated connection, prefixed with
   msg.

*/
#define MPL_DBG_VCSTATECHANGE(_vc,_newstate) do { \
     MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT,TYPICAL,(MPL_DBG_FDEST, \
     "vc=%p: Setting state (vc) from %s to %s, vcchstate is %s", \
                 _vc, MPIDI_VC_GetStateString((_vc)->state), \
                 #_newstate, MPIDI_CH3_VC_GetStateString( (_vc) ))); \
} while (0)

#define MPL_DBG_VCCHSTATECHANGE(_vc,_newstate) \
     MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT,TYPICAL,(MPL_DBG_FDEST, \
     "vc=%p: Setting state (ch) from %s to %s, vc state is %s", \
           _vc, MPIDI_CH3_VC_GetStateString((_vc)), \
           #_newstate, MPIDI_VC_GetStateString( (_vc)->state )) )

#define MPL_DBG_CONNSTATECHANGE(_vc,_conn,_newstate) \
     MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT,TYPICAL,(MPL_DBG_FDEST, \
     "vc=%p,conn=%p: Setting state (conn) from %s to %s, vcstate = %s", \
             _vc, _conn, \
             MPIDI_Conn_GetStateString((_conn)->state), #_newstate, \
             _vc ? MPIDI_VC_GetStateString((_vc)->state) : "<no vc>" ))

#define MPL_DBG_CONNSTATECHANGE_MSG(_vc,_conn,_newstate,_msg) \
     MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT,TYPICAL,(MPL_DBG_FDEST, \
     "vc=%p,conn=%p: Setting conn state from %s to %s, vcstate = %s %s", \
             _vc, _conn, \
             MPIDI_Conn_GetStateString((_conn)->state), #_newstate, \
             _vc ? MPIDI_VC_GetStateString((_vc)->state) : "<no vc>", _msg ))
#define MPL_DBG_VCUSE(_vc,_msg) \
     MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT,TYPICAL,(MPL_DBG_FDEST,\
      "vc=%p: Using vc for %s", _vc, _msg ))
#define MPL_DBG_PKT(_conn,_pkt,_msg) \
     MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TYPICAL,(MPL_DBG_FDEST,\
     "conn=%p: %s %s", _conn, _msg, MPIDI_Pkt_GetDescString( _pkt ) ))

const char *MPIDI_Pkt_GetDescString( MPIDI_CH3_Pkt_t *pkt );

/* These macros help trace communication headers */
#define MPL_DBG_MSGPKT(_vc,_tag,_contextid,_dest,_size,_kind)	\
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_MSG,TYPICAL,(MPL_DBG_FDEST,\
		      "%s: vc=%p, tag=%d, context=%d, dest=%d, datasz=%" PRIdPTR,\
		      _kind,_vc,_tag,_contextid,_dest,_size) )

/* FIXME: Switch this to use the common debug code */
void MPIDI_err_printf(char *, char *, ...);

/* FIXME: This does not belong here */
#ifdef USE_MPIDI_DBG_PRINT_VC
extern char *MPIDI_DBG_parent_str;
#endif

#define MPIDI_ERR_PRINTF(e) MPIDI_err_printf e

#if defined(HAVE_MACRO_VA_ARGS)
#   define MPIDI_err_printf(func, fmt, ...)				\
    {									\
        MPL_error_printf("[%d] ERROR - %s(): " fmt "\n", MPIR_Process.comm_world->rank, func, __VA_ARGS__);    \
        fflush(stdout);							\
    }
#endif

/* This is used to quote a name in a definition (see FUNCNAME/FCNAME below) */
#define MPL_QUOTE(A) MPL_QUOTE2(A)
#define MPL_QUOTE2(A) #A

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
int MPIDI_Isend_self(const void *, MPI_Aint, MPI_Datatype, int, int, MPIR_Comm *,
		     int, int, MPIR_Request **);

/*--------------------------
  BEGIN MPI PORT SECTION
  --------------------------*/
/* These are the default functions */
int MPIDI_Comm_connect(const char *, MPIR_Info *, int, MPIR_Comm *, MPIR_Comm **);
int MPIDI_Comm_accept(const char *, MPIR_Info *, int, MPIR_Comm *, MPIR_Comm **);

int MPIDI_Comm_spawn_multiple(int, char **, char ***, const int *, MPIR_Info **,
			      int, MPIR_Comm *, MPIR_Comm **, int *);


/* This structure defines a module that handles the routines that
   work with MPI port names */
typedef struct MPIDI_Port_Ops {
    int (*OpenPort)( MPIR_Info *, char * );
    int (*ClosePort)( const char * );
    int (*CommAccept)( const char *, MPIR_Info *, int, MPIR_Comm *,
		       MPIR_Comm ** );
    int (*CommConnect)( const char *, MPIR_Info *, int, MPIR_Comm *,
			MPIR_Comm ** );
} MPIDI_PortFns;
#define MPIDI_PORTFNS_VERSION 1
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns * );

#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
/* Utility routines provided in src/ch3u_port.c for working with connection
   queues */
int MPIDI_CH3I_Acceptq_enqueue(MPIDI_VC_t * vc, int port_name_tag);
int MPIDI_Port_finalize(void);

int MPIDI_CH3I_Port_init(int port_name_tag);
int MPIDI_CH3I_Port_destroy(int port_name_tag);
#else
/* Need empty symbols to avoid failure at compile time if defined
 * MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS. */
#define MPIDI_CH3I_Acceptq_enqueue(vc, port_name_tag) (MPI_SUCCESS)
#define MPIDI_Port_finalize() (MPI_SUCCESS)

#define MPIDI_CH3I_Port_init(port_name_tag) (MPI_SUCCESS)
#define MPIDI_CH3I_Port_destroy(port_name_tag) (MPI_SUCCESS)
#endif
/*--------------------------
  END MPI PORT SECTION 
  --------------------------*/

#define MPIDI_MAX_KVS_VALUE_LEN    4096

/* ------------------------------------------------------------------------- */
/* mpirma.h (in src/mpi/rma?) */
/* ------------------------------------------------------------------------- */

int MPIDI_RMA_init(void);
void MPIDI_RMA_finalize(void);

/* The Win_fns table contains pointers to the channel's implementation of the
 * RMA window creation routines.  The channel must provide the init function,
 * which can optionally override any defaults already set by CH3.
 */

typedef struct {
    int (*create)(void *, MPI_Aint, int, MPIR_Info *, MPIR_Comm *, MPIR_Win **);
    int (*allocate)(MPI_Aint, int, MPIR_Info *, MPIR_Comm *, void *, MPIR_Win **);
    int (*allocate_shared)(MPI_Aint, int, MPIR_Info *, MPIR_Comm *, void *, MPIR_Win **);
    int (*allocate_shm)(MPI_Aint, int, MPIR_Info *, MPIR_Comm *, void *, MPIR_Win **);
    int (*create_dynamic)(MPIR_Info *, MPIR_Comm *, MPIR_Win **);
    int (*detect_shm)(MPIR_Win **);
    int (*gather_info)(void *, MPI_Aint, int, MPIR_Info *, MPIR_Comm *, MPIR_Win **);
    int (*shared_query)(MPIR_Win *, int, MPI_Aint *, int *, void *);
} MPIDI_CH3U_Win_fns_t;

extern MPIDI_CH3U_Win_fns_t MPIDI_CH3U_Win_fns;

typedef struct {
    int (*win_init)(MPI_Aint, int, int, int, MPIR_Info *, MPIR_Comm *, MPIR_Win **);
    int (*win_free)(MPIR_Win **);
} MPIDI_CH3U_Win_hooks_t;

extern MPIDI_CH3U_Win_hooks_t MPIDI_CH3U_Win_hooks;

typedef struct MPIDI_CH3U_Win_pkt_ordering {

    /* Ordered AM flush.
     * It means whether AM flush is guaranteed to be finished after all previous
     * RMA operations. It initialized by Nemesis and used by CH3.
     * Note that we use single global flag for all targets including both
     * intra-node and inter-node processes.*/
    int am_flush_ordered;
} MPIDI_CH3U_Win_pkt_ordering_t;

extern MPIDI_CH3U_Win_pkt_ordering_t MPIDI_CH3U_Win_pkt_orderings;

/* CH3 and Channel window functions initializers */
int MPIDI_Win_fns_init(MPIDI_CH3U_Win_fns_t *win_fns);
int MPIDI_CH3_Win_fns_init(MPIDI_CH3U_Win_fns_t *win_fns);

/* Channel window hooks initializer */
int MPIDI_CH3_Win_hooks_init(MPIDI_CH3U_Win_hooks_t *win_hooks);

int MPIDI_CH3_Win_pkt_orderings_init(MPIDI_CH3U_Win_pkt_ordering_t * win_pkt_orderings);

/* Default window creation functions provided by CH3 */
int MPIDI_CH3U_Win_create(void *, MPI_Aint, int, MPIR_Info *, MPIR_Comm *,
                         MPIR_Win **);
int MPIDI_CH3U_Win_allocate(MPI_Aint size, int disp_unit, MPIR_Info *info,
                           MPIR_Comm *comm, void *baseptr, MPIR_Win **win);
int MPIDI_CH3U_Win_allocate_no_shm(MPI_Aint size, int disp_unit, MPIR_Info *info,
                                   MPIR_Comm *comm_ptr, void *baseptr, MPIR_Win **win_ptr);
int MPIDI_CH3U_Win_create_dynamic(MPIR_Info *info, MPIR_Comm *comm, MPIR_Win **win);
int MPIDI_CH3U_Win_shared_query(MPIR_Win * win_ptr, int target_rank, MPI_Aint * size,
                                int *disp_unit, void *baseptr);

/* MPI RMA Utility functions */

int MPIDI_CH3U_Win_gather_info(void *, MPI_Aint, int, MPIR_Info *, MPIR_Comm *,
                                 MPIR_Win **);


#ifdef MPIDI_CH3I_HAS_ALLOC_MEM
void* MPIDI_CH3I_Alloc_mem(size_t size, MPIR_Info *info_ptr);
/* fallback to MPL_malloc if channel does not have its own RMA memory allocator */
#else
#define MPIDI_CH3I_Alloc_mem(size, info_ptr)    MPL_malloc(size)
#endif

#ifdef MPIDI_CH3I_HAS_FREE_MEM
int MPIDI_CH3I_Free_mem(void *ptr);
#else
#define MPIDI_CH3I_Free_mem(ptr)    MPL_free(ptr);
#endif

/* Pvars */
void MPIDI_CH3_RMA_Init_sync_pvars(void);
void MPIDI_CH3_RMA_Init_pkthandler_pvars(void);

/* internal */
int MPIDI_CH3I_Release_lock(MPIR_Win * win_ptr);
int MPIDI_CH3I_Try_acquire_win_lock(MPIR_Win * win_ptr, int requested_lock);

int MPIDI_CH3I_Progress_finalize(void);


/* Internal RMA operation routines.
 * Called by normal RMA operations and request-based RMA operations . */
int MPIDI_CH3I_Put(const void *origin_addr, int origin_count, MPI_Datatype
                   origin_datatype, int target_rank, MPI_Aint target_disp,
                   int target_count, MPI_Datatype target_datatype, MPIR_Win * win_ptr,
                   MPIR_Request * ureq);
int MPIDI_CH3I_Get(void *origin_addr, int origin_count, MPI_Datatype
                   origin_datatype, int target_rank, MPI_Aint target_disp,
                   int target_count, MPI_Datatype target_datatype, MPIR_Win * win_ptr,
                   MPIR_Request * ureq);
int MPIDI_CH3I_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype
                          origin_datatype, int target_rank, MPI_Aint target_disp,
                          int target_count, MPI_Datatype target_datatype, MPI_Op op,
                          MPIR_Win * win_ptr, MPIR_Request * ureq);
int MPIDI_CH3I_Get_accumulate(const void *origin_addr, int origin_count,
                              MPI_Datatype origin_datatype, void *result_addr, int result_count,
                              MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                              int target_count, MPI_Datatype target_datatype, MPI_Op op,
                              MPIR_Win * win_ptr, MPIR_Request * ureq);

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
  function the implements MPID_Request_complete.
@*/

/*
 * MPIDI_CH3_Progress_signal_completion() is used to notify the progress
 * engine that a completion has occurred.  The multi-threaded version will need
 * to wake up any (and all) threads blocking in MPIDI_CH3_Progress().
 */

/* This allows the channel to define an alternate to the 
   completion counter.  */
#ifndef MPIDI_CH3I_INCR_PROGRESS_COMPLETION_COUNT
#define MPIDI_CH3I_INCR_PROGRESS_COMPLETION_COUNT                                \
    do {                                                                         \
        MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_COMPLETION_MUTEX);                                       \
        ++MPIDI_CH3I_progress_completion_count;                                  \
        MPL_DBG_MSG_D(MPIDI_CH3_DBG_PROGRESS,VERBOSE,                                     \
                     "just incremented MPIDI_CH3I_progress_completion_count=%d", \
                     MPIDI_CH3I_progress_completion_count);                      \
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMPLETION_MUTEX);                                        \
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
int MPID_PG_BCast( MPIR_Comm *peercomm_p, MPIR_Comm *comm_p, int root );

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
int MPIDI_CH3_iStartMsg(MPIDI_VC_t * vc, void * pkt, intptr_t pkt_sz,
			MPIR_Request **sreq_ptr);


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
int MPIDI_CH3_iStartMsgv(MPIDI_VC_t * vc, MPL_IOV * iov, int iov_n, 
			 MPIR_Request **sreq_ptr);


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
int MPIDI_CH3_iSend(MPIDI_VC_t * vc, MPIR_Request * sreq, void * pkt,
		    intptr_t pkt_sz);


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
int MPIDI_CH3_iSendv(MPIDI_VC_t * vc, MPIR_Request * sreq, MPL_IOV * iov,
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

/*
 * Channel utility prototypes
 */
int MPIDI_CH3U_Recvq_init(void);
int MPIDI_CH3U_Recvq_FU(int, int, int, MPI_Status * );
MPIR_Request * MPIDI_CH3U_Recvq_FDU(MPI_Request, MPIDI_Message_match *);
MPIR_Request * MPIDI_CH3U_Recvq_FDU_matchonly(int source, int tag, int context_id, MPIR_Comm *comm,
                                   int *foundp);
MPIR_Request * MPIDI_CH3U_Recvq_FDU_or_AEP(int source, int tag,
                                          int context_id, MPIR_Comm *comm, void *user_buf,
                                           MPI_Aint user_count, MPI_Datatype datatype, int * foundp);
int MPIDI_CH3U_Recvq_DP(MPIR_Request * rreq);
MPIR_Request * MPIDI_CH3U_Recvq_FDP_or_AEU(MPIDI_Message_match * match,
					   int * found);
int MPIDI_CH3U_Recvq_count_unexp(void);
int MPIDI_CH3U_Complete_posted_with_error(MPIDI_VC_t *vc);
int MPIDI_CH3U_Clean_recvq(MPIR_Comm *comm_ptr);


int MPIDI_CH3U_Request_load_send_iov(MPIR_Request * const sreq,
				     MPL_IOV * const iov, int * const iov_n);
int MPIDI_CH3U_Request_load_recv_iov(MPIR_Request * const rreq);
int MPIDI_CH3U_Request_unpack_uebuf(MPIR_Request * rreq);
int MPIDI_CH3U_Request_unpack_srbuf(MPIR_Request * rreq);

void MPIDI_CH3U_Buffer_copy(const void * const sbuf, MPI_Aint scount,
			    MPI_Datatype sdt, int * smpi_errno,
			    void * const rbuf, MPI_Aint rcount, MPI_Datatype rdt,
			    intptr_t * rdata_sz, int * rmpi_errno);
int MPIDI_CH3U_Post_data_receive(int found, MPIR_Request ** rreqp);
int MPIDI_CH3U_Post_data_receive_found(MPIR_Request * rreqp);
int MPIDI_CH3U_Post_data_receive_unexpected(MPIR_Request * rreqp);
int MPIDI_CH3U_Receive_data_found(MPIR_Request *rreq, void *buf, intptr_t *buflen, int *complete);
int MPIDI_CH3U_Receive_data_unexpected(MPIR_Request * rreq, void *buf, intptr_t *buflen, int *complete);

/* Initialization routine for ch3u_comm.c */
int MPIDI_CH3I_Comm_init(void);

int MPIDI_CH3I_Comm_handle_failed_procs(MPIR_Group *new_failed_procs);
void MPIDI_CH3I_Comm_find(MPIR_Context_id_t context_id, MPIR_Comm **comm);

/* The functions below allow channels to register functions to be
   called immediately after a communicator has been created, and
   immediately before a communicator is to be destroyed.
 */
int MPIDI_CH3U_Comm_register_create_hook(int (*hook_fn)(struct MPIR_Comm *, void *), void *param);
int MPIDI_CH3U_Comm_register_destroy_hook(int (*hook_fn)(struct MPIR_Comm *, void *), void *param);

/* FIXME: This is a macro! */
#ifndef MPIDI_CH3_Request_add_ref
/*@
  MPIDI_CH3_Request_add_ref - Increment the reference count associated with a
  request object

  Input Parameters:
. req - pointer to the request object
@*/
void MPIDI_CH3_Request_add_ref(MPIR_Request * req);
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
- pkt - pointer to the CH3 packet header
- data - pointer to the start address of data

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
int MPIDI_CH3U_Handle_recv_pkt(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt, void *data,
			       intptr_t *buflen, MPIR_Request ** rreqp);

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
int MPIDI_CH3U_Handle_recv_req(MPIDI_VC_t * vc, MPIR_Request * rreq,
			       int * complete);

/* Handle_send_req invokes the action (method/function) when data 
   becomes available.  It is an obsolete routine; the completion 
   function should be invoked directly.  */
int MPIDI_CH3U_Handle_send_req(MPIDI_VC_t * vc, MPIR_Request * sreq,
			       int *complete);

int MPIDI_CH3U_Handle_connection(MPIDI_VC_t * vc, MPIDI_VC_Event_t event);

int MPIDI_CH3U_VC_SendClose( MPIDI_VC_t *vc, int rank );
int MPIDI_CH3U_VC_WaitForClose( void );
#ifdef MPIDI_CH3_HAS_CHANNEL_CLOSE
int MPIDI_CH3_Channel_close( void );
#else
#define MPIDI_CH3_Channel_close( )   MPI_SUCCESS
#endif

/* MPIDI_CH3U_Get_failed_group() generates a group of failed processes based
 * on the last list generated during MPIDI_CH3U_Check_for_failed_procs */
int MPIDI_CH3U_Get_failed_group(int last_rank, MPIR_Group **failed_group);
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

/*@ MPIDI_CH3_InitCompleted - Perform any channel-specific initialization
  actions after MPID_Init but before MPI_Init (or MPI_Initthread) returns
  @*/
int MPIDI_CH3_InitCompleted( void );

#ifdef MPIDI_CH3_HASIMPL_HEADER
#include "mpidi_ch3_mpid.h"
#endif
/* Routines in support of ch3 */

#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
/* Routine to return the tag associated with a port */
int MPIDI_GetTagFromPort( const char *, int * );
#else
/* Need empty symbol to avoid failure at compile time if defined
 * MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS. */
#define MPIDI_GetTagFromPort(port_name, port_name_tag) (MPI_SUCCESS)
#endif

/* Here are the packet handlers */
int MPIDI_CH3_PktHandler_EagerSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
				   intptr_t *, MPIR_Request ** );
#ifdef USE_EAGER_SHORT
int MPIDI_CH3_PktHandler_EagerShortSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
					 intptr_t *, MPIR_Request ** );
#endif
int MPIDI_CH3_PktHandler_ReadySend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
				    intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_EagerSyncSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
					intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_EagerSyncAck( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
				       intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_RndvReqToSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
					intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_RndvClrToSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
					intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_RndvSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
				   intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_CancelSendReq( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
					intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_CancelSendResp( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
					 intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_Put( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
			      intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_Accumulate( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
				     intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_GetAccumulate( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
                                        intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_CAS( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
                              intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_CASResp( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
                                  intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_FOP( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
                              intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_FOPResp( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
                                  intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_Get_AccumResp( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
                                        intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_Get( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
			      intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_GetResp( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
				 intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_Lock( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
			      intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_LockAck( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
				      intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_LockOpAck( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
                                    intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_Unlock( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
                                 intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_Flush( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
                                intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_Ack( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
                              intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_DecrAtCnt( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
                                    intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_FlowCntlUpdate( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *,
					 intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_Close( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
				intptr_t *, MPIR_Request ** );

#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
/* packet handlers used in dynamic process connection. */
int MPIDI_CH3_PktHandler_ConnAck(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt, void * data,
                                 intptr_t * buflen, MPIR_Request ** rreqp);
int MPIDI_CH3_PktHandler_AcceptAck(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt, void * data,
                                   intptr_t * buflen, MPIR_Request ** rreqp);
#endif /* end of MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS */

int MPIDI_CH3_PktHandler_EndCH3( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, void *,
				 intptr_t *, MPIR_Request ** );
int MPIDI_CH3_PktHandler_Revoke(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void * data,
                                intptr_t *buflen, MPIR_Request **rreqp);
int MPIDI_CH3_PktHandler_Init( MPIDI_CH3_PktHandler_Fcn *[], int );

int MPIDI_CH3I_RMA_Make_progress_global(int *made_progress);

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
int MPIDI_CH3_EagerNoncontigSend( MPIR_Request **, MPIDI_CH3_Pkt_type_t,
				  const void *, MPI_Aint,
				  MPI_Datatype, intptr_t, int, int, MPIR_Comm *,
				  int );
int MPIDI_CH3_EagerContigSend( MPIR_Request **, MPIDI_CH3_Pkt_type_t,
			       const void *, intptr_t, int,
			       int, MPIR_Comm *, int );
int MPIDI_CH3_EagerContigShortSend( MPIR_Request **, MPIDI_CH3_Pkt_type_t,
				    const void *, intptr_t,
				    int, int, MPIR_Comm *, int );
int MPIDI_CH3_EagerContigIsend( MPIR_Request **, MPIDI_CH3_Pkt_type_t,
				const void *, intptr_t, int,
				int, MPIR_Comm *, int );


int MPIDI_CH3_RndvSend( MPIR_Request **, const void *, MPI_Aint, MPI_Datatype,
			int, intptr_t, MPI_Aint, int, int, MPIR_Comm *, int );

int MPIDI_CH3_EagerSyncNoncontigSend( MPIR_Request **, const void *, int,
				      MPI_Datatype, intptr_t, int, MPI_Aint,
				      int, int, MPIR_Comm *, int );
int MPIDI_CH3_EagerSyncZero(MPIR_Request **, int, int, MPIR_Comm *, int );

int MPIDI_CH3_SendNoncontig_iov( struct MPIDI_VC *vc, struct MPIR_Request *sreq,
                                 void *header, intptr_t hdr_sz );

/* Routines to ack packets, called in the receive routines when a 
   message is matched */
int MPIDI_CH3_EagerSyncAck( MPIDI_VC_t *, MPIR_Request * );
int MPIDI_CH3_RecvFromSelf( MPIR_Request *, void *, MPI_Aint, MPI_Datatype );
int MPIDI_CH3_RecvRndv( MPIDI_VC_t *, MPIR_Request * );

/* Handler routines to continuing after an IOV is processed (assigned to the
   OnDataAvail field in the device part of a request) */
int MPIDI_CH3_ReqHandler_RecvComplete( MPIDI_VC_t *, MPIR_Request *, int * );
int MPIDI_CH3_ReqHandler_UnpackUEBufComplete( MPIDI_VC_t *, MPIR_Request *,
					      int * );
int MPIDI_CH3_ReqHandler_ReloadIOV( MPIDI_VC_t *, MPIR_Request *, int * );

int MPIDI_CH3_ReqHandler_UnpackSRBufReloadIOV( MPIDI_VC_t *, MPIR_Request *,
					       int * );
int MPIDI_CH3_ReqHandler_UnpackSRBufComplete( MPIDI_VC_t *, MPIR_Request *,
					      int * );
int MPIDI_CH3_ReqHandler_PutDerivedDTRecvComplete( MPIDI_VC_t *,
						   MPIR_Request *, int * );
int MPIDI_CH3_ReqHandler_PutRecvComplete( MPIDI_VC_t *, MPIR_Request *,
                                          int * );
int MPIDI_CH3_ReqHandler_AccumRecvComplete( MPIDI_VC_t *, MPIR_Request *,
                                            int * );
int MPIDI_CH3_ReqHandler_GaccumRecvComplete( MPIDI_VC_t *, MPIR_Request *,
                                             int * );
int MPIDI_CH3_ReqHandler_FOPRecvComplete( MPIDI_VC_t *, MPIR_Request *,
                                          int * );
int MPIDI_CH3_ReqHandler_AccumMetadataRecvComplete( MPIDI_VC_t *,
                                                    MPIR_Request *,
                                                    int * );
int MPIDI_CH3_ReqHandler_GaccumMetadataRecvComplete( MPIDI_VC_t *,
                                                     MPIR_Request *,
                                                     int * );
int MPIDI_CH3_ReqHandler_GetDerivedDTRecvComplete( MPIDI_VC_t *,
						   MPIR_Request *, int * );
int MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete( MPIDI_VC_t *,
                                                      MPIR_Request *, int * );
/* Send Handlers */
int MPIDI_CH3_ReqHandler_SendReloadIOV( MPIDI_VC_t *vc, MPIR_Request *sreq,
					int *complete );
int MPIDI_CH3_ReqHandler_GetSendComplete( MPIDI_VC_t *, MPIR_Request *,
                                          int * );
int MPIDI_CH3_ReqHandler_GaccumSendComplete( MPIDI_VC_t *, MPIR_Request *,
                                             int * );
int MPIDI_CH3_ReqHandler_CASSendComplete( MPIDI_VC_t *, MPIR_Request *,
                                          int * );
int MPIDI_CH3_ReqHandler_FOPSendComplete( MPIDI_VC_t *, MPIR_Request *,
                                          int * );
/* RMA operation request handler */
int MPIDI_CH3_Req_handler_rma_op_complete(MPIR_Request *);

#define MPIDI_CH3_GET_EAGER_THRESHOLD(eager_threshold_p, comm, vc)  \
    do {                                                            \
        if ((comm)->dev.eager_max_msg_sz != -1)                     \
            *(eager_threshold_p) = (comm)->dev.eager_max_msg_sz;    \
        else                                                        \
            *(eager_threshold_p) = (vc)->eager_max_msg_sz;          \
    } while (0)


#endif /* !defined(MPICH_MPIDIMPL_H_INCLUDED) */
