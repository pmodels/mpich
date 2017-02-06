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
#include <mpidimpl.h>
#include "ofi_impl.h"
MPIDI_OFI_global_t MPIDI_Global = { 0 };

MPIDI_OFI_huge_recv_t *MPIDI_unexp_huge_recv_head = NULL;
MPIDI_OFI_huge_recv_t *MPIDI_unexp_huge_recv_tail = NULL;
MPIDI_OFI_huge_recv_list_t *MPIDI_posted_huge_recv_head = NULL;
MPIDI_OFI_huge_recv_list_t *MPIDI_posted_huge_recv_tail = NULL;

MPIDI_OFI_capabilities_t MPIDI_OFI_caps_list[MPIDI_OFI_NUM_SETS] =

/* Initialize a runtime version of all of the capability sets defined in
 * ofi_capability_sets.h so we can refer to it if we want to preload a
 * capability set at runtime */
{
    { /* minimal required capability */
        .enable_data               = MPIDI_OFI_ENABLE_DATA_MINIMAL,
        .enable_av_table           = MPIDI_OFI_ENABLE_AV_TABLE_MINIMAL,
        .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_MINIMAL,
        .enable_stx_rma            = MPIDI_OFI_ENABLE_STX_RMA_MINIMAL,
        .enable_mr_scalable        = MPIDI_OFI_ENABLE_MR_SCALABLE_MINIMAL,
        .enable_tagged             = MPIDI_OFI_ENABLE_TAGGED_MINIMAL,
        .enable_am                 = MPIDI_OFI_ENABLE_AM_MINIMAL,
        .enable_rma                = MPIDI_OFI_ENABLE_RMA_MINIMAL,
        .enable_atomics            = MPIDI_OFI_ENABLE_ATOMICS_MINIMAL,
        .max_endpoints             = MPIDI_OFI_MAX_ENDPOINTS_MINIMAL,
        .max_endpoints_bits        = MPIDI_OFI_MAX_ENDPOINTS_BITS_MINIMAL,
        .context_bits              = MPIDI_OFI_CONTEXT_BITS_MINIMAL,
        .source_bits               = MPIDI_OFI_SOURCE_BITS_MINIMAL,
        .tag_bits                  = MPIDI_OFI_TAG_BITS_MINIMAL
    },
    { /* psm */
        .enable_data               = MPIDI_OFI_ENABLE_DATA_PSM,
        .enable_av_table           = MPIDI_OFI_ENABLE_AV_TABLE_PSM,
        .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_PSM,
        .enable_stx_rma            = MPIDI_OFI_ENABLE_STX_RMA_PSM,
        .enable_mr_scalable        = MPIDI_OFI_ENABLE_MR_SCALABLE_PSM,
        .enable_tagged             = MPIDI_OFI_ENABLE_TAGGED_PSM,
        .enable_am                 = MPIDI_OFI_ENABLE_AM_PSM,
        .enable_rma                = MPIDI_OFI_ENABLE_RMA_PSM,
        .enable_atomics            = MPIDI_OFI_ENABLE_ATOMICS_PSM,
        .max_endpoints             = MPIDI_OFI_MAX_ENDPOINTS_PSM,
        .max_endpoints_bits        = MPIDI_OFI_MAX_ENDPOINTS_BITS_PSM,
        .context_bits              = MPIDI_OFI_CONTEXT_BITS_PSM,
        .source_bits               = MPIDI_OFI_SOURCE_BITS_PSM,
        .tag_bits                  = MPIDI_OFI_TAG_BITS_PSM
    },
    { /* psm2 */
        .enable_data               = MPIDI_OFI_ENABLE_DATA_PSM2,
        .enable_av_table           = MPIDI_OFI_ENABLE_AV_TABLE_PSM2,
        .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_PSM2,
        .enable_stx_rma            = MPIDI_OFI_ENABLE_STX_RMA_PSM2,
        .enable_mr_scalable        = MPIDI_OFI_ENABLE_MR_SCALABLE_PSM2,
        .enable_tagged             = MPIDI_OFI_ENABLE_TAGGED_PSM2,
        .enable_am                 = MPIDI_OFI_ENABLE_AM_PSM2,
        .enable_rma                = MPIDI_OFI_ENABLE_RMA_PSM2,
        .enable_atomics            = MPIDI_OFI_ENABLE_ATOMICS_PSM2,
        .max_endpoints             = MPIDI_OFI_MAX_ENDPOINTS_PSM2,
        .max_endpoints_bits        = MPIDI_OFI_MAX_ENDPOINTS_BITS_PSM2,
        .context_bits              = MPIDI_OFI_CONTEXT_BITS_PSM2,
        .source_bits               = MPIDI_OFI_SOURCE_BITS_PSM2,
        .tag_bits                  = MPIDI_OFI_TAG_BITS_PSM2
    },
    { /* gni */
        .enable_data               = MPIDI_OFI_ENABLE_DATA_GNI,
        .enable_av_table           = MPIDI_OFI_ENABLE_AV_TABLE_GNI,
        .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_GNI,
        .enable_stx_rma            = MPIDI_OFI_ENABLE_STX_RMA_GNI,
        .enable_mr_scalable        = MPIDI_OFI_ENABLE_MR_SCALABLE_GNI,
        .enable_tagged             = MPIDI_OFI_ENABLE_TAGGED_GNI,
        .enable_am                 = MPIDI_OFI_ENABLE_AM_GNI,
        .enable_rma                = MPIDI_OFI_ENABLE_RMA_GNI,
        .enable_atomics            = MPIDI_OFI_ENABLE_ATOMICS_GNI,
        .max_endpoints             = MPIDI_OFI_MAX_ENDPOINTS_GNI,
        .max_endpoints_bits        = MPIDI_OFI_MAX_ENDPOINTS_BITS_GNI,
        .context_bits              = MPIDI_OFI_CONTEXT_BITS_GNI,
        .source_bits               = MPIDI_OFI_SOURCE_BITS_GNI,
        .tag_bits                  = MPIDI_OFI_TAG_BITS_GNI
    },
    { /* sockets */
        .enable_data               = MPIDI_OFI_ENABLE_DATA_SOCKETS,
        .enable_av_table           = MPIDI_OFI_ENABLE_AV_TABLE_SOCKETS,
        .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_SOCKETS,
        .enable_stx_rma            = MPIDI_OFI_ENABLE_STX_RMA_SOCKETS,
        .enable_mr_scalable        = MPIDI_OFI_ENABLE_MR_SCALABLE_SOCKETS,
        .enable_tagged             = MPIDI_OFI_ENABLE_TAGGED_SOCKETS,
        .enable_am                 = MPIDI_OFI_ENABLE_AM_SOCKETS,
        .enable_rma                = MPIDI_OFI_ENABLE_RMA_SOCKETS,
        .enable_atomics            = MPIDI_OFI_ENABLE_ATOMICS_SOCKETS,
        .max_endpoints             = MPIDI_OFI_MAX_ENDPOINTS_SOCKETS,
        .max_endpoints_bits        = MPIDI_OFI_MAX_ENDPOINTS_BITS_SOCKETS,
        .context_bits              = MPIDI_OFI_CONTEXT_BITS_SOCKETS,
        .source_bits               = MPIDI_OFI_SOURCE_BITS_SOCKETS,
        .tag_bits                  = MPIDI_OFI_TAG_BITS_SOCKETS
    },
    { /* bgq */
        .enable_data               = MPIDI_OFI_ENABLE_DATA_BGQ,
        .enable_av_table           = MPIDI_OFI_ENABLE_AV_TABLE_BGQ,
        .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_BGQ,
        .enable_stx_rma            = MPIDI_OFI_ENABLE_STX_RMA_BGQ,
        .enable_mr_scalable        = MPIDI_OFI_ENABLE_MR_SCALABLE_BGQ,
        .enable_tagged             = MPIDI_OFI_ENABLE_TAGGED_BGQ,
        .enable_am                 = MPIDI_OFI_ENABLE_AM_BGQ,
        .enable_rma                = MPIDI_OFI_ENABLE_RMA_BGQ,
        .enable_atomics            = MPIDI_OFI_ENABLE_ATOMICS_BGQ,
        .max_endpoints             = MPIDI_OFI_MAX_ENDPOINTS_BGQ,
        .max_endpoints_bits        = MPIDI_OFI_MAX_ENDPOINTS_BITS_BGQ,
        .context_bits              = MPIDI_OFI_CONTEXT_BITS_BGQ,
        .source_bits               = MPIDI_OFI_SOURCE_BITS_BGQ,
        .tag_bits                  = MPIDI_OFI_TAG_BITS_BGQ
    },
    { /* verbs */
        .enable_data               = MPIDI_OFI_ENABLE_DATA_VERBS,
        .enable_av_table           = MPIDI_OFI_ENABLE_AV_TABLE_VERBS,
        .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_VERBS,
        .enable_stx_rma            = MPIDI_OFI_ENABLE_STX_RMA_VERBS,
        .enable_mr_scalable        = MPIDI_OFI_ENABLE_MR_SCALABLE_VERBS,
        .enable_tagged             = MPIDI_OFI_ENABLE_TAGGED_VERBS,
        .enable_am                 = MPIDI_OFI_ENABLE_AM_VERBS,
        .enable_rma                = MPIDI_OFI_ENABLE_RMA_VERBS,
        .enable_atomics            = MPIDI_OFI_ENABLE_ATOMICS_VERBS,
        .max_endpoints             = MPIDI_OFI_MAX_ENDPOINTS_VERBS,
        .max_endpoints_bits        = MPIDI_OFI_MAX_ENDPOINTS_BITS_VERBS,
        .context_bits              = MPIDI_OFI_CONTEXT_BITS_VERBS,
        .source_bits               = MPIDI_OFI_SOURCE_BITS_VERBS,
        .tag_bits                  = MPIDI_OFI_TAG_BITS_VERBS
    }
};
