/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HAVE_MPIDPKT_H
#define HAVE_MPIDPKT_H

#include "oputil.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

/* Enable the use of data within the message packet for small messages */
#define USE_EAGER_SHORT
#define MPIDI_EAGER_SHORT_INTS 4
/* FIXME: This appears to assume that sizeof(int) == 4 (or at least >= 4) */
#define MPIDI_EAGER_SHORT_SIZE 16

/* This is the number of ints that can be carried within an RMA packet */
#define MPIDI_RMA_IMMED_BYTES 8

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
    MPIDI_CH3_PKT_PUT_IMMED,
    MPIDI_CH3_PKT_GET,
    MPIDI_CH3_PKT_ACCUMULATE,
    MPIDI_CH3_PKT_ACCUMULATE_IMMED,
    MPIDI_CH3_PKT_GET_ACCUM,
    MPIDI_CH3_PKT_GET_ACCUM_IMMED,
    MPIDI_CH3_PKT_FOP,
    MPIDI_CH3_PKT_FOP_IMMED,
    MPIDI_CH3_PKT_CAS_IMMED,
    MPIDI_CH3_PKT_GET_RESP,
    MPIDI_CH3_PKT_GET_RESP_IMMED,
    MPIDI_CH3_PKT_GET_ACCUM_RESP,
    MPIDI_CH3_PKT_GET_ACCUM_RESP_IMMED,
    MPIDI_CH3_PKT_FOP_RESP,
    MPIDI_CH3_PKT_FOP_RESP_IMMED,
    MPIDI_CH3_PKT_CAS_RESP_IMMED,
    MPIDI_CH3_PKT_LOCK,
    MPIDI_CH3_PKT_LOCK_ACK,
    MPIDI_CH3_PKT_LOCK_OP_ACK,
    MPIDI_CH3_PKT_UNLOCK,
    MPIDI_CH3_PKT_FLUSH,
    MPIDI_CH3_PKT_ACK,  /* ACK packet for FLUSH, UNLOCK, DECR_AT_COUNTER */
    MPIDI_CH3_PKT_DECR_AT_COUNTER,
    /* RMA Packets end here */
    MPIDI_CH3_PKT_FLOW_CNTL_UPDATE,     /* FIXME: Unused */
    MPIDI_CH3_PKT_CLOSE,
    MPIDI_CH3_PKT_REVOKE,
    MPIDI_CH3_PKT_END_CH3,
    /* The channel can define additional types by defining the value
     * MPIDI_CH3_PKT_ENUM */
#if defined(MPIDI_CH3_PKT_ENUM)
    MPIDI_CH3_PKT_ENUM,
#endif
    MPIDI_CH3_PKT_END_ALL,
    MPIDI_CH3_PKT_INVALID = -1  /* forces a signed enum to quash warnings */
} MPIDI_CH3_Pkt_type_t;

/* These flags can be "OR'ed" together */
typedef enum {
    MPIDI_CH3_PKT_FLAG_NONE = 0,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED = 1,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE = 2,
    MPIDI_CH3_PKT_FLAG_RMA_UNLOCK = 4,
    MPIDI_CH3_PKT_FLAG_RMA_FLUSH = 8,
    MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK = 16,
    MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER = 32,
    MPIDI_CH3_PKT_FLAG_RMA_NOCHECK = 64,
    MPIDI_CH3_PKT_FLAG_RMA_ACK = 128,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED = 256,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_QUEUED = 512,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_DISCARDED = 1024,
    MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED = 2048,
    MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_NO_ACK = 4096,
    MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP = 8192,
    MPIDI_CH3_PKT_FLAG_RMA_STREAM = 16384
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
        /* This macro returns target_datatype in RMA operation          \
           packets. (PUT, GET, ACC, GACC, CAS, FOP) */                  \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
        case (MPIDI_CH3_PKT_PUT_IMMED):                                 \
            datatype_ = (pkt_).put.datatype;                            \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            datatype_ = (pkt_).get.datatype;                            \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
        case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):                          \
            datatype_ = (pkt_).accum.datatype;                          \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
        case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):                           \
            datatype_ = (pkt_).get_accum.datatype;                      \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS_IMMED):                                 \
            datatype_ = (pkt_).cas.datatype;                            \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
        case (MPIDI_CH3_PKT_FOP_IMMED):                                 \
            datatype_ = (pkt_).fop.datatype;                            \
            break;                                                      \
        default:                                                        \
            MPIR_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_TARGET_COUNT(pkt_, count_, err_)          \
    {                                                                   \
        /* This macro returns target_count in RMA operation             \
           packets. (PUT, GET, ACC, GACC, CAS, FOP) */                  \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
        case (MPIDI_CH3_PKT_PUT_IMMED):                                 \
            count_ = (pkt_).put.count;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            count_ = (pkt_).get.count;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
        case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):                          \
            count_ = (pkt_).accum.count;                                \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
        case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):                           \
            count_ = (pkt_).get_accum.count;                            \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS_IMMED):                                 \
        case (MPIDI_CH3_PKT_FOP):                                       \
        case (MPIDI_CH3_PKT_FOP_IMMED):                                 \
            count_ = 1;                                                 \
            break;                                                      \
        default:                                                        \
            MPIR_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_IMMED_DATA_PTR(pkt_, immed_data_, err_)   \
    {                                                                   \
        /* This macro returns pointer to immed data in RMA operation    \
           packets (PUT, ACC, GACC, FOP, CAS) and RMA response          \
           packets (GET_RESP, GACC_RESP, FOP_RESP, CAS_RESP). */        \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT_IMMED):                                 \
            immed_data_ = (pkt_).put.info.data;                         \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):                          \
            immed_data_ = (pkt_).accum.info.data;                       \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):                           \
            immed_data_ = (pkt_).get_accum.info.data;                   \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP_IMMED):                                 \
            immed_data_ = (pkt_).fop.info.data;                         \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS_IMMED):                                 \
            /* Note that here we return pointer of origin data, not     \
               pointer of compare data. */                              \
            immed_data_ = &((pkt_).cas.origin_data);                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_RESP_IMMED):                            \
            immed_data_ = (pkt_).get_resp.info.data;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM_RESP_IMMED):                      \
            immed_data_ = (pkt_).get_accum_resp.info.data;              \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP_RESP_IMMED):                            \
            immed_data_ = (pkt_).fop_resp.info.data;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS_RESP_IMMED):                            \
            immed_data_ = &((pkt_).cas_resp.info.data);                 \
            break;                                                      \
        default:                                                        \
            MPIR_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_FLAGS(pkt_, flags_, err_)                 \
    {                                                                   \
        /* This macro returns flags in RMA operation packets (PUT, GET, \
           ACC, GACC, FOP, CAS), RMA operation response packets         \
           (GET_RESP, GET_ACCUM_RESP, FOP_RESP, CAS_RESP), RMA control  \
           packets (UNLOCK) and RMA control response packets (LOCK_ACK, \
           LOCK_OP_ACK) */                                              \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
        case (MPIDI_CH3_PKT_PUT_IMMED):                                 \
            flags_ = (pkt_).put.flags;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            flags_ = (pkt_).get.flags;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
        case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):                          \
            flags_ = (pkt_).accum.flags;                                \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
        case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):                           \
            flags_ = (pkt_).get_accum.flags;                            \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS_IMMED):                                 \
            flags_ = (pkt_).cas.flags;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
        case (MPIDI_CH3_PKT_FOP_IMMED):                                 \
            flags_ = (pkt_).fop.flags;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_RESP):                                  \
        case (MPIDI_CH3_PKT_GET_RESP_IMMED):                            \
            flags_ = (pkt_).get_resp.flags;                             \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM_RESP):                            \
        case (MPIDI_CH3_PKT_GET_ACCUM_RESP_IMMED):                      \
            flags_ = (pkt_).get_accum_resp.flags;                       \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP_RESP):                                  \
        case (MPIDI_CH3_PKT_FOP_RESP_IMMED):                            \
            flags_ = (pkt_).fop_resp.flags;                             \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS_RESP_IMMED):                            \
            flags_ = (pkt_).cas_resp.flags;                             \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK):                                      \
            flags_ = (pkt_).lock.flags;                                 \
            break;                                                      \
        case (MPIDI_CH3_PKT_UNLOCK):                                    \
            flags_ = (pkt_).unlock.flags;                               \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK_ACK):                                  \
            flags_ = (pkt_).lock_ack.flags;                             \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK_OP_ACK):                               \
            flags_ = (pkt_).lock_op_ack.flags;                          \
            break;                                                      \
        default:                                                        \
            MPIR_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_OP(pkt_, op_, err_)                       \
    {                                                                   \
        /* This macro returns op in RMA operation packets (ACC, GACC,   \
           FOP) */                                                      \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
        case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):                          \
            op_ = (pkt_).accum.op;                                      \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
        case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):                           \
            op_ = (pkt_).get_accum.op;                                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
        case (MPIDI_CH3_PKT_FOP_IMMED):                                 \
            op_ = (pkt_).fop.op;                                        \
            break;                                                      \
        default:                                                        \
            MPIR_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_ERASE_FLAGS(pkt_, err_)                       \
    {                                                                   \
        /* This macro erases flags in RMA operation packets (PUT, GET,  \
           ACC, GACC, FOP, CAS), RMA operation response packets         \
           (GET_RESP, GET_ACCUM_RESP, FOP_RESP, CAS_RESP), RMA control  \
           packets (UNLOCK) and RMA control response packets (LOCK_ACK, \
           LOCK_OP_ACK) */                                              \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
        case (MPIDI_CH3_PKT_PUT_IMMED):                                 \
            (pkt_).put.flags = MPIDI_CH3_PKT_FLAG_NONE;                 \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            (pkt_).get.flags = MPIDI_CH3_PKT_FLAG_NONE;                 \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
        case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):                          \
            (pkt_).accum.flags = MPIDI_CH3_PKT_FLAG_NONE;               \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
        case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):                           \
            (pkt_).get_accum.flags = MPIDI_CH3_PKT_FLAG_NONE;           \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS_IMMED):                                 \
            (pkt_).cas.flags = MPIDI_CH3_PKT_FLAG_NONE;                 \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
        case (MPIDI_CH3_PKT_FOP_IMMED):                                 \
            (pkt_).fop.flags = MPIDI_CH3_PKT_FLAG_NONE;                 \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_RESP):                                  \
        case (MPIDI_CH3_PKT_GET_RESP_IMMED):                            \
            (pkt_).get_resp.flags = MPIDI_CH3_PKT_FLAG_NONE;            \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM_RESP):                            \
        case (MPIDI_CH3_PKT_GET_ACCUM_RESP_IMMED):                      \
            (pkt_).get_accum_resp.flags = MPIDI_CH3_PKT_FLAG_NONE;      \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP_RESP):                                  \
        case (MPIDI_CH3_PKT_FOP_RESP_IMMED):                            \
            (pkt_).fop_resp.flags = MPIDI_CH3_PKT_FLAG_NONE;            \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS_RESP_IMMED):                            \
            (pkt_).cas_resp.flags = MPIDI_CH3_PKT_FLAG_NONE;            \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK):                                      \
            (pkt_).lock.flags = MPIDI_CH3_PKT_FLAG_NONE;                \
            break;                                                      \
        case (MPIDI_CH3_PKT_UNLOCK):                                    \
            (pkt_).unlock.flags = MPIDI_CH3_PKT_FLAG_NONE;              \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK_ACK):                                  \
            (pkt_).lock_ack.flags = MPIDI_CH3_PKT_FLAG_NONE;            \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK_OP_ACK):                               \
            (pkt_).lock_op_ack.flags = MPIDI_CH3_PKT_FLAG_NONE;         \
            break;                                                      \
        default:                                                        \
            MPIR_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_SOURCE_WIN_HANDLE(pkt_, win_hdl_, err_)   \
    {                                                                   \
        /* This macro returns source_win_handle in RMA operation        \
           packets (PUT, GET, ACC, GACC, CAS, FOP), RMA operation       \
           response packets (GET_RESP, GACC_RESP, CAS_RESP, FOP_RESP),  \
           RMA control packets (LOCK, UNLOCK, FLUSH), and RMA control   \
           response packets (LOCK_ACK, LOCK_OP_ACK, ACK). */            \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
        case (MPIDI_CH3_PKT_PUT_IMMED):                                 \
            win_hdl_ = (pkt_).put.source_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
        case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):                          \
            win_hdl_ = (pkt_).accum.source_win_handle;                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK):                                      \
            win_hdl_ = (pkt_).lock.source_win_handle;                   \
            break;                                                      \
        case (MPIDI_CH3_PKT_UNLOCK):                                    \
            win_hdl_ = (pkt_).unlock.source_win_handle;                 \
            break;                                                      \
        case (MPIDI_CH3_PKT_FLUSH):                                     \
            win_hdl_ = (pkt_).flush.source_win_handle;                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK_ACK):                                  \
            win_hdl_ = (pkt_).lock_ack.source_win_handle;               \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK_OP_ACK):                               \
            win_hdl_ = (pkt_).lock_op_ack.source_win_handle;            \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACK):                                       \
            win_hdl_ = (pkt_).ack.source_win_handle;                    \
            break;                                                      \
        default:                                                        \
            MPIR_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_TARGET_WIN_HANDLE(pkt_, win_hdl_, err_)   \
    {                                                                   \
        /* This macro returns target_win_handle in RMA operation        \
           packets (PUT, GET, ACC, GACC, CAS, FOP) and RMA control      \
           packets (LOCK, UNLOCK, FLUSH, DECR_AT_CNT) */                \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
        case (MPIDI_CH3_PKT_PUT_IMMED):                                 \
            win_hdl_ = (pkt_).put.target_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            win_hdl_ = (pkt_).get.target_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
        case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):                          \
            win_hdl_ = (pkt_).accum.target_win_handle;                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
        case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):                           \
            win_hdl_ = (pkt_).get_accum.target_win_handle;              \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS_IMMED):                                 \
            win_hdl_ = (pkt_).cas.target_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
        case (MPIDI_CH3_PKT_FOP_IMMED):                                 \
            win_hdl_ = (pkt_).fop.target_win_handle;                    \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK):                                      \
            win_hdl_ = (pkt_).lock.target_win_handle;                   \
            break;                                                      \
        case (MPIDI_CH3_PKT_UNLOCK):                                    \
            win_hdl_ = (pkt_).unlock.target_win_handle;                 \
            break;                                                      \
        case (MPIDI_CH3_PKT_FLUSH):                                     \
            win_hdl_ = (pkt_).flush.target_win_handle;                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_DECR_AT_COUNTER):                           \
            win_hdl_ = (pkt_).decr_at_cnt.target_win_handle;            \
            break;                                                      \
        default:                                                        \
            MPIR_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_SET_DATALOOP_SIZE(pkt_, dataloop_size_, err_) \
    {                                                                   \
        /* This macro sets dataloop_size in RMA operation packets       \
           (PUT, GET, ACC, GACC) */                                     \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_PUT):                                       \
            (pkt_).put.info.dataloop_size = (dataloop_size_);           \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET):                                       \
            (pkt_).get.info.dataloop_size = (dataloop_size_);           \
            break;                                                      \
        case (MPIDI_CH3_PKT_ACCUMULATE):                                \
            (pkt_).accum.info.dataloop_size = (dataloop_size_);         \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
            (pkt_).get_accum.info.dataloop_size = (dataloop_size_);     \
            break;                                                      \
        default:                                                        \
            MPIR_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }

#define MPIDI_CH3_PKT_RMA_GET_REQUEST_HANDLE(pkt_, request_hdl_, err_)  \
    {                                                                   \
        err_ = MPI_SUCCESS;                                             \
        switch((pkt_).type) {                                           \
        case (MPIDI_CH3_PKT_GET):                                       \
            request_hdl_ = (pkt_).get.request_handle;                   \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM):                                 \
        case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):                           \
            request_hdl_ = (pkt_).get_accum.request_handle;             \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS_IMMED):                                 \
            request_hdl_ = (pkt_).cas.request_handle;                   \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP):                                       \
        case (MPIDI_CH3_PKT_FOP_IMMED):                                 \
            request_hdl_ = (pkt_).fop.request_handle;                   \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_RESP):                                  \
        case (MPIDI_CH3_PKT_GET_RESP_IMMED):                            \
            request_hdl_ = (pkt_).get_resp.request_handle;              \
            break;                                                      \
        case (MPIDI_CH3_PKT_GET_ACCUM_RESP):                            \
        case (MPIDI_CH3_PKT_GET_ACCUM_RESP_IMMED):                      \
            request_hdl_ = (pkt_).get_accum_resp.request_handle;        \
            break;                                                      \
        case (MPIDI_CH3_PKT_FOP_RESP):                                  \
        case (MPIDI_CH3_PKT_FOP_RESP_IMMED):                            \
            request_hdl_ = (pkt_).fop_resp.request_handle;              \
            break;                                                      \
        case (MPIDI_CH3_PKT_CAS_RESP_IMMED):                            \
            request_hdl_ = (pkt_).cas_resp.request_handle;              \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK):                                      \
            request_hdl_ = (pkt_).lock.request_handle;                  \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK_ACK):                                  \
            request_hdl_ = (pkt_).lock_ack.request_handle;              \
            break;                                                      \
        case (MPIDI_CH3_PKT_LOCK_OP_ACK):                               \
            request_hdl_ = (pkt_).lock_op_ack.request_handle;           \
            break;                                                      \
        default:                                                        \
            MPIR_ERR_SETANDJUMP1(err_, MPI_ERR_OTHER, "**invalidpkt", "**invalidpkt %d", (pkt_).type); \
        }                                                               \
    }


/* This macro judges if the RMA operation is a read operation,
 * which means, it will triffer the issuing of response data from
 * the target to the origin */
#define MPIDI_CH3I_RMA_PKT_IS_READ_OP(pkt_)                             \
    ((pkt_).type == MPIDI_CH3_PKT_GET_ACCUM_IMMED ||                    \
     (pkt_).type == MPIDI_CH3_PKT_GET_ACCUM ||                          \
     (pkt_).type == MPIDI_CH3_PKT_FOP_IMMED ||                          \
     (pkt_).type == MPIDI_CH3_PKT_FOP ||                                \
     (pkt_).type == MPIDI_CH3_PKT_CAS_IMMED ||                          \
     (pkt_).type == MPIDI_CH3_PKT_GET)

/* This macro judges if the RMA operation is a immed operation */
#define MPIDI_CH3I_RMA_PKT_IS_IMMED_OP(pkt_)                            \
    ((pkt_).type == MPIDI_CH3_PKT_GET_ACCUM_IMMED ||                    \
     (pkt_).type == MPIDI_CH3_PKT_FOP_IMMED ||                          \
     (pkt_).type == MPIDI_CH3_PKT_CAS_IMMED ||                          \
     (pkt_).type == MPIDI_CH3_PKT_PUT_IMMED ||                          \
     (pkt_).type == MPIDI_CH3_PKT_ACCUMULATE_IMMED)

typedef struct MPIDI_CH3_Pkt_put {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    void *addr;
    int count;
    MPI_Datatype datatype;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    union {
        int dataloop_size;
        char data[MPIDI_RMA_IMMED_BYTES];
    } info;
} MPIDI_CH3_Pkt_put_t;

typedef struct MPIDI_CH3_Pkt_get {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    void *addr;
    int count;
    MPI_Datatype datatype;
    struct {
        int dataloop_size;      /* for derived datatypes */
    } info;
    MPI_Request request_handle;
    MPI_Win target_win_handle;
} MPIDI_CH3_Pkt_get_t;

typedef struct MPIDI_CH3_Pkt_get_resp {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request request_handle;
    /* followings are used to decrement ack_counter at origin */
    int target_rank;
    MPIDI_CH3_Pkt_flags_t flags;
    /* Followings are to piggyback IMMED data */
    struct {
        /* note that we use struct here in order
         * to consistently access data
         * by "pkt->info.data". */
        char data[MPIDI_RMA_IMMED_BYTES];
    } info;
} MPIDI_CH3_Pkt_get_resp_t;

typedef struct MPIDI_CH3_Pkt_accum {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    void *addr;
    int count;
    MPI_Datatype datatype;
    MPI_Op op;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    union {
        int dataloop_size;
        char data[MPIDI_RMA_IMMED_BYTES];
    } info;
} MPIDI_CH3_Pkt_accum_t;

typedef struct MPIDI_CH3_Pkt_get_accum {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Request request_handle; /* For get_accumulate response */
    void *addr;
    int count;
    MPI_Datatype datatype;
    MPI_Op op;
    MPI_Win target_win_handle;
    union {
        int dataloop_size;
        char data[MPIDI_RMA_IMMED_BYTES];
    } info;
} MPIDI_CH3_Pkt_get_accum_t;

typedef struct MPIDI_CH3_Pkt_get_accum_resp {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request request_handle;
    /* followings are used to decrement ack_counter at origin */
    int target_rank;
    MPIDI_CH3_Pkt_flags_t flags;
    /* Followings are to piggyback IMMED data */
    struct {
        /* note that we use struct here in order
         * to consistently access data
         * by "pkt->info.data". */
        char data[MPIDI_RMA_IMMED_BYTES];
    } info;
} MPIDI_CH3_Pkt_get_accum_resp_t;

typedef struct MPIDI_CH3_Pkt_cas {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Datatype datatype;
    void *addr;
    MPI_Request request_handle;
    MPI_Win target_win_handle;
    MPIDI_CH3_CAS_Immed_u origin_data;
    MPIDI_CH3_CAS_Immed_u compare_data;
} MPIDI_CH3_Pkt_cas_t;

typedef struct MPIDI_CH3_Pkt_cas_resp {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request request_handle;
    struct {
        /* note that we use struct here in order
         * to consistently access data
         * by "pkt->info.data". */
        MPIDI_CH3_CAS_Immed_u data;
    } info;
    /* followings are used to decrement ack_counter at orign */
    int target_rank;
    MPIDI_CH3_Pkt_flags_t flags;
} MPIDI_CH3_Pkt_cas_resp_t;

typedef struct MPIDI_CH3_Pkt_fop {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Datatype datatype;
    void *addr;
    MPI_Op op;
    MPI_Request request_handle;
    MPI_Win target_win_handle;
    struct {
        /* note that we use struct here in order
         * to consistently access data
         * by "pkt->info.data". */
        char data[MPIDI_RMA_IMMED_BYTES];
    } info;
} MPIDI_CH3_Pkt_fop_t;

typedef struct MPIDI_CH3_Pkt_fop_resp {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request request_handle;
    struct {
        /* note that we use struct here in order
         * to consistently access data
         * by "pkt->info.data". */
        char data[MPIDI_RMA_IMMED_BYTES];
    } info;
    /* followings are used to decrement ack_counter at orign */
    int target_rank;
    MPIDI_CH3_Pkt_flags_t flags;
} MPIDI_CH3_Pkt_fop_resp_t;

typedef struct MPIDI_CH3_Pkt_lock {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    MPI_Win target_win_handle;
    /* Note that either source_win_handle
     * or request_handle will be used. Here
     * we need both of them because PUT/GET
     * may be converted to LOCK packet,
     * PUT has source_win_handle area and
     * GET has request_handle area. */
    MPI_Win source_win_handle;
    MPI_Request request_handle;
} MPIDI_CH3_Pkt_lock_t;

typedef struct MPIDI_CH3_Pkt_unlock {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    MPIDI_CH3_Pkt_flags_t flags;
} MPIDI_CH3_Pkt_unlock_t;

typedef struct MPIDI_CH3_Pkt_flush {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
} MPIDI_CH3_Pkt_flush_t;

typedef struct MPIDI_CH3_Pkt_lock_ack {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    /* note that either source_win_handle
     * or request_handle is used. */
    MPI_Win source_win_handle;
    MPI_Request request_handle;
    int target_rank;
} MPIDI_CH3_Pkt_lock_ack_t;

typedef struct MPIDI_CH3_Pkt_lock_op_ack {
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_flags_t flags;
    /* note that either source_win_handle
     * or request_handle is used. */
    MPI_Win source_win_handle;
    MPI_Request request_handle;
    int target_rank;
} MPIDI_CH3_Pkt_lock_op_ack_t;

/* This ACK packet is the acknowledgement
 * for FLUSH, UNLOCK and DECR_AT_COUNTER
 * packet */
typedef struct MPIDI_CH3_Pkt_ack {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Win source_win_handle;
    int target_rank;
    MPIDI_CH3_Pkt_flags_t flags;
} MPIDI_CH3_Pkt_ack_t;

typedef struct MPIDI_CH3_Pkt_decr_at_counter {
    MPIDI_CH3_Pkt_type_t type;
    MPI_Win target_win_handle;
    MPI_Win source_win_handle;
    MPIDI_CH3_Pkt_flags_t flags;
} MPIDI_CH3_Pkt_decr_at_counter_t;

typedef struct MPIDI_CH3_Pkt_close {
    MPIDI_CH3_Pkt_type_t type;
    int ack;
} MPIDI_CH3_Pkt_close_t;

typedef struct MPIDI_CH3_Pkt_revoke {
    MPIDI_CH3_Pkt_type_t type;
    MPIU_Context_id_t revoked_comm;
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
    MPIDI_CH3_Pkt_put_t put;
    MPIDI_CH3_Pkt_get_t get;
    MPIDI_CH3_Pkt_get_resp_t get_resp;
    MPIDI_CH3_Pkt_accum_t accum;
    MPIDI_CH3_Pkt_get_accum_t get_accum;
    MPIDI_CH3_Pkt_lock_t lock;
    MPIDI_CH3_Pkt_lock_ack_t lock_ack;
    MPIDI_CH3_Pkt_lock_op_ack_t lock_op_ack;
    MPIDI_CH3_Pkt_unlock_t unlock;
    MPIDI_CH3_Pkt_flush_t flush;
    MPIDI_CH3_Pkt_ack_t ack;
    MPIDI_CH3_Pkt_decr_at_counter_t decr_at_cnt;
    MPIDI_CH3_Pkt_close_t close;
    MPIDI_CH3_Pkt_cas_t cas;
    MPIDI_CH3_Pkt_cas_resp_t cas_resp;
    MPIDI_CH3_Pkt_fop_t fop;
    MPIDI_CH3_Pkt_fop_resp_t fop_resp;
    MPIDI_CH3_Pkt_get_accum_resp_t get_accum_resp;
    MPIDI_CH3_Pkt_revoke_t revoke;
#if defined(MPIDI_CH3_PKT_DECL)
     MPIDI_CH3_PKT_DECL
#endif
} MPIDI_CH3_Pkt_t;

/* Extended header packet types */

/* to send derived datatype across in RMA ops */
typedef struct MPIDI_RMA_dtype_info {   /* for derived datatypes */
    int is_contig;
    MPI_Aint max_contig_blocks;
    MPI_Aint size;
    MPI_Aint extent;
    MPI_Aint dataloop_size;     /* not needed because this info is sent in
                                 * packet header. remove it after lock/unlock
                                 * is implemented in the device */
    void *dataloop;             /* pointer needed to update pointers
                                 * within dataloop on remote side */
    int dataloop_depth;
    int basic_type;
    MPI_Aint ub, lb, true_ub, true_lb;
    int has_sticky_ub, has_sticky_lb;
} MPIDI_RMA_dtype_info;

typedef struct MPIDI_CH3_Ext_pkt_stream {
    MPI_Aint stream_offset;
} MPIDI_CH3_Ext_pkt_stream_t;

typedef struct MPIDI_CH3_Ext_pkt_derived {
    MPIDI_RMA_dtype_info dtype_info;
    /* Follow with variable-length dataloop.
     * On origin we allocate a large buffer including
     * this header and the dataloop; on target we use
     * separate buffer to receive dataloop in order
     * to avoid extra copy.*/
} MPIDI_CH3_Ext_pkt_derived_t;

typedef struct MPIDI_CH3_Ext_pkt_stream_derived {
    MPI_Aint stream_offset;
    MPIDI_RMA_dtype_info dtype_info;
    /* follow with variable-length dataloop. */
} MPIDI_CH3_Ext_pkt_stream_derived_t;

/* Note that since ACC and GET_ACC contain the same extended attributes,
 * we use generic routines for them in some places (see below).
 * If we add OP-specific attribute in future, we should handle them separately.
 *  1. origin issuing function
 *  2. target packet handler
 *  3. target data receive complete handler. */
typedef MPIDI_CH3_Ext_pkt_stream_t MPIDI_CH3_Ext_pkt_accum_stream_t;
typedef MPIDI_CH3_Ext_pkt_derived_t MPIDI_CH3_Ext_pkt_accum_derived_t;
typedef MPIDI_CH3_Ext_pkt_stream_derived_t MPIDI_CH3_Ext_pkt_accum_stream_derived_t;

typedef MPIDI_CH3_Ext_pkt_stream_t MPIDI_CH3_Ext_pkt_get_accum_stream_t;
typedef MPIDI_CH3_Ext_pkt_derived_t MPIDI_CH3_Ext_pkt_get_accum_derived_t;
typedef MPIDI_CH3_Ext_pkt_stream_derived_t MPIDI_CH3_Ext_pkt_get_accum_stream_derived_t;

typedef MPIDI_CH3_Ext_pkt_derived_t MPIDI_CH3_Ext_pkt_put_derived_t;
typedef MPIDI_CH3_Ext_pkt_derived_t MPIDI_CH3_Ext_pkt_get_derived_t;

#if defined(MPID_USE_SEQUENCE_NUMBERS)
typedef struct MPIDI_CH3_Pkt_send_container {
    MPIDI_CH3_Pkt_send_t pkt;
    struct MPIDI_CH3_Pkt_send_container_s *next;
} MPIDI_CH3_Pkt_send_container_t;
#endif

#endif
