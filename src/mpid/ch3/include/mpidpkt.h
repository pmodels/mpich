/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HAVE_MPIDPKT_H
#define HAVE_MPIDPKT_H

#include "oputil.h"

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif

/* Enable the use of data within the message packet for small messages */
#define USE_EAGER_SHORT
#define MPIDI_EAGER_SHORT_INTS 4
/* FIXME: This appears to assume that sizeof(int) == 4 (or at least >= 4) */
#define MPIDI_EAGER_SHORT_SIZE 16

/* This is the number of ints that can be carried within an RMA packet */
#define MPIDI_RMA_IMMED_INTS 1

/* Union over all types (integer, logical, and multi-language types) that are
   allowed in a CAS operation.  This is used to allocate enough space in the
   packet header for immediate data.  */
typedef union {
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) c_type_ cas_##type_name_;
        MPIR_OP_TYPE_GROUP(C_INTEGER)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
        MPIR_OP_TYPE_GROUP(LOGICAL)
        MPIR_OP_TYPE_GROUP(BYTE)
        MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
        MPIR_OP_TYPE_GROUP(LOGICAL_EXTRA)
        MPIR_OP_TYPE_GROUP(BYTE_EXTRA)
#undef MPIR_OP_TYPE_MACRO
} MPIDI_CH3_CAS_Immed_u;

/* Union over all types (all predefined types) that are allowed in a
   Fetch-and-op operation.  This can be too large for the packet header, so we
   limit the immediate space in the header to FOP_IMMED_INTS. */

#define MPIDI_RMA_FOP_IMMED_INTS 2
#define MPIDI_RMA_FOP_RESP_IMMED_INTS 8

typedef union {
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) c_type_ fop##type_name_;
        MPIR_OP_TYPE_GROUP_ALL_BASIC
        MPIR_OP_TYPE_GROUP_ALL_EXTRA
#undef MPIR_OP_TYPE_MACRO
} MPIDI_CH3_FOP_Immed_u;

/*
 * Predefined packet types.  This simplifies some of the code.
 */
/* FIXME: Having predefined names makes it harder to add new message types,
   such as different RMA types. */
/* We start with an arbitrarily chosen number (42), to help with
 * debugging when a packet type is not initialized or wrongly
 * initialized. */
typedef enum
{
    MPIDI_CH3_PKT_EAGER_SEND = 42,
#if defined(USE_EAGER_SHORT)
    MPIDI_CH3_PKT_EAGERSHORT_SEND,
#endif /* defined(USE_EAGER_SHORT) */
    MPIDI_CH3_PKT_EAGER_SYNC_SEND,    /* FIXME: no sync eager */
    MPIDI_CH3_PKT_EAGER_SYNC_ACK,
    MPIDI_CH3_PKT_READY_SEND,
    MPIDI_CH3_PKT_RNDV_REQ_TO_SEND,
    MPIDI_CH3_PKT_RNDV_CLR_TO_SEND,
    MPIDI_CH3_PKT_RNDV_SEND,          /* FIXME: should be stream put */
    MPIDI_CH3_PKT_CANCEL_SEND_REQ,
    MPIDI_CH3_PKT_CANCEL_SEND_RESP,
    MPIDI_CH3_PKT_PUT,                /* RMA Packets begin here */
    MPIDI_CH3_PKT_GET,
    MPIDI_CH3_PKT_GET_RESP,
    MPIDI_CH3_PKT_ACCUMULATE,
    MPIDI_CH3_PKT_LOCK,
    MPIDI_CH3_PKT_LOCK_GRANTED,
    MPIDI_CH3_PKT_UNLOCK,
    MPIDI_CH3_PKT_FLUSH,
    MPIDI_CH3_PKT_PT_RMA_DONE,
    MPIDI_CH3_PKT_LOCK_PUT_UNLOCK, /* optimization for single puts */
    MPIDI_CH3_PKT_LOCK_GET_UNLOCK, /* optimization for single gets */
    MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK, /* optimization for single accumulates */
                                     /* RMA Packets end here */
    MPIDI_CH3_PKT_ACCUM_IMMED,     /* optimization for short accumulate */
    /* FIXME: Add PUT, GET_IMMED packet types */
    MPIDI_CH3_PKT_CAS,
    MPIDI_CH3_PKT_CAS_RESP,
    MPIDI_CH3_PKT_FOP,
    MPIDI_CH3_PKT_FOP_RESP,
    MPIDI_CH3_PKT_GET_ACCUM,
    MPIDI_CH3_PKT_GET_ACCUM_RESP,
    MPIDI_CH3_PKT_FLOW_CNTL_UPDATE,  /* FIXME: Unused */
    MPIDI_CH3_PKT_CLOSE,
    MPIDI_CH3_PKT_REVOKE,
    MPIDI_CH3_PKT_END_CH3,
    /* The channel can define additional types by defining the value
       MPIDI_CH3_PKT_ENUM */
# if defined(MPIDI_CH3_PKT_ENUM)
    MPIDI_CH3_PKT_ENUM,
# endif    
    MPIDI_CH3_PKT_END_ALL,
    MPIDI_CH3_PKT_INVALID = -1 /* forces a signed enum to quash warnings */
} MPIDI_CH3_Pkt_type_t;

typedef enum {
    MPIDI_CH3_PKT_FLAG_NONE = 0,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK = 1,
    MPIDI_CH3_PKT_FLAG_RMA_UNLOCK = 2,
    MPIDI_CH3_PKT_FLAG_RMA_FLUSH = 4,
    MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK = 8,
    MPIDI_CH3_PKT_FLAG_RMA_AT_COMPLETE = 16,
    MPIDI_CH3_PKT_FLAG_RMA_NOCHECK = 32,
    MPIDI_CH3_PKT_FLAG_RMA_SHARED = 64,
    MPIDI_CH3_PKT_FLAG_RMA_EXCLUSIVE = 128
} MPIDI_CH3_Pkt_flags_t;

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

/* NOTE: Normal and synchronous eager sends, as well as all ready-mode sends, 
   use the same structure but have a different type value. */
typedef MPIDI_CH3_Pkt_send_t MPIDI_CH3_Pkt_eager_send_t;
typedef MPIDI_CH3_Pkt_send_t MPIDI_CH3_Pkt_eager_sync_send_t;
typedef MPIDI_CH3_Pkt_send_t MPIDI_CH3_Pkt_ready_send_t;

#if defined(USE_EAGER_SHORT)
typedef struct MPIDI_CH3_Pkt_eagershort_send
{
    MPIDI_CH3_Pkt_type_t type;  /* XXX - uint8_t to conserve space ??? */
    MPIDI_Message_match match;
    MPIDI_msg_sz_t data_sz;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif
    int  data[MPIDI_EAGER_SHORT_INTS];    /* FIXME: Experimental for now */
}
MPIDI_CH3_Pkt_eagershort_send_t;
#endif /* defined(USE_EAGER_SHORT) */

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
    MPIDI_CH3_Pkt_flags_t flags;
    void *addr;
    int count;
    MPI_Datatype datatype;
    int dataloop_size;   /* for derived datatypes */
    MPI_Win target_win_handle; /* Used in the last RMA operation in each
                               * epoch for decrementing rma op counter in
                               * active target rma and for unlocking window 
                               * in passive target rma. Otherwise set to NULL*/
    MPI_Win source_win_handle; /* Used in the last RMA operation in an
                               * epoch in the case of passive target rma
                               * with shared locks. Otherwise set to NULL*/
}
MPIDI_CH3_Pkt_put_t;

typedef struct MPIDI_CH3_Pkt_get
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    void *addr;
    int count;
    MPI_Datatype datatype;
    int dataloop_size;   /* for derived datatypes */
    MPI_Request request_handle;
    MPI_Win target_win_handle; /* Used in the last RMA operation in each
                               * epoch for decrementing rma op counter in
                               * active target rma and for unlocking window 
                               * in passive target rma. Otherwise set to NULL*/
    MPI_Win source_win_handle; /* Used in the last RMA operation in an
                               * epoch in the case of passive target rma
                               * with shared locks. Otherwise set to NULL*/
}
MPIDI_CH3_Pkt_get_t;

typedef struct MPIDI_CH3_Pkt_get_resp
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request request_handle;
}
MPIDI_CH3_Pkt_get_resp_t;

typedef struct MPIDI_CH3_Pkt_accum
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Request request_handle; /* For get_accumulate response */
    void *addr;
    int count;
    MPI_Datatype datatype;
    int dataloop_size;   /* for derived datatypes */
    MPI_Op op;
    MPI_Win target_win_handle; /* Used in the last RMA operation in each
                               * epoch for decrementing rma op counter in
                               * active target rma and for unlocking window 
                               * in passive target rma. Otherwise set to NULL*/
    MPI_Win source_win_handle; /* Used in the last RMA operation in an
                               * epoch in the case of passive target rma
                               * with shared locks. Otherwise set to NULL*/
}
MPIDI_CH3_Pkt_accum_t;

typedef struct MPIDI_CH3_Pkt_get_accum_resp
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request request_handle;
}
MPIDI_CH3_Pkt_get_accum_resp_t;

typedef struct MPIDI_CH3_Pkt_accum_immed
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    void *addr;
    int count;
    /* FIXME: Compress datatype/op into a single word (immedate mode) */
    MPI_Datatype datatype;
    MPI_Op op;
    /* FIXME: do we need these (use a regular accum packet if we do?) */
    MPI_Win target_win_handle; /* Used in the last RMA operation in each
                               * epoch for decrementing rma op counter in
                               * active target rma and for unlocking window 
                               * in passive target rma. Otherwise set to NULL*/
    MPI_Win source_win_handle; /* Used in the last RMA operation in an
                               * epoch in the case of passive target rma
                               * with shared locks. Otherwise set to NULL*/
    int data[MPIDI_RMA_IMMED_INTS];
}
MPIDI_CH3_Pkt_accum_immed_t;

typedef struct MPIDI_CH3_Pkt_cas
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Datatype datatype;
    void *addr;
    MPI_Request request_handle;
    MPI_Win target_win_handle; /* Used in the last RMA operation in each
                                * epoch for decrementing rma op counter in
                                * active target rma and for unlocking window 
                                * in passive target rma. Otherwise set to NULL*/
    MPIDI_CH3_CAS_Immed_u origin_data;
    MPIDI_CH3_CAS_Immed_u compare_data;
}
MPIDI_CH3_Pkt_cas_t;

typedef struct MPIDI_CH3_Pkt_cas_resp
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request request_handle;
    MPIDI_CH3_CAS_Immed_u data;
}
MPIDI_CH3_Pkt_cas_resp_t;

typedef struct MPIDI_CH3_Pkt_fop
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Datatype datatype;
    void *addr;
    MPI_Op op;
    MPI_Request request_handle;
    MPI_Win target_win_handle; /* Used in the last RMA operation in each
                                * epoch for decrementing rma op counter in
                                * active target rma and for unlocking window 
                                * in passive target rma. Otherwise set to NULL*/
    int origin_data[MPIDI_RMA_FOP_IMMED_INTS];
}
MPIDI_CH3_Pkt_fop_t;

typedef struct MPIDI_CH3_Pkt_fop_resp
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request request_handle;
    int data[MPIDI_RMA_FOP_RESP_IMMED_INTS];
}
MPIDI_CH3_Pkt_fop_resp_t;

typedef struct MPIDI_CH3_Pkt_lock
{
    MPIDI_CH3_Pkt_type_t type;
    int lock_type;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    int target_rank;            /* Used in unluck/flush response to look up the
                                   target state at the origin. */
}
MPIDI_CH3_Pkt_lock_t;

typedef struct MPIDI_CH3_Pkt_lock_granted
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Win source_win_handle;
    int target_rank;            /* Used in pt_rma_done response to look up the
                                   target state at the origin. */
}
MPIDI_CH3_Pkt_lock_granted_t;

typedef MPIDI_CH3_Pkt_lock_granted_t MPIDI_CH3_Pkt_pt_rma_done_t;
typedef MPIDI_CH3_Pkt_lock_t MPIDI_CH3_Pkt_unlock_t;
typedef MPIDI_CH3_Pkt_lock_t MPIDI_CH3_Pkt_flush_t;

typedef struct MPIDI_CH3_Pkt_lock_put_unlock
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    int lock_type;
    void *addr;
    int count;
    MPI_Datatype datatype;
}
MPIDI_CH3_Pkt_lock_put_unlock_t;

typedef struct MPIDI_CH3_Pkt_lock_get_unlock
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    int lock_type;
    void *addr;
    int count;
    MPI_Datatype datatype;
    MPI_Request request_handle;
}
MPIDI_CH3_Pkt_lock_get_unlock_t;

typedef struct MPIDI_CH3_Pkt_lock_accum_unlock
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    int lock_type;
    void *addr;
    int count;
    MPI_Datatype datatype;
    MPI_Op op;
}
MPIDI_CH3_Pkt_lock_accum_unlock_t;


typedef struct MPIDI_CH3_Pkt_close
{
    MPIDI_CH3_Pkt_type_t type;
    int ack;
}
MPIDI_CH3_Pkt_close_t;

typedef struct MPIDI_CH3_Pkt_revoke
{
    MPIDI_CH3_Pkt_type_t type;
    MPIR_Context_id_t revoked_comm;
}
MPIDI_CH3_Pkt_revoke_t;

typedef union MPIDI_CH3_Pkt
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_eager_send_t eager_send;
#if defined(USE_EAGER_SHORT)
    MPIDI_CH3_Pkt_eagershort_send_t eagershort_send;
#endif /* defined(USE_EAGER_SHORT) */
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
    MPIDI_CH3_Pkt_accum_immed_t accum_immed;
    MPIDI_CH3_Pkt_lock_t lock;
    MPIDI_CH3_Pkt_lock_granted_t lock_granted;
    MPIDI_CH3_Pkt_unlock_t unlock;
    MPIDI_CH3_Pkt_flush_t flush;
    MPIDI_CH3_Pkt_pt_rma_done_t pt_rma_done;    
    MPIDI_CH3_Pkt_lock_put_unlock_t lock_put_unlock;
    MPIDI_CH3_Pkt_lock_get_unlock_t lock_get_unlock;
    MPIDI_CH3_Pkt_lock_accum_unlock_t lock_accum_unlock;
    MPIDI_CH3_Pkt_close_t close;
    MPIDI_CH3_Pkt_cas_t cas;
    MPIDI_CH3_Pkt_cas_resp_t cas_resp;
    MPIDI_CH3_Pkt_fop_t fop;
    MPIDI_CH3_Pkt_fop_resp_t fop_resp;
    MPIDI_CH3_Pkt_get_accum_resp_t get_accum_resp;
    MPIDI_CH3_Pkt_revoke_t revoke;
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

#endif
