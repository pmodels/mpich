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
#ifndef NETMOD_OFI_CAPABILITY_SETS_H_INCLUDED
#define NETMOD_OFI_CAPABILITY_SETS_H_INCLUDED

#define MPIDI_OFI_OFF     0
#define MPIDI_OFI_ON      1

/*
 * The definitions map to these capability sets:
 *
 * MPIDI_OFI_ENABLE_DATA               fi_tsenddata (and other functions with immediate data)
 *                                     Uses FI_REMOTE_CQ_DATA, FI_DIRECTED_RECV
 * MPIDI_OFI_ENABLE_AV_TABLE           Use FI_AV_TABLE instead of FI_AV_MAP
 * MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS fi_scalable_ep instead of fi_ep
 *                                     domain_attr.max_ep_tx_ctx > 1
 * MPIDI_OFI_ENABLE_STX_RMA            Use shared transmit contexts for RMA
 *                                     Uses FI_SHARED_CONTEXT
 * MPIDI_OFI_ENABLE_MR_SCALABLE        Use FI_MR_SCALABLE instead of FI_MR_BASIC
 *                                     If using runtime mode, this will be set to FI_MR_UNSPEC
 * MPIDI_OFI_ENABLE_TAGGED             Use FI_TAGGED interface instead of FI_MSG
 * MPIDI_OFI_ENABLE_AM                 Use FI_MSG and FI_MULTI_RECV for active messages
 * MPIDI_OFI_ENABLE_RMA                Use FI_ATOMICS and FI_RMA interfaces
 */

#ifdef MPIDI_CH4_OFI_USE_SET_PSM
#define MPIDI_OFI_ENABLE_DATA               MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_AV_TABLE           MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_STX_RMA            MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_MR_SCALABLE        MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_TAGGED             MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_AM                 MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_RMA                MPIDI_OFI_ON
#endif

#ifdef MPIDI_CH4_OFI_USE_SET_PSM2
#define MPIDI_OFI_ENABLE_DATA               MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_AV_TABLE           MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_STX_RMA            MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_MR_SCALABLE        MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_TAGGED             MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_AM                 MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_RMA                MPIDI_OFI_ON
#endif

#ifdef MPIDI_CH4_OFI_USE_SET_GNI
#define MPIDI_OFI_ENABLE_DATA               MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_AV_TABLE           MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_STX_RMA            MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_MR_SCALABLE        MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_TAGGED             MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_AM                 MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_RMA                MPIDI_OFI_ON
#endif

#ifdef MPIDI_CH4_OFI_USE_SET_SOCKETS
#define MPIDI_OFI_ENABLE_DATA               MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_AV_TABLE           MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_STX_RMA            MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_MR_SCALABLE        MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_TAGGED             MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_AM                 MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_RMA                MPIDI_OFI_ON
#endif

#ifdef MPIDI_CH4_OFI_USE_SET_BGQ
#define MPIDI_OFI_ENABLE_RUNTIME_CHECKS     0
#define MPIDI_OFI_ENABLE_DATA               MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_AV_TABLE           MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_STX_RMA            MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_MR_SCALABLE        MPIDI_OFI_OFF
#define MPIDI_OFI_ENABLE_TAGGED             MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_AM                 MPIDI_OFI_ON
#define MPIDI_OFI_ENABLE_RMA                MPIDI_OFI_ON
#endif

#endif
