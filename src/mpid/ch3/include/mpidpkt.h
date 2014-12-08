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
#define MPIDI_RMA_IMMED_BYTES 16

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

/* *INDENT-OFF* */
/* Indentation turned off because "indent" is getting confused with
 * the lack of a semi-colon in the fields below */
typedef union {
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) c_type_ fop##type_name_;
    MPIR_OP_TYPE_GROUP_ALL_BASIC
    MPIR_OP_TYPE_GROUP_ALL_EXTRA
#undef MPIR_OP_TYPE_MACRO
} MPIDI_CH3_FOP_Immed_u;
/* *INDENT-ON* */

/*
 * Predefined packet types.  This simplifies some of the code.
 */
/* FIXME: Having predefined names makes it harder to add new message types,
   such as different RMA types. */
typedef enum {
    MPIDI_CH3_PKT_EAGER_SEND = 0,
#if defined(USE_EAGER_SHORT)
    MPIDI_CH3_PKT_EAGERSHORT_SEND,
#endif /* defined(USE_EAGER_SHORT) */
    MPIDI_CH3_PKT_EAGER_SYNC_SEND,      /* FIXME: no sync eager */
    MPIDI_CH3_PKT_EAGER_SYNC_ACK,
    MPIDI_CH3_PKT_READY_SEND,
    MPIDI_CH3_PKT_RNDV_REQ_TO_SEND,
    MPIDI_CH3_PKT_RNDV_CLR_TO_SEND,
    MPIDI_CH3_PKT_RNDV_SEND,    /* FIXME: should be stream put */
    MPIDI_CH3_PKT_CANCEL_SEND_REQ,
    MPIDI_CH3_PKT_CANCEL_SEND_RESP,
    /* RMA Packets begin here */
    MPIDI_CH3_PKT_PUT,
    MPIDI_CH3_PKT_GET,
    MPIDI_CH3_PKT_ACCUMULATE,
    MPIDI_CH3_PKT_GET_ACCUM,
    MPIDI_CH3_PKT_FOP,
    MPIDI_CH3_PKT_CAS,
    MPIDI_CH3_PKT_GET_RESP,
    MPIDI_CH3_PKT_GET_ACCUM_RESP,
    MPIDI_CH3_PKT_FOP_RESP,
    MPIDI_CH3_PKT_CAS_RESP,
    MPIDI_CH3_PKT_LOCK,
    MPIDI_CH3_PKT_UNLOCK,
    MPIDI_CH3_PKT_FLUSH,
    MPIDI_CH3_PKT_DECR_AT_COUNTER,
    MPIDI_CH3_PKT_LOCK_ACK,
    MPIDI_CH3_PKT_LOCK_OP_ACK,
    MPIDI_CH3_PKT_FLUSH_ACK,
    /* RMA Packets end here */
    MPIDI_CH3_PKT_FLOW_CNTL_UPDATE,     /* FIXME: Unused */
    MPIDI_CH3_PKT_CLOSE,
    MPIDI_CH3_PKT_REVOKE,
    MPIDI_CH3_PKT_END_CH3,
    /* The channel can define additional types by defining the value
     * MPIDI_CH3_PKT_ENUM */
# if defined(MPIDI_CH3_PKT_ENUM)
    MPIDI_CH3_PKT_ENUM,
# endif
    MPIDI_CH3_PKT_END_ALL,
    MPIDI_CH3_PKT_INVALID = -1  /* forces a signed enum to quash warnings */
} MPIDI_CH3_Pkt_type_t;

/* These flags can be "OR'ed" together */
typedef enum {
    MPIDI_CH3_PKT_FLAG_NONE = 0,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK = 1,
    MPIDI_CH3_PKT_FLAG_RMA_UNLOCK = 2,
    MPIDI_CH3_PKT_FLAG_RMA_FLUSH = 4,
    MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK = 8,
    MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER = 16,
    MPIDI_CH3_PKT_FLAG_RMA_NOCHECK = 32,
    MPIDI_CH3_PKT_FLAG_RMA_SHARED = 64,
    MPIDI_CH3_PKT_FLAG_RMA_EXCLUSIVE = 128,
    MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK = 256,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED = 512,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_QUEUED = 1024,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_DISCARDED = 2048,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED = 4096,
    MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_NO_ACK = 8192,
    MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP = 16384
} MPIDI_CH3_Pkt_flags_t;

typedef struct MPIDI_CH3_Pkt_send {
    MPIDI_CH3_Pkt_type_t type;  /* XXX - uint8_t to conserve space ??? */
    MPIDI_Message_match match;
    MPI_Request sender_req_id;  /* needed for ssend and send cancel */
    MPIDI_msg_sz_t data_sz;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif
} MPIDI_CH3_Pkt_send_t;

/* NOTE: Normal and synchronous eager sends, as well as all ready-mode sends,
   use the same structure but have a different type value. */
typedef MPIDI_CH3_Pkt_send_t MPIDI_CH3_Pkt_eager_send_t;
typedef MPIDI_CH3_Pkt_send_t MPIDI_CH3_Pkt_eager_sync_send_t;
typedef MPIDI_CH3_Pkt_send_t MPIDI_CH3_Pkt_ready_send_t;

#if defined(USE_EAGER_SHORT)
typedef struct MPIDI_CH3_Pkt_eagershort_send {
    MPIDI_CH3_Pkt_type_t type;  /* XXX - uint8_t to conserve space ??? */
    MPIDI_Message_match match;
    MPIDI_msg_sz_t data_sz;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif
    int data[MPIDI_EAGER_SHORT_INTS];   /* FIXME: Experimental for now */
} MPIDI_CH3_Pkt_eagershort_send_t;
#endif /* defined(USE_EAGER_SHORT) */

typedef struct MPIDI_CH3_Pkt_eager_sync_ack {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request sender_req_id;
} MPIDI_CH3_Pkt_eager_sync_ack_t;

typedef MPIDI_CH3_Pkt_send_t MPIDI_CH3_Pkt_rndv_req_to_send_t;

typedef struct MPIDI_CH3_Pkt_rndv_clr_to_send {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request sender_req_id;
    MPI_Request receiver_req_id;
} MPIDI_CH3_Pkt_rndv_clr_to_send_t;

typedef struct MPIDI_CH3_Pkt_rndv_send {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request receiver_req_id;
} MPIDI_CH3_Pkt_rndv_send_t;

typedef struct MPIDI_CH3_Pkt_cancel_send_req {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_Message_match match;
    MPI_Request sender_req_id;
} MPIDI_CH3_Pkt_cancel_send_req_t;

typedef struct MPIDI_CH3_Pkt_cancel_send_resp {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request sender_req_id;
    int ack;
} MPIDI_CH3_Pkt_cancel_send_resp_t;

/* *INDENT-OFF* */
/* Indentation turned off because "indent" is getting confused with
 * the lack of a semi-colon in the field below */
#if defined(MPIDI_CH3_PKT_DEFS)
MPIDI_CH3_PKT_DEFS
#endif
/* *INDENT-ON* */

#define MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(pkt_, datatype_, err_)    \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            datatype_ = (pkt_).put.datatype;                            \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            datatype_ = (pkt_).get.datatype;                            \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            datatype_ = (pkt_).accum.datatype;                          \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            datatype_ = (pkt_).get_accum.datatype;                      \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS):                                       \
            datatype_ = (pkt_).cas.datatype;                            \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
            datatype_ = (pkt_).fop.datatype;                            \
            break;                                                      \
        default:                                                        \
            MPIU_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_TARGET_COUNT(pkt_, count_, err_)          \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            count_ = (pkt_).put.count;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            count_ = (pkt_).get.count;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            count_ = (pkt_).accum.count;                                \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            count_ = (pkt_).get_accum.count;                            \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS):                                       \
        case (MPIDI_CH3_PKT_FOP):                                       \
            count_ = 1;                                                 \
            break;                                                      \
        default:                                                        \
            MPIU_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_IMMED_LEN(pkt_, immed_len_, err_)         \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            immed_len_ = (pkt_).put.immed_len;                          \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            immed_len_ = (pkt_).accum.immed_len;                        \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            immed_len_ = (pkt_).get_accum.immed_len;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
            immed_len_ = (pkt_).fop.immed_len;                          \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS):                                       \
            /* FIXME: we should deal with CAS here */                   \
            break;                                                      \
        default:                                                        \
            MPIU_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_IMMED_DATA_PTR(pkt_, immed_data_, err_)   \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            immed_data_ = (pkt_).put.data;                              \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            immed_data_ = (pkt_).accum.data;                            \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            immed_data_ = (pkt_).get_accum.data;                        \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
            immed_data_ = (pkt_).fop.data;                              \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS):                                       \
            /* FIXME: we should deal with CAS here */                   \
            break;                                                      \
        default:                                                        \
            MPIU_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_LOCK_TYPE(pkt_, lock_type_, err_)         \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            lock_type_ = (pkt_).put.lock_type;                          \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            lock_type_ = (pkt_).get.lock_type;                          \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            lock_type_ = (pkt_).accum.lock_type;                        \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            lock_type_ = (pkt_).get_accum.lock_type;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS):                                       \
            lock_type_ = (pkt_).cas.lock_type;                          \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
            lock_type_ = (pkt_).fop.lock_type;                          \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK):                                      \
            lock_type_ = (pkt_).lock.lock_type;                         \
            break;                                                      \
        default:                                                        \
            MPIU_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_ORIGIN_RANK(pkt_, origin_rank_, err_)     \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            origin_rank_ = (pkt_).put.origin_rank;                      \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            origin_rank_ = (pkt_).get.origin_rank;                      \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            origin_rank_ = (pkt_).accum.origin_rank;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            origin_rank_ = (pkt_).get_accum.origin_rank;                \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS):                                       \
            origin_rank_ = (pkt_).cas.origin_rank;                      \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
            origin_rank_ = (pkt_).fop.origin_rank;                      \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK):                                      \
            origin_rank_ = (pkt_).lock.origin_rank;                     \
            break;                                                      \
        default:                                                        \
            MPIU_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_FLAGS(pkt_, flags_, err_)                 \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            flags_ = (pkt_).put.flags;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            flags_ = (pkt_).get.flags;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            flags_ = (pkt_).accum.flags;                                \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            flags_ = (pkt_).get_accum.flags;                            \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS):                                       \
            flags_ = (pkt_).cas.flags;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
            flags_ = (pkt_).fop.flags;                                  \
            break;                                                      \
        default:                                                        \
            MPIU_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_ERASE_FLAGS(pkt_, err_)                       \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            (pkt_).put.flags = MPIDI_CH3_PKT_FLAG_NONE;                 \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            (pkt_).get.flags = MPIDI_CH3_PKT_FLAG_NONE;                 \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            (pkt_).accum.flags = MPIDI_CH3_PKT_FLAG_NONE;               \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            (pkt_).get_accum.flags = MPIDI_CH3_PKT_FLAG_NONE;           \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS):                                       \
            (pkt_).cas.flags = MPIDI_CH3_PKT_FLAG_NONE;                 \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
            (pkt_).fop.flags = MPIDI_CH3_PKT_FLAG_NONE;                 \
            break;                                                      \
        default:                                                        \
            MPIU_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_SOURCE_WIN_HANDLE(pkt_, win_hdl_, err_)   \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            win_hdl_ = (pkt_).put.source_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            win_hdl_ = (pkt_).get.source_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            win_hdl_ = (pkt_).accum.source_win_handle;                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            win_hdl_ = (pkt_).get_accum.source_win_handle;              \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS):                                       \
            win_hdl_ = (pkt_).cas.source_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
            win_hdl_ = (pkt_).fop.source_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK):                                      \
            win_hdl_ = (pkt_).lock.source_win_handle;                   \
            break;                                                      \
        default:                                                        \
            MPIU_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_TARGET_WIN_HANDLE(pkt_, win_hdl_, err_)   \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            win_hdl_ = (pkt_).put.target_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            win_hdl_ = (pkt_).get.target_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            win_hdl_ = (pkt_).accum.target_win_handle;                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            win_hdl_ = (pkt_).get_accum.target_win_handle;              \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS):                                       \
            win_hdl_ = (pkt_).cas.target_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
            win_hdl_ = (pkt_).fop.target_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK):                                      \
            win_hdl_ = (pkt_).lock.target_win_handle;                   \
            break;                                                      \
        default:                                                        \
            MPIU_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_SET_DATALOOP_SIZE(pkt_, dataloop_size_, err_) \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            (pkt_).put.dataloop_size = (dataloop_size_);                \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            (pkt_).get.dataloop_size = (dataloop_size_);                \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            (pkt_).accum.dataloop_size = (dataloop_size_);              \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            (pkt_).get_accum.dataloop_size = (dataloop_size_);          \
            break;                                                      \
        default:                                                        \
            MPIU_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

/* RMA packets start here */

/********************************************************************************/
/* RMA packet (from origin to target, including PUT, GET, ACC, GACC, CAS, FOP)  */
/********************************************************************************/

typedef struct MPIDI_CH3_Pkt_put {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    /* Followings are to describe target data */
    void *addr;
    int count;
    MPI_Datatype datatype;
    int dataloop_size;
    /* Followings are to piggyback LOCK */
    int lock_type;
    int origin_rank;
    /* Followings are to piggyback IMMED data */
    int immed_len;
    char data[MPIDI_RMA_IMMED_BYTES];
} MPIDI_CH3_Pkt_put_t;

typedef struct MPIDI_CH3_Pkt_get {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    /* Followings are to describe target data */
    void *addr;
    int count;
    MPI_Datatype datatype;
    int dataloop_size;
    /* Following is to complete request at origin */
    MPI_Request request_handle;
    /* Followings are to piggyback LOCK */
    int lock_type;
    int origin_rank;
} MPIDI_CH3_Pkt_get_t;

typedef struct MPIDI_CH3_Pkt_accum {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    /* Followings are to describe target data */
    void *addr;
    int count;
    MPI_Datatype datatype;
    int dataloop_size;
    /* Following is to specify ACC op */
    MPI_Op op;
    /* Followings are to piggyback LOCK */
    int lock_type;
    int origin_rank;
    /* Followings are to piggyback IMMED data */
    int immed_len;
    char data[MPIDI_RMA_IMMED_BYTES];
} MPIDI_CH3_Pkt_accum_t;

typedef struct MPIDI_CH3_Pkt_get_accum {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    /* Followings are to describe target data */
    void *addr;
    int count;
    MPI_Datatype datatype;
    int dataloop_size;
    /* Following is to describe ACC op */
    MPI_Op op;
    /* Following is to complete request on origin */
    MPI_Request request_handle;
    /* Followings are to piggyback LOCK */
    int lock_type;
    int origin_rank;
    /* Followings are to piggback IMMED data */
    int immed_len;
    char data[MPIDI_RMA_IMMED_BYTES];
} MPIDI_CH3_Pkt_get_accum_t;

typedef struct MPIDI_CH3_Pkt_fop {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win source_win_handle;
    MPI_Win target_win_handle;
    /* Followings are to describe target data */
    void *addr;
    MPI_Datatype datatype;
    /* Following is to speicfy ACC op */
    MPI_Op op;
    /* Following is to complete request at origin */
    MPI_Request request_handle;
    /* Followings are to piggyback IMMED data */
    int lock_type;
    int origin_rank;
    /* Followings are to piggyback IMMED data */
    int immed_len;
    char data[MPIDI_RMA_IMMED_BYTES];
} MPIDI_CH3_Pkt_fop_t;

typedef struct MPIDI_CH3_Pkt_cas {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win source_win_handle;
    MPI_Win target_win_handle;
    /* Followings are to describe target data */
    void *addr;
    MPI_Datatype datatype;
    /* Following is to complete request on origin */
    MPI_Request request_handle;
    /* Followings are to piggyback LOCK */
    int lock_type;
    int origin_rank;
    /* Followings are to piggyback IMMED data */
    MPIDI_CH3_CAS_Immed_u origin_data;
    MPIDI_CH3_CAS_Immed_u compare_data;
} MPIDI_CH3_Pkt_cas_t;


/*********************************************************************************/
/* RMA response packet (from target to origin, including GET_RESP, GET_ACC_RESP, */
/* CAS_RESP, FOP_RESP)                                                           */
/*********************************************************************************/

typedef struct MPIDI_CH3_Pkt_get_resp {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    /* Following is to complete request at origin */
    MPI_Request request_handle;
    /* TODO: we should add IMMED data here */
    /* Followings are used to decrement ack_counter at origin */
    MPI_Win source_win_handle;
    int target_rank;
    /* Followings are to piggyback IMMED data */
    int immed_len;
    char data[MPIDI_RMA_IMMED_BYTES];
} MPIDI_CH3_Pkt_get_resp_t;

typedef struct MPIDI_CH3_Pkt_get_accum_resp {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    /* Following is to complete request at origin */
    MPI_Request request_handle;
    /* TODO: we should add IMMED data here */
    /* Followings are used to decrement ack_counter at origin */
    MPI_Win source_win_handle;
    int target_rank;
    /* Followings are to piggyback IMMED data */
    int immed_len;
    char data[MPIDI_RMA_IMMED_BYTES];
} MPIDI_CH3_Pkt_get_accum_resp_t;

typedef struct MPIDI_CH3_Pkt_fop_resp {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    /* Following is to complete request at origin */
    MPI_Request request_handle;
    /* Followings are used to decrement ack_counter at orign */
    MPI_Win source_win_handle;
    int target_rank;
    /* Followings are to piggyback IMMED data */
    int immed_len;
    char data[MPIDI_RMA_IMMED_BYTES];
} MPIDI_CH3_Pkt_fop_resp_t;

typedef struct MPIDI_CH3_Pkt_cas_resp {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    /* Following is to complete request at origin */
    MPI_Request request_handle;
    /* Followings are used to decrement ack_counter at orign */
    MPI_Win source_win_handle;
    int target_rank;
    /* Following is to piggyback IMMED data */
    MPIDI_CH3_CAS_Immed_u data;
} MPIDI_CH3_Pkt_cas_resp_t;

/*********************************************************************************/
/* RMA control packet (from origin to target, including LOCK, UNLOCK, FLUSH)     */
/*********************************************************************************/

typedef struct MPIDI_CH3_Pkt_lock {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    int lock_type;
    int origin_rank;
} MPIDI_CH3_Pkt_lock_t;

typedef struct MPIDI_CH3_Pkt_unlock {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
} MPIDI_CH3_Pkt_unlock_t;

typedef struct MPIDI_CH3_Pkt_flush {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
} MPIDI_CH3_Pkt_flush_t;

typedef struct MPIDI_CH3_Pkt_decr_at_counter {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Win target_win_handle;
} MPIDI_CH3_Pkt_decr_at_counter_t;

/*********************************************************************************/
/* RMA control response packet (from target to origin, including LOCK_ACK,       */
/* LOCK_OP_ACK, FLUSH_ACK)                                                       */
/*********************************************************************************/

typedef struct MPIDI_CH3_Pkt_lock_ack {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win source_win_handle;
    int target_rank;
} MPIDI_CH3_Pkt_lock_ack_t;

typedef struct MPIDI_CH3_Pkt_lock_op_ack {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win source_win_handle;
    int target_rank;
} MPIDI_CH3_Pkt_lock_op_ack_t;

typedef struct MPIDI_CH3_Pkt_flush_ack {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Win source_win_handle;
    int target_rank;
} MPIDI_CH3_Pkt_flush_ack_t;

/* RMA packets end here */

typedef struct MPIDI_CH3_Pkt_close {
    MPIDI_CH3_Pkt_type_t type;
    int ack;
} MPIDI_CH3_Pkt_close_t;

typedef struct MPIDI_CH3_Pkt_revoke {
    MPIDI_CH3_Pkt_type_t type;
    MPIR_Context_id_t revoked_comm;
} MPIDI_CH3_Pkt_revoke_t;

typedef union MPIDI_CH3_Pkt {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_eager_send_t eager_send;
#if defined(USE_EAGER_SHORT)
    MPIDI_CH3_Pkt_eagershort_send_t eagershort_send;
#endif                          /* defined(USE_EAGER_SHORT) */
    MPIDI_CH3_Pkt_eager_sync_send_t eager_sync_send;
    MPIDI_CH3_Pkt_eager_sync_ack_t eager_sync_ack;
    MPIDI_CH3_Pkt_eager_send_t ready_send;
    MPIDI_CH3_Pkt_rndv_req_to_send_t rndv_req_to_send;
    MPIDI_CH3_Pkt_rndv_clr_to_send_t rndv_clr_to_send;
    MPIDI_CH3_Pkt_rndv_send_t rndv_send;
    MPIDI_CH3_Pkt_cancel_send_req_t cancel_send_req;
    MPIDI_CH3_Pkt_cancel_send_resp_t cancel_send_resp;
    /* RMA packets start here */
    MPIDI_CH3_Pkt_put_t put;
    MPIDI_CH3_Pkt_get_t get;
    MPIDI_CH3_Pkt_accum_t accum;
    MPIDI_CH3_Pkt_get_accum_t get_accum;
    MPIDI_CH3_Pkt_fop_t fop;
    MPIDI_CH3_Pkt_cas_t cas;
    MPIDI_CH3_Pkt_get_resp_t get_resp;
    MPIDI_CH3_Pkt_get_accum_resp_t get_accum_resp;
    MPIDI_CH3_Pkt_fop_resp_t fop_resp;
    MPIDI_CH3_Pkt_cas_resp_t cas_resp;
    MPIDI_CH3_Pkt_lock_t lock;
    MPIDI_CH3_Pkt_unlock_t unlock;
    MPIDI_CH3_Pkt_flush_t flush;
    MPIDI_CH3_Pkt_decr_at_counter_t decr_at_cnt;
    MPIDI_CH3_Pkt_lock_ack_t lock_ack;
    MPIDI_CH3_Pkt_lock_op_ack_t lock_op_ack;
    MPIDI_CH3_Pkt_flush_ack_t flush_ack;
    /* RMA packets end here */
    MPIDI_CH3_Pkt_close_t close;
    MPIDI_CH3_Pkt_revoke_t revoke;
# if defined(MPIDI_CH3_PKT_DECL)
     MPIDI_CH3_PKT_DECL
# endif
} MPIDI_CH3_Pkt_t;

#if defined(MPID_USE_SEQUENCE_NUMBERS)
typedef struct MPIDI_CH3_Pkt_send_container {
    MPIDI_CH3_Pkt_send_t pkt;
    struct MPIDI_CH3_Pkt_send_container_s *next;
} MPIDI_CH3_Pkt_send_container_t;
#endif

#endif
