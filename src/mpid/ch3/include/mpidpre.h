/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* FIXME: This header should contain only the definitions exported to the
   mpiimpl.h level */

#if !defined(MPICH_MPIDPRE_H_INCLUDED)
#define MPICH_MPIDPRE_H_INCLUDED

/* Tell the compiler that we're going to declare struct MPID_Request later */
struct MPID_Request;

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

/* The maximum message size is the size of a pointer; this allows MPI_Aint 
   to be larger than a pointer */
typedef MPIR_Pint MPIDI_msg_sz_t;

#include "mpid_dataloop.h"

/* FIXME: Include here? */
#include "opa_primitives.h"

union MPIDI_CH3_Pkt;
struct MPIDI_VC;
struct MPID_Request;

/* PktHandler function:
   vc  (INPUT) -- vc on which the packet was received
   pkt (INPUT) -- pointer to packet header at beginning of receive buffer
   buflen (I/O) -- IN: number of bytes received into receive buffer
                   OUT: number of bytes processed by the handler function
   req (OUTPUT) -- NULL, if the whole message has been processed by the handler
                   function, otherwise, pointer to the receive request for this
                   message.  The IOV will be set describing where the rest of the
                   message should be received.
   (This decl needs to come before mpidi_ch3_pre.h)
*/
typedef int MPIDI_CH3_PktHandler_Fcn(struct MPIDI_VC *vc, union MPIDI_CH3_Pkt *pkt,
				     MPIDI_msg_sz_t *buflen, struct MPID_Request **req );

/* Include definitions from the channel which must exist before items in this 
   file (mpidpre.h) or the file it includes (mpiimpl.h) can be defined. */
#include "mpidi_ch3_pre.h"

/* FIXME: Who defines this name */
/* As of 8/1/06, no-one defined MSGS_UNORDERED.  We should consider 
   moving support for unordered messages to a different part of the code
   However, note that sequence numbers may be useful in other contexts, 
   including identifying messages when multithreaded (for better profiling
   tools) and handling cancellations (rather than relying on unique 
   request ids) 
*/
#if defined (MPIDI_CH3_MSGS_UNORDERED)
#define MPID_USE_SEQUENCE_NUMBERS
#endif

#if defined(MPID_USE_SEQUENCE_NUMBERS)
typedef unsigned long MPID_Seqnum_t;
#endif

#include "mpichconf.h"

#if CH3_RANK_BITS == 16
typedef int16_t MPIR_Rank_t;
#elif CH3_RANK_BITS == 32
typedef int32_t MPIR_Rank_t;
#endif /* CH3_RANK_BITS */

/* Indicates that this device is topology aware and implements the
   MPID_Get_node_id function (and friends). */
#define MPID_USE_NODE_IDS
typedef MPIR_Rank_t MPID_Node_id_t;


/* provides "pre" typedefs and such for NBC scheduling mechanism */
#include "mpid_sched_pre.h"

/* For the typical communication system for which the ch3 channel is
   appropriate, 16 bits is sufficient for the rank.  By also using 16
   bits for the context, we can reduce the size of the match
   information, which is beneficial for slower communication
   links. Further, this allows the total structure size to be 64 bits
   and the search operations can be optimized on 64-bit platforms. We
   use a union of the actual required structure with a MPIR_Upint, so
   in this optimized case, the "whole" field can be used for
   comparisons.

   Note that the MPICH code (in src/mpi) uses int for rank (and usually for 
   contextids, though some work is needed there).  

   Note:  We need to check for truncation of rank in MPID_Init - it should 
   confirm that the size of comm_world is less than 2^15, and in an communicator
   create (that may make use of dynamically created processes) that the
   size of the communicator is within range.

   If any part of the definition of this type is changed, those changes
   must be reflected in the debugger interface in src/mpi/debugger/dll_mpich.c
   and dbgstub.c
*/
typedef struct MPIDI_Message_match_parts {
    int32_t tag;
    MPIR_Rank_t rank;
    MPIR_Context_id_t context_id;
} MPIDI_Message_match_parts_t;
typedef union {
    MPIDI_Message_match_parts_t parts;
    MPIR_Upint whole;
} MPIDI_Message_match;

/* Provides MPIDI_CH3_Pkt_t.  Must come after MPIDI_Message_match definition. */
#include "mpidpkt.h"

/*
 * THIS IS OBSOLETE AND UNUSED, BUT RETAINED FOR ITS DESCRIPTIONS OF THE
 * VARIOUS STATES.  Note that this is not entirely accurate, as the 
 * CA_COMPLETE state could depend on the packet type (e.g., for RMA 
 * operations).
 *
 * MPIDI_CA_t
 *
 * An enumeration of the actions to perform when the requested I/O operation 
 * has completed.
 *
 * MPIDI_CH3_CA_COMPLETE - The last operation for this request has completed.
 * The completion counter should be decremented.  If
 * it has reached zero, then the request should be released by calling 
 * MPID_Request_release().
 *
 * MPIDI_CH3_CA_UNPACK_UEBUF_AND_COMPLETE - This is a special case of the 
 * MPIDI_CH3_CA_COMPLETE.  The data for an unexpected
 * eager messaage has been stored into a temporary buffer and needs to be 
 * copied/unpacked into the user buffer before the
 * completion counter can be decremented, etc.
 *
 * MPIDI_CH3_CA_UNPACK_SRBUF_AND_COMPLETE - This is a special case of the 
 * MPIDI_CH3_CA_COMPLETE.  The data from the completing
 * read has been stored into a temporary send/receive buffer and needs to be 
 * copied/unpacked into the user buffer before the
 * completion counter can be decremented, etc.
 *
 * MPIDI_CH3_CA_RELOAD_IOV - This request contains more segments of data than 
 * the IOV or buffer space allow.  Since the
 * previously request operation has completed, the IOV in the request should 
 * be reload at this time.
 *
 * MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV - This is a special case of the 
 * MPIDI_CH3_CA_RELOAD_IOV.  The data from the
 * completing read operation has been stored into a temporary send/receive 
 * buffer and needs to be copied/unpacked into the user
 * buffer before the IOV is reloaded.
 *
 * MPIDI_CH3_CA_END_CH3 - This not a real action, but rather a marker.  
 * All actions numerically less than MPID_CA_END are defined
 * by channel device.  Any actions numerically greater than MPIDI_CA_END are 
 * internal to the channel instance and must be handled
 * by the channel instance.
 */

#define HAVE_DEV_COMM_HOOK
#define MPID_Dev_comm_create_hook(comm_) MPIDI_CH3I_Comm_create_hook(comm_)
#define MPID_Dev_comm_destroy_hook(comm_) MPIDI_CH3I_Comm_destroy_hook(comm_)

#define MPIDI_CH3I_Comm_AS_enabled(comm) ((comm)->dev.anysource_enabled)

typedef struct MPIDI_CH3I_comm
{
    int eager_max_msg_sz;   /* comm-wide eager/rendezvous message threshold */
    int anysource_enabled;  /* TRUE iff this anysource recvs can be posted on this communicator */
    int last_ack_rank;      /* The rank of the last acknowledged failure */
    int waiting_for_revoke; /* The number of other processes from which we are
                             * waiting for a revoke message before we can release
                             * the context id */
    struct MPID_Comm *next; /* next pointer for list of communicators */
    struct MPID_Comm *prev; /* prev pointer for list of communicators */
    MPIDI_CH3I_CH_comm_t ch;
}
MPIDI_CH3I_comm_t;

#define MPID_DEV_COMM_DECL MPIDI_CH3I_comm_t dev;

#ifndef HAVE_MPIDI_VCRT
#define HAVE_MPIDI_VCRT
typedef struct MPIDI_VCRT * MPID_VCRT;
typedef struct MPIDI_VC * MPID_VCR;
#endif

#ifndef DEFINED_REQ
#define DEFINED_REQ
#if defined(MPID_USE_SEQUENCE_NUMBERS)
#   define MPIDI_REQUEST_SEQNUM	\
        MPID_Seqnum_t seqnum;
#else
#   define MPIDI_REQUEST_SEQNUM
#endif

/* We start with an arbitrarily chosen number (42), to help with
 * debugging when a packet type is not initialized or wrongly
 * initialized. */
enum MPIDI_CH3_Lock_states {
    MPIDI_CH3_WIN_LOCK_NONE = 42,
    MPIDI_CH3_WIN_LOCK_CALLED,
    MPIDI_CH3_WIN_LOCK_REQUESTED,
    MPIDI_CH3_WIN_LOCK_GRANTED,
    MPIDI_CH3_WIN_LOCK_FLUSH
};

enum MPIDI_Win_info_arv_vals_accumulate_ordering {
    MPIDI_ACC_ORDER_RAR = 1,
    MPIDI_ACC_ORDER_RAW = 2,
    MPIDI_ACC_ORDER_WAR = 4,
    MPIDI_ACC_ORDER_WAW = 8
};

enum MPIDI_Win_info_arg_vals_accumulate_ops {
    MPIDI_ACC_OPS_SAME_OP,
    MPIDI_ACC_OPS_SAME_OP_NO_OP
};

/* We start with an arbitrarily chosen number (42), to help with
 * debugging when a packet type is not initialized or wrongly
 * initialized. */
enum MPIDI_Win_epoch_states {
    MPIDI_EPOCH_NONE = 42,
    MPIDI_EPOCH_FENCE,
    MPIDI_EPOCH_POST,
    MPIDI_EPOCH_START,
    MPIDI_EPOCH_PSCW,           /* Both post and start have been called. */
    MPIDI_EPOCH_LOCK,
    MPIDI_EPOCH_LOCK_ALL
};

struct MPIDI_Win_info_args {
    int no_locks;               /* valid flavor = all */
    int accumulate_ordering;
    int accumulate_ops;
    int same_size;              /* valid flavor = allocate */
    int alloc_shared_noncontig; /* valid flavor = allocate shared */
    int alloc_shm;              /* valid flavor = allocate */
};

struct MPIDI_RMA_op;            /* forward decl from mpidrma.h */

struct MPIDI_Win_target_state {
    struct MPIDI_RMA_Op *rma_ops_list;
                                /* List of outstanding RMA operations */
    volatile enum MPIDI_CH3_Lock_states remote_lock_state;
                                /* Indicates the state of the target
                                   process' "lock" for passive target
                                   RMA. */
    int remote_lock_mode;       /* Indicates the access mode
                                   (shared/exclusive) of the target
                                   process for passive target RMA. Valid
                                   whenever state != NONE. */
    int remote_lock_assert;     /* Assertion value provided in the call
                                   to Lock */
};

#define MPIDI_DEV_WIN_DECL                                               \
    volatile int at_completion_counter;  /* completion counter for operations \
                                 targeting this window */                \
    void **base_addrs;     /* array of base addresses of the windows of  \
                              all processes */                           \
    void **shm_base_addrs; /* shared memory windows -- array of base     \
                              addresses of the windows of all processes  \
                              in this process's address space */         \
    int *disp_units;      /* array of displacement units of all windows */\
    MPI_Win *all_win_handles;    /* array of handles to the window objects\
                                          of all processes */            \
    volatile int current_lock_type;   /* current lock type on this window (as target)   \
                              * (none, shared, exclusive) */             \
    volatile int shared_lock_ref_cnt;                                    \
    struct MPIDI_Win_lock_queue volatile *lock_queue;  /* list of unsatisfied locks */  \
                                                                         \
    int *pt_rma_puts_accs;  /* array containing the no. of passive target\
                               puts/accums issued from this process to other \
                               processes. */                             \
    volatile int my_pt_rma_puts_accs;  /* no. of passive target puts/accums  \
                                          that this process has          \
                                          completed as target */         \
    MPI_Aint *sizes;      /* array of sizes of all windows */            \
    struct MPIDI_Win_info_args info_args;                                \
    struct MPIDI_Win_target_state *targets; /* Target state and ops      \
                                               lists for passive target  \
                                               mode of operation */      \
    struct MPIDI_RMA_Op *at_rma_ops_list; /* Ops list for active target  \
                                             mode of operation. */       \
    enum MPIDI_Win_epoch_states epoch_state;                             \
    int epoch_count;                                                     \
    int fence_issued;   /* Indicates if fence has been called, and if an \
                           active target fence epoch is possible. This   \
                           is maintained separately from the epoch state;\
                           this state must be updated collectively (in   \
                           fence) to ensure that the fence state across  \
                           all processes remains consistent. */          \
    MPID_Group *start_group_ptr; /* group passed in MPI_Win_start */     \
    int start_assert;   /* assert passed to MPI_Win_start */             \
    int shm_allocated; /* flag: TRUE iff this window has a shared memory \
                          region associated with it */                   \

#ifdef MPIDI_CH3_WIN_DECL
#define MPID_DEV_WIN_DECL \
MPIDI_DEV_WIN_DECL \
MPIDI_CH3_WIN_DECL
#else
#define MPID_DEV_WIN_DECL \
MPIDI_DEV_WIN_DECL
#endif


typedef struct MPIDI_Request {
    MPIDI_Message_match match;
    MPIDI_Message_match mask;

    /* user_buf, user_count, and datatype needed to process 
       rendezvous messages. */
    void        *user_buf;
    int          user_count;
    MPI_Datatype datatype;

    /* segment, segment_first, and segment_size are used when processing 
       non-contiguous datatypes */
    /*    MPID_Segment   segment; */
    struct MPID_Segment *segment_ptr;
    MPIDI_msg_sz_t segment_first;
    MPIDI_msg_sz_t segment_size;

    /* Pointer to datatype for reference counting purposes */
    struct MPID_Datatype * datatype_ptr;

    /* iov and iov_count define the data to be transferred/received.  
       iov_offset points to the current head element in the IOV */
    MPID_IOV iov[MPID_IOV_LIMIT];
    int iov_count;
    size_t iov_offset;

    /* OnDataAvail is the action to take when data is now available.
       For example, when an operation described by an iov has 
       completed.  This replaces the MPIDI_CA_t (completion action)
       field used through MPICH 1.0.4. */
    int (*OnDataAvail)( struct MPIDI_VC *, struct MPID_Request *, int * );
    /* OnFinal is used in the following case:
       OnDataAvail is set to a function, and that function has processed
       all of the data.  At that point, the OnDataAvail function can
       reset OnDataAvail to OnFinal.  This is normally used when processing
       non-contiguous data, where there is one more action to take (such
       as a get-response) when processing of the non-contiguous data 
       completes. This value need not be initialized unless OnDataAvail
       is set to a non-null value (and then only in certain cases) */
    int (*OnFinal)( struct MPIDI_VC *, struct MPID_Request *, int * );

    /* tmpbuf and tmpbuf_sz describe temporary storage used for things like 
       unexpected eager messages and packing/unpacking
       buffers.  tmpuf_off is the current offset into the temporary buffer. */
    void          *tmpbuf;
    MPIDI_msg_sz_t tmpbuf_off;
    MPIDI_msg_sz_t tmpbuf_sz;

    MPIDI_msg_sz_t recv_data_sz;
    MPI_Request    sender_req_id;

    unsigned int   state;
    int            cancel_pending;

    /* This field seems to be used for unexpected messages.  Unexpected messages
     * need to go through two steps: matching and receiving the data.  These
     * steps could happen in either order though, so this field is initialized
     * to 2.  It is decremented when the request is matched and also when all of
     * the data is available.  Once it reaches 0 it should be safe to copy from
     * the temporary buffer (if there is one) to the user buffer.  This field is
     * related to, but not quite the same thing as the completion counter (cc). */
    /* MT access should be controlled by the MSGQUEUE CS when the req is still
     * unexpected, exclusive access otherwise */
    int            recv_pending_count;

    /* The next several fields are used to hold state for ongoing RMA operations */
    MPI_Op op;
    /* For accumulate, since data is first read into a tmp_buf */
    void *real_user_buf;
    /* For derived datatypes at target */
    struct MPIDI_RMA_dtype_info *dtype_info;
    void *dataloop;
    /* req. handle needed to implement derived datatype gets  */
    MPI_Request request_handle;
    MPI_Win     target_win_handle;
    MPI_Win     source_win_handle;
    MPIDI_CH3_Pkt_flags_t flags; /* flags that were included in the original RMA packet header */
    struct MPIDI_Win_lock_queue *lock_queue_entry; /* for single lock-put-unlock optimization */
    MPI_Request resp_request_handle; /* Handle for get_accumulate response */

    MPIDI_REQUEST_SEQNUM

    /* Occasionally, when a message cannot be sent, we need to cache the
       data that is required.  The fields above (such as userbuf and tmpbuf)
       are used for the message data.  However, we also need space for the
       message packet. This field provide a generic location for that.
       Question: do we want to make this a link instead of reserving 
       a fixed spot in the request? */
    MPIDI_CH3_Pkt_t pending_pkt;
    struct MPID_Request * next;
} MPIDI_Request;
#define MPID_REQUEST_DECL MPIDI_Request dev;

#if defined(MPIDI_CH3_REQUEST_DECL)
#define MPID_DEV_REQUEST_DECL			\
MPID_REQUEST_DECL				\
MPIDI_CH3_REQUEST_DECL
#else
#define MPID_DEV_REQUEST_DECL			\
MPID_REQUEST_DECL
#endif

#ifdef MPIDI_CH3_REQUEST_KIND_DECL
#define MPID_DEV_REQUEST_KIND_DECL MPIDI_CH3_REQUEST_KIND_DECL
#endif

#endif

/* FIXME: This ifndef test is a temp until mpidpre is cleaned of
   all items that do not belong (e.g., all items not needed by the
   top layers of MPICH) */
/* FIXME: The progress routines will be made into ch3-common definitions, not
   channel specific.  Channels that need more will need to piggy back or 
   otherwise override */
#ifndef MPID_PROGRESS_STATE_DECL
#if defined(MPIDI_CH3_PROGRESS_STATE_DECL)
#   define MPID_PROGRESS_STATE_DECL MPIDI_CH3_PROGRESS_STATE_DECL
#else
#   define MPID_PROGRESS_STATE_DECL int foo;
#endif
#endif


/* Tell Intercomm create and friends that the GPID routines have been
   implemented */
#define HAVE_GPID_ROUTINES

/* Tell initthread to prepare a private comm_world */
#define MPID_NEEDS_ICOMM_WORLD

/* Tell the RMA code to use a table of RMA functions provided by the 
   ADI */
#define USE_MPID_RMA_TABLE
#endif /* !defined(MPICH_MPIDPRE_H_INCLUDED) */
