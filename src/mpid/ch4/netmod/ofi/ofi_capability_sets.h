/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_CAPABILITY_SETS_H_INCLUDED
#define OFI_CAPABILITY_SETS_H_INCLUDED

#define MPIDI_OFI_OFF     0
#define MPIDI_OFI_ON      1

enum {
    MPIDI_OFI_SET_NUMBER_DEFAULT = 0,
    MPIDI_OFI_SET_NUMBER_MINIMAL,
    MPIDI_OFI_SET_NUMBER_PSM2,
    MPIDI_OFI_SET_NUMBER_SOCKETS,
    MPIDI_OFI_SET_NUMBER_BGQ,
    MPIDI_OFI_SET_NUMBER_RXM,
    MPIDI_OFI_NUM_SETS
};

#define MPIDI_OFI_SET_NAME_DEFAULT "default"
#define MPIDI_OFI_SET_NAME_MINIMAL "minimal"

#define MPIDI_OFI_MAX_ENDPOINTS_SCALABLE        256
#define MPIDI_OFI_MAX_ENDPOINTS_BITS_SCALABLE   8
#define MPIDI_OFI_MAX_ENDPOINTS_REGULAR         1
#define MPIDI_OFI_MAX_ENDPOINTS_BITS_REGULAR    0

/* This needs to be kept in sync with the order in globals.c */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_get_set_number(const char *set_name)
{
    if (set_name == NULL) {
        return MPIDI_OFI_SET_NUMBER_DEFAULT;
    } else if (!strcmp("psm2", set_name)) {
        return MPIDI_OFI_SET_NUMBER_PSM2;
    } else if (!strcmp("sockets", set_name)) {
        return MPIDI_OFI_SET_NUMBER_SOCKETS;
    } else if (!strcmp("bgq", set_name)) {
        return MPIDI_OFI_SET_NUMBER_BGQ;
    } else if (strstr(set_name, "ofi_rxm")) {
        return MPIDI_OFI_SET_NUMBER_RXM;
    } else if (!strcmp(MPIDI_OFI_SET_NAME_MINIMAL, set_name)) {
        return MPIDI_OFI_SET_NUMBER_MINIMAL;
    } else {
        return MPIDI_OFI_SET_NUMBER_DEFAULT;
    }
}

/*
 * The definitions map to these capability sets:
 *
 * MPIDI_OFI_ENABLE_AV_TABLE           Use FI_AV_TABLE instead of FI_AV_MAP
 * MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS fi_scalable_ep instead of fi_ep
 *                                     domain_attr.max_ep_tx_ctx > 1
 * MPIDI_OFI_MAX_ENDPOINTS             Maximum number of endpoints (regular or scalable) to use
 * MPIDI_OFI_MAX_ENDPOINTS_BITS        Number of bits used by MPIDI_OFI_MAX_ENDPOINTS_BITS
 * MPIDI_OFI_ENABLE_SHARED_CONTEXTS    Uses FI_SHARED_CONTEXT
 * MPIDI_OFI_ENABLE_MR_SCALABLE        Use FI_MR_SCALABLE instead of FI_MR_BASIC
 *                                     If using runtime mode, this will be set to FI_MR_UNSPEC
 * MPIDI_OFI_ENABLE_TAGGED             Use FI_TAGGED interface instead of FI_MSG
 * MPIDI_OFI_ENABLE_AM                 Use FI_MSG and FI_MULTI_RECV for active messages
 * MPIDI_OFI_ENABLE_RMA                Require FI_DELIVERY_COMPLETE and use the OFI RMA code path
 *                                     for all RMA operations. With this turned off, we only use
 *                                     FI_READ in the active message code path (so we still require
 *                                     FI_RMA either way).
 * MPIDI_OFI_ENABLE_ATOMICS            Use FI_ATOMICS interfaces
 * MPIDI_OFI_FETCH_ATOMIC_IOVECS       The maximum number of iovecs that can be used for fetch_atomic operations
 * MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS Use a progress thread for normal data messages
 * MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS Use a progress thread for control messages
 * MPIDI_OFI_ENABLE_PT2PT_NOPACK       Allow sending messages without packing
 * MPIDI_OFI_NUM_AM_BUFFERS            Number of buffers available for active messages
 * MPIDI_OFI_CONTEXT_BITS              The number of bits used for the context ID in an OFI message
 * MPIDI_OFI_SOURCE_BITS               The number of bits used for the source rank in an OFI message
 * MPIDI_OFI_TAG_BITS                  The number of bits used for the tag in an OFI message
 * MPIDI_OFI_MAJOR_VERSION             The major API version of libfabric required
 * MPIDI_OFI_MINOR_VERSION             The minor API version of libfabric required
 *
 * === Compile time only ===
 * MPIDI_OFI_IOVEC_ALIGN               Required alignment for iovecs
 * The first four values an optimization to avoid calculating this value every time they are needed.
 * They can be calculated from the bits above.
 * MPIDI_OFI_PROTOCOL_MASK             The bitmask used to extract the protocol from the match_bits in an OFI message
 * MPIDI_OFI_CONTEXT_MASK              The bitmask used to extract the context ID from the match_bits in an OFI message
 * MPIDI_OFI_SOURCE_MASK               The bitmask used to extract the source rank from the match_bits in an OFI message
 * MPIDI_OFI_TAG_MASK                  The bitmask used to extract the tag from the match_bits in an OFI message
 * Protocol bits
 * MPIDI_OFI_SYNC_SEND                 The bit to indicate a sync send
 * MPIDI_OFI_SYNC_SEND_ACK             The bit to indicate a sync send ack
 * MPIDI_OFI_DYNPROC_SEND              The bit to indicate a dynamic process send
 * MPIDI_OFI_CONTEXT_STRUCTS           The number of fi_context structs needed for the provider
 */

#define MPIDI_OFI_ENABLE_AV_TABLE_PSM2           MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_PSM2 MPIDI_OFI_ON
#define MPIDI_OFI_MAX_ENDPOINTS_PSM2             MPIDI_OFI_MAX_ENDPOINTS_SCALABLE
#define MPIDI_OFI_MAX_ENDPOINTS_BITS_PSM2        MPIDI_OFI_MAX_ENDPOINTS_BITS_SCALABLE
#define MPIDI_OFI_ENABLE_SHARED_CONTEXTS_PSM2    MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_MR_SCALABLE_PSM2        MPIDI_OFI_ON
#ifdef MPIDI_OFI_FORCE_AM
#define MPIDI_OFI_ENABLE_TAGGED_PSM2             MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_RMA_PSM2                MPIDI_OFI_OFF
#else
#define MPIDI_OFI_ENABLE_TAGGED_PSM2             MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_RMA_PSM2                MPIDI_OFI_ON
#endif
#define MPIDI_OFI_ENABLE_AM_PSM2                 MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_ATOMICS_PSM2            MPIDI_OFI_ENABLE_RMA_PSM2
#define MPIDI_OFI_FETCH_ATOMIC_IOVECS_PSM2       1
#define MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_PSM2 MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_PSM2  MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_PT2PT_NOPACK_PSM2       MPIDI_OFI_ON
#define MPIDI_OFI_NUM_AM_BUFFERS_PSM2            MPIDI_OFI_MAX_NUM_AM_BUFFERS
#define MPIDI_OFI_CONTEXT_BITS_PSM2              (16)
#define MPIDI_OFI_SOURCE_BITS_PSM2               (0)
#define MPIDI_OFI_TAG_BITS_PSM2                  (31)
#define MPIDI_OFI_MAJOR_VERSION_PSM2             1
#define MPIDI_OFI_MINOR_VERSION_PSM2             6

#ifdef MPIDI_CH4_OFI_USE_SET_PSM2
#define MPIDI_OFI_SET_NUMBER                MPIDI_OFI_SET_NUMBER_PSM2
#define MPIDI_OFI_ENABLE_RUNTIME_CHECKS     MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_AV_TABLE           MPIDI_OFI_ENABLE_AV_TABLE_PSM2
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_PSM2
#define MPIDI_OFI_MAX_ENDPOINTS             MPIDI_OFI_MAX_ENDPOINTS_PSM2
#define MPIDI_OFI_MAX_ENDPOINTS_BITS        MPIDI_OFI_MAX_ENDPOINTS_BITS_PSM2
#define MPIDI_OFI_ENABLE_SHARED_CONTEXTS    MPIDI_OFI_ENABLE_SHARED_CONTEXTS_PSM2
#define MPIDI_OFI_ENABLE_MR_SCALABLE        MPIDI_OFI_ENABLE_MR_SCALABLE_PSM2
#define MPIDI_OFI_ENABLE_TAGGED             MPIDI_OFI_ENABLE_TAGGED_PSM2
#define MPIDI_OFI_ENABLE_AM                 MPIDI_OFI_ENABLE_AM_PSM2
#define MPIDI_OFI_ENABLE_RMA                MPIDI_OFI_ENABLE_RMA_PSM2
#define MPIDI_OFI_ENABLE_ATOMICS            MPIDI_OFI_ENABLE_ATOMICS_PSM2
#define MPIDI_OFI_FETCH_ATOMIC_IOVECS       MPIDI_OFI_FETCH_ATOMIC_IOVECS_PSM2
#define MPIDI_OFI_IOVEC_ALIGN               (1)
#define MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_PSM2
#define MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS  MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_PSM2
#define MPIDI_OFI_ENABLE_PT2PT_NOPACK       MPIDI_OFI_ENABLE_PT2PT_NOPACK_PSM2
#define MPIDI_OFI_NUM_AM_BUFFERS            MPIDI_OFI_NUM_AM_BUFFERS_PSM2
#define MPIDI_OFI_PROTOCOL_MASK             (0x0006000000000000ULL)
#define MPIDI_OFI_CONTEXT_MASK              (0x0000FFFF00000000ULL)
#define MPIDI_OFI_SOURCE_MASK               (0x0000000000000000ULL)     /* PSM2 does support immediate data
                                                                         * so this field is zeroed */
#define MPIDI_OFI_TAG_MASK                  (0x000000007FFFFFFFULL)
#define MPIDI_OFI_CONTEXT_BITS              MPIDI_OFI_CONTEXT_BITS_PSM2
#define MPIDI_OFI_SOURCE_BITS               MPIDI_OFI_SOURCE_BITS_PSM2
#define MPIDI_OFI_TAG_BITS                  MPIDI_OFI_TAG_BITS_PSM2
#define MPIDI_OFI_SYNC_SEND_ACK             (0x0001000000000000ULL)
#define MPIDI_OFI_SYNC_SEND                 (0x0002000000000000ULL)
#define MPIDI_OFI_DYNPROC_SEND              (0x0004000000000000ULL)
#define MPIDI_OFI_MAJOR_VERSION             MPIDI_OFI_MAJOR_VERSION_PSM2
#define MPIDI_OFI_MINOR_VERSION             MPIDI_OFI_MINOR_VERSION_PSM2
#define MPIDI_OFI_CONTEXT_STRUCTS           1
#endif

#define MPIDI_OFI_ENABLE_AV_TABLE_SOCKETS           MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_SOCKETS MPIDI_OFI_ON
#define MPIDI_OFI_MAX_ENDPOINTS_SOCKETS             MPIDI_OFI_MAX_ENDPOINTS_SCALABLE
#define MPIDI_OFI_MAX_ENDPOINTS_BITS_SOCKETS        MPIDI_OFI_MAX_ENDPOINTS_BITS_SCALABLE
#define MPIDI_OFI_ENABLE_SHARED_CONTEXTS_SOCKETS    MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_MR_SCALABLE_SOCKETS        MPIDI_OFI_ON
#ifdef MPIDI_OFI_FORCE_AM
#define MPIDI_OFI_ENABLE_TAGGED_SOCKETS             MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_RMA_SOCKETS                MPIDI_OFI_OFF
#else
#define MPIDI_OFI_ENABLE_TAGGED_SOCKETS             MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_RMA_SOCKETS                MPIDI_OFI_ON
#endif
#define MPIDI_OFI_ENABLE_AM_SOCKETS                 MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_ATOMICS_SOCKETS            MPIDI_OFI_ENABLE_RMA_SOCKETS
#define MPIDI_OFI_FETCH_ATOMIC_IOVECS_SOCKETS       1
#define MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_SOCKETS MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_SOCKETS  MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_PT2PT_NOPACK_SOCKETS       MPIDI_OFI_ON
#define MPIDI_OFI_NUM_AM_BUFFERS_SOCKETS            MPIDI_OFI_MAX_NUM_AM_BUFFERS
#define MPIDI_OFI_CONTEXT_BITS_SOCKETS              (16)
#define MPIDI_OFI_SOURCE_BITS_SOCKETS               (0)
#define MPIDI_OFI_TAG_BITS_SOCKETS                  (31)
#define MPIDI_OFI_MAJOR_VERSION_SOCKETS             1
#define MPIDI_OFI_MINOR_VERSION_SOCKETS             5

#ifdef MPIDI_CH4_OFI_USE_SET_SOCKETS
#define MPIDI_OFI_SET_NUMBER                MPIDI_OFI_SET_NUMBER_SOCKETS
#define MPIDI_OFI_ENABLE_RUNTIME_CHECKS     MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_AV_TABLE           MPIDI_OFI_ENABLE_AV_TABLE_SOCKETS
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_SOCKETS
#define MPIDI_OFI_MAX_ENDPOINTS             MPIDI_OFI_MAX_ENDPOINTS_SOCKETS
#define MPIDI_OFI_MAX_ENDPOINTS_BITS        MPIDI_OFI_MAX_ENDPOINTS_BITS_SOCKETS
#define MPIDI_OFI_ENABLE_SHARED_CONTEXTS    MPIDI_OFI_ENABLE_SHARED_CONTEXTS_SOCKETS
#define MPIDI_OFI_ENABLE_MR_SCALABLE        MPIDI_OFI_ENABLE_MR_SCALABLE_SOCKETS
#define MPIDI_OFI_ENABLE_TAGGED             MPIDI_OFI_ENABLE_TAGGED_SOCKETS
#define MPIDI_OFI_ENABLE_AM                 MPIDI_OFI_ENABLE_AM_SOCKETS
#define MPIDI_OFI_ENABLE_RMA                MPIDI_OFI_ENABLE_RMA_SOCKETS
#define MPIDI_OFI_ENABLE_ATOMICS            MPIDI_OFI_ENABLE_ATOMICS_SOCKETS
#define MPIDI_OFI_FETCH_ATOMIC_IOVECS       MPIDI_OFI_FETCH_ATOMIC_IOVECS_SOCKETS
#define MPIDI_OFI_IOVEC_ALIGN               (1)
#define MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_SOCKETS
#define MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS  MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_SOCKETS
#define MPIDI_OFI_ENABLE_PT2PT_NOPACK       MPIDI_OFI_ENABLE_PT2PT_NOPACK_SOCKETS
#define MPIDI_OFI_NUM_AM_BUFFERS            MPIDI_OFI_NUM_AM_BUFFERS_SOCKETS
#define MPIDI_OFI_PROTOCOL_MASK             (0x0006000000000000ULL)
#define MPIDI_OFI_CONTEXT_MASK              (0x0000FFFF00000000ULL)
#define MPIDI_OFI_SOURCE_MASK               (0x0000000000000000ULL)     /* Sockets does support immediate data
                                                                         * so this field is zeroed */
#define MPIDI_OFI_TAG_MASK                  (0x000000007FFFFFFFULL)
#define MPIDI_OFI_CONTEXT_BITS              MPIDI_OFI_CONTEXT_BITS_SOCKETS
#define MPIDI_OFI_SOURCE_BITS               MPIDI_OFI_SOURCE_BITS_SOCKETS
#define MPIDI_OFI_TAG_BITS                  MPIDI_OFI_TAG_BITS_SOCKETS
#define MPIDI_OFI_SYNC_SEND_ACK             (0x0001000000000000ULL)
#define MPIDI_OFI_SYNC_SEND                 (0x0002000000000000ULL)
#define MPIDI_OFI_DYNPROC_SEND              (0x0004000000000000ULL)
#define MPIDI_OFI_MAJOR_VERSION             MPIDI_OFI_MAJOR_VERSION_SOCKETS
#define MPIDI_OFI_MINOR_VERSION             MPIDI_OFI_MINOR_VERSION_SOCKETS
#define MPIDI_OFI_CONTEXT_STRUCTS           1
#endif

#define MPIDI_OFI_ENABLE_AV_TABLE_BGQ           MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_BGQ MPIDI_OFI_OFF
#define MPIDI_OFI_MAX_ENDPOINTS_BGQ             MPIDI_OFI_MAX_ENDPOINTS_REGULAR
#define MPIDI_OFI_MAX_ENDPOINTS_BITS_BGQ        MPIDI_OFI_MAX_ENDPOINTS_BITS_REGULAR
#define MPIDI_OFI_ENABLE_SHARED_CONTEXTS_BGQ    MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_MR_SCALABLE_BGQ        MPIDI_OFI_OFF
#ifdef MPIDI_OFI_FORCE_AM
#define MPIDI_OFI_ENABLE_TAGGED_BGQ             MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_RMA_BGQ                MPIDI_OFI_OFF
#else
#define MPIDI_OFI_ENABLE_TAGGED_BGQ             MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_RMA_BGQ                MPIDI_OFI_ON
#endif
#define MPIDI_OFI_ENABLE_AM_BGQ                 MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_ATOMICS_BGQ            MPIDI_OFI_ENABLE_RMA_BGQ
#define MPIDI_OFI_FETCH_ATOMIC_IOVECS_BGQ       1
#define MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_BGQ MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_BGQ  MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_PT2PT_NOPACK_BGQ       MPIDI_OFI_OFF
#define MPIDI_OFI_NUM_AM_BUFFERS_BGQ            MPIDI_OFI_MAX_NUM_AM_BUFFERS
#define MPIDI_OFI_CONTEXT_BITS_BGQ              (16)
#define MPIDI_OFI_SOURCE_BITS_BGQ               (0)
#define MPIDI_OFI_TAG_BITS_BGQ                  (31)
#define MPIDI_OFI_MAJOR_VERSION_BGQ             1
#define MPIDI_OFI_MINOR_VERSION_BGQ             5

#ifdef MPIDI_CH4_OFI_USE_SET_BGQ
#define MPIDI_OFI_SET_NUMBER                MPIDI_OFI_SET_NUMBER_BGQ
#define MPIDI_OFI_ENABLE_RUNTIME_CHECKS     MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_AV_TABLE           MPIDI_OFI_ENABLE_AV_TABLE_BGQ
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_BGQ
#define MPIDI_OFI_MAX_ENDPOINTS             MPIDI_OFI_MAX_ENDPOINTS_BGQ
#define MPIDI_OFI_MAX_ENDPOINTS_BITS        MPIDI_OFI_MAX_ENDPOINTS_BITS_BGQ
#define MPIDI_OFI_ENABLE_SHARED_CONTEXTS    MPIDI_OFI_ENABLE_SHARED_CONTEXTS_BGQ
#define MPIDI_OFI_ENABLE_MR_SCALABLE        MPIDI_OFI_ENABLE_MR_SCALABLE_BGQ
#define MPIDI_OFI_ENABLE_TAGGED             MPIDI_OFI_ENABLE_TAGGED_BGQ
#define MPIDI_OFI_ENABLE_AM                 MPIDI_OFI_ENABLE_AM_BGQ
#define MPIDI_OFI_ENABLE_RMA                MPIDI_OFI_ENABLE_RMA_BGQ
#define MPIDI_OFI_ENABLE_ATOMICS            MPIDI_OFI_ENABLE_ATOMICS_BGQ
#define MPIDI_OFI_FETCH_ATOMIC_IOVECS       MPIDI_OFI_FETCH_ATOMIC_IOVECS_BGQ
#define MPIDI_OFI_IOVEC_ALIGN               (1)
#define MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_BGQ
#define MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS  MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_BGQ
#define MPIDI_OFI_ENABLE_PT2PT_NOPACK       MPIDI_OFI_ENABLE_PT2PT_NOPACK_BGQ
#define MPIDI_OFI_NUM_AM_BUFFERS            MPIDI_OFI_NUM_AM_BUFFERS_BGQ
#define MPIDI_OFI_PROTOCOL_MASK             (0x0006000000000000ULL)
#define MPIDI_OFI_CONTEXT_MASK              (0x0000FFFF00000000ULL)
#define MPIDI_OFI_SOURCE_MASK               (0x0000000000000000ULL)     /* BGQ does support immediate data
                                                                         * so this field is zeroed */
#define MPIDI_OFI_TAG_MASK                  (0x000000007FFFFFFFULL)
#define MPIDI_OFI_CONTEXT_BITS              MPIDI_OFI_CONTEXT_BITS_BGQ
#define MPIDI_OFI_SOURCE_BITS               MPIDI_OFI_SOURCE_BITS_BGQ
#define MPIDI_OFI_TAG_BITS                  MPIDI_OFI_TAG_BITS_BGQ
#define MPIDI_OFI_SYNC_SEND_ACK             (0x0001000000000000ULL)
#define MPIDI_OFI_SYNC_SEND                 (0x0002000000000000ULL)
#define MPIDI_OFI_DYNPROC_SEND              (0x0004000000000000ULL)
#define MPIDI_OFI_MAJOR_VERSION             MPIDI_OFI_MAJOR_VERSION_BGQ
#define MPIDI_OFI_MINOR_VERSION             MPIDI_OFI_MINOR_VERSION_BGQ
#define MPIDI_OFI_CONTEXT_STRUCTS           2
#endif

#define MPIDI_OFI_ENABLE_AV_TABLE_RXM              MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_RXM    MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_SHARED_CONTEXTS_RXM       MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_MR_SCALABLE_RXM           MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_TAGGED_RXM                MPIDI_OFI_ON
#ifdef MPIDI_OFI_FORCE_AM
#define MPIDI_OFI_ENABLE_AM_RXM                    MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_RMA_RXM                   MPIDI_OFI_OFF
#else
#define MPIDI_OFI_ENABLE_AM_RXM                    MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_RMA_RXM                   MPIDI_OFI_ON
#endif
#define MPIDI_OFI_ENABLE_ATOMICS_RXM               MPIDI_OFI_OFF
#define MPIDI_OFI_MAX_ENDPOINTS_RXM                MPIDI_OFI_MAX_ENDPOINTS_REGULAR
#define MPIDI_OFI_MAX_ENDPOINTS_BITS_RXM           MPIDI_OFI_MAX_ENDPOINTS_BITS_REGULAR
#define MPIDI_OFI_FETCH_ATOMIC_IOVECS_RXM           1
#define MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_RXM    MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_RXM MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_PT2PT_NOPACK_RXM          MPIDI_OFI_ON
#define MPIDI_OFI_NUM_AM_BUFFERS_RXM               MPIDI_OFI_MAX_NUM_AM_BUFFERS
#define MPIDI_OFI_CONTEXT_BITS_RXM                 (16)
#define MPIDI_OFI_SOURCE_BITS_RXM                  (0)
#define MPIDI_OFI_TAG_BITS_RXM                     (31)
#define MPIDI_OFI_MAJOR_VERSION_RXM                 1
#define MPIDI_OFI_MINOR_VERSION_RXM                 6

#ifdef MPIDI_CH4_OFI_USE_SET_RXM
#define MPIDI_OFI_SET_NUMBER                    MPIDI_OFI_SET_NUMBER_RXM
#define MPIDI_OFI_ENABLE_RUNTIME_CHECKS         MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_AV_TABLE               MPIDI_OFI_ENABLE_AV_TABLE_RXM
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS     MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_RXM
#define MPIDI_OFI_MAX_ENDPOINTS                 MPIDI_OFI_MAX_ENDPOINTS_RXM
#define MPIDI_OFI_MAX_ENDPOINTS_BITS            MPIDI_OFI_MAX_ENDPOINTS_BITS_RXM
#define MPIDI_OFI_ENABLE_SHARED_CONTEXTS        MPIDI_OFI_ENABLE_SHARED_CONTEXTS_RXM
#define MPIDI_OFI_ENABLE_MR_SCALABLE            MPIDI_OFI_ENABLE_MR_SCALABLE_RXM
#define MPIDI_OFI_ENABLE_TAGGED                 MPIDI_OFI_ENABLE_TAGGED_RXM
#define MPIDI_OFI_ENABLE_AM                     MPIDI_OFI_ENABLE_AM_RXM
#define MPIDI_OFI_ENABLE_RMA                    MPIDI_OFI_ENABLE_RMA_RXM
#define MPIDI_OFI_ENABLE_ATOMICS                MPIDI_OFI_ENABLE_ATOMICS_RXM
#define MPIDI_OFI_FETCH_ATOMIC_IOVECS           MPIDI_OFI_FETCH_ATOMIC_IOVECS_RXM
#define MPIDI_OFI_IOVEC_ALIGN                   (1)
#define MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS     MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_RXM
#define MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS  MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_RXM
#define MPIDI_OFI_ENABLE_PT2PT_NOPACK           MPIDI_OFI_ENABLE_PT2PT_NOPACK_RXM
#define MPIDI_OFI_NUM_AM_BUFFERS                MPIDI_OFI_NUM_AM_BUFFERS_RXM
#define MPIDI_OFI_PROTOCOL_MASK                 (0x0006000000000000ULL)
#define MPIDI_OFI_CONTEXT_MASK                  (0x0000FFFF00000000ULL)
#define MPIDI_OFI_SOURCE_MASK                   (0x0000000000000000ULL) /* RxM does support immediate data
                                                                         * so this field is zeroed */
#define MPIDI_OFI_TAG_MASK                      (0x000000007FFFFFFFULL)
#define MPIDI_OFI_CONTEXT_BITS                  MPIDI_OFI_CONTEXT_BITS_RXM
#define MPIDI_OFI_SOURCE_BITS                   MPIDI_OFI_SOURCE_BITS_RXM
#define MPIDI_OFI_TAG_BITS                      MPIDI_OFI_TAG_BITS_RXM
#define MPIDI_OFI_SYNC_SEND_ACK                 (0x0001000000000000ULL)
#define MPIDI_OFI_SYNC_SEND                     (0x0002000000000000ULL)
#define MPIDI_OFI_DYNPROC_SEND                  (0x0004000000000000ULL)
#define MPIDI_OFI_MAJOR_VERSION                 MPIDI_OFI_MAJOR_VERSION_RXM
#define MPIDI_OFI_MINOR_VERSION                 MPIDI_OFI_MINOR_VERSION_RXM
#define MPIDI_OFI_CONTEXT_STRUCTS                1
#endif

/* capability set to request the default supported capabilities (reasonable performance with a
 * reasonable set of requirements) */
#define MPIDI_OFI_ENABLE_AV_TABLE_DEFAULT           MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_DEFAULT MPIDI_OFI_OFF
#define MPIDI_OFI_MAX_ENDPOINTS_DEFAULT             MPIDI_OFI_MAX_ENDPOINTS_REGULAR
#define MPIDI_OFI_MAX_ENDPOINTS_BITS_DEFAULT        MPIDI_OFI_MAX_ENDPOINTS_BITS_REGULAR
#define MPIDI_OFI_ENABLE_SHARED_CONTEXTS_DEFAULT    MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_MR_SCALABLE_DEFAULT        MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_TAGGED_DEFAULT             MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_RMA_DEFAULT                MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_AM_DEFAULT                 MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_ATOMICS_DEFAULT            MPIDI_OFI_ENABLE_RMA_DEFAULT
#define MPIDI_OFI_FETCH_ATOMIC_IOVECS_DEFAULT       1
#define MPIDI_OFI_IOVEC_ALIGN_DEFAULT               (1)
#define MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_DEFAULT MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_DEFAULT  MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_PT2PT_NOPACK_DEFAULT       MPIDI_OFI_ON
#define MPIDI_OFI_NUM_AM_BUFFERS_DEFAULT            MPIDI_OFI_MAX_NUM_AM_BUFFERS
#define MPIDI_OFI_PROTOCOL_MASK_DEFAULT             (0x0006000000000000ULL)
#define MPIDI_OFI_CONTEXT_MASK_DEFAULT              (0x0000FFFF00000000ULL)
#define MPIDI_OFI_SOURCE_MASK_DEFAULT               (0x0000000000000000ULL)     /* We require support for immediate data
                                                                                 * so this field is zeroed */
#define MPIDI_OFI_TAG_MASK_DEFAULT                  (0x000000007FFFFFFFULL)
#define MPIDI_OFI_CONTEXT_BITS_DEFAULT              (16)
#define MPIDI_OFI_SOURCE_BITS_DEFAULT               (0)
#define MPIDI_OFI_TAG_BITS_DEFAULT                  (31)
#define MPIDI_OFI_SYNC_SEND_ACK_DEFAULT             (0x0001000000000000ULL)
#define MPIDI_OFI_SYNC_SEND_DEFAULT                 (0x0002000000000000ULL)
#define MPIDI_OFI_DYNPROC_SEND_DEFAULT              (0x0004000000000000ULL)
#define MPIDI_OFI_MAJOR_VERSION_DEFAULT             FI_MAJOR_VERSION
#define MPIDI_OFI_MINOR_VERSION_DEFAULT             FI_MINOR_VERSION

/* capability set to request the minimal supported capabilities */
#define MPIDI_OFI_ENABLE_AV_TABLE_MINIMAL           MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_MINIMAL MPIDI_OFI_OFF
#define MPIDI_OFI_MAX_ENDPOINTS_MINIMAL             MPIDI_OFI_MAX_ENDPOINTS_REGULAR
#define MPIDI_OFI_MAX_ENDPOINTS_BITS_MINIMAL        MPIDI_OFI_MAX_ENDPOINTS_BITS_REGULAR
#define MPIDI_OFI_ENABLE_SHARED_CONTEXTS_MINIMAL    MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_MR_SCALABLE_MINIMAL        MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_TAGGED_MINIMAL             MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_RMA_MINIMAL                MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_AM_MINIMAL                 MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_ATOMICS_MINIMAL            MPIDI_OFI_ENABLE_RMA_MINIMAL
#define MPIDI_OFI_FETCH_ATOMIC_IOVECS_MINIMAL       1
#define MPIDI_OFI_IOVEC_ALIGN_MINIMAL               (1)
#define MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_MINIMAL MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_MINIMAL  MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_PT2PT_NOPACK_MINIMAL       MPIDI_OFI_ON
#define MPIDI_OFI_NUM_AM_BUFFERS_MINIMAL            MPIDI_OFI_MAX_NUM_AM_BUFFERS
#define MPIDI_OFI_PROTOCOL_MASK_MINIMAL             (0xE000000000000000ULL)     /* This will be a problem for providers that require all 64 match bits. */
#define MPIDI_OFI_CONTEXT_MASK_MINIMAL              (0x0FFFF00000000000ULL)
#define MPIDI_OFI_SOURCE_MASK_MINIMAL               (0x00000FFFFFF00000ULL)     /* assume that provider does not support immediate data
                                                                                 * so this field needs to be available */
#define MPIDI_OFI_TAG_MASK_MINIMAL                  (0x00000000000FFFFFULL)
#define MPIDI_OFI_CONTEXT_BITS_MINIMAL              (16)
#define MPIDI_OFI_SOURCE_BITS_MINIMAL               (24)
#define MPIDI_OFI_TAG_BITS_MINIMAL                  (20)
#define MPIDI_OFI_SYNC_SEND_MINIMAL                 (0x1000000000000000ULL)
#define MPIDI_OFI_SYNC_SEND_ACK_MINIMAL             (0x2000000000000000ULL)
#define MPIDI_OFI_DYNPROC_SEND_MINIMAL              (0x4000000000000000ULL)
#define MPIDI_OFI_MAJOR_VERSION_MINIMAL             FI_MAJOR_VERSION
#define MPIDI_OFI_MINOR_VERSION_MINIMAL             FI_MINOR_VERSION

#ifdef MPIDI_CH4_OFI_USE_SET_RUNTIME
#define MPIDI_OFI_SET_NUMBER                MPIDI_OFI_SET_NUMBER_DEFAULT
#define MPIDI_OFI_ENABLE_RUNTIME_CHECKS     1
#define MPIDI_OFI_ENABLE_AV_TABLE           MPIDI_Global.settings.enable_av_table
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS MPIDI_Global.settings.enable_scalable_endpoints
#define MPIDI_OFI_MAX_ENDPOINTS             MPIDI_Global.settings.max_endpoints
#define MPIDI_OFI_MAX_ENDPOINTS_BITS        MPIDI_Global.settings.max_endpoints_bits
#define MPIDI_OFI_ENABLE_SHARED_CONTEXTS    MPIDI_Global.settings.enable_shared_contexts
#define MPIDI_OFI_ENABLE_MR_SCALABLE        MPIDI_Global.settings.enable_mr_scalable
#define MPIDI_OFI_ENABLE_TAGGED             MPIDI_Global.settings.enable_tagged
#define MPIDI_OFI_ENABLE_AM                 MPIDI_Global.settings.enable_am
#define MPIDI_OFI_ENABLE_RMA                MPIDI_Global.settings.enable_rma
#define MPIDI_OFI_ENABLE_ATOMICS            MPIDI_Global.settings.enable_atomics
#define MPIDI_OFI_FETCH_ATOMIC_IOVECS       MPIDI_Global.settings.fetch_atomic_iovecs
#define MPIDI_OFI_IOVEC_ALIGN               (1) /* Compile time configuration only */
#define MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS MPIDI_Global.settings.enable_data_auto_progress
#define MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS  MPIDI_Global.settings.enable_control_auto_progress
#define MPIDI_OFI_ENABLE_PT2PT_NOPACK       MPIDI_Global.settings.enable_pt2pt_nopack
#define MPIDI_OFI_NUM_AM_BUFFERS            MPIDI_Global.settings.num_am_buffers
#define MPIDI_OFI_CONTEXT_BITS              MPIDI_Global.settings.context_bits
#define MPIDI_OFI_SOURCE_BITS               MPIDI_Global.settings.source_bits
#define MPIDI_OFI_TAG_BITS                  MPIDI_Global.settings.tag_bits
#define MPIDI_OFI_MAJOR_VERSION             MPIDI_Global.settings.major_version
#define MPIDI_OFI_MINOR_VERSION             MPIDI_Global.settings.minor_version
#define MPIDI_OFI_CONTEXT_STRUCTS           2   /* Compile time configurable only */
#endif

#endif /* OFI_CAPABILITY_SETS_H_INCLUDED */
