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
    {   /* default required capability */
     .enable_av_table = MPIDI_OFI_ENABLE_AV_TABLE_DEFAULT,
     .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_DEFAULT,
     .enable_shared_contexts = MPIDI_OFI_ENABLE_SHARED_CONTEXTS_DEFAULT,
     .enable_mr_scalable = MPIDI_OFI_ENABLE_MR_SCALABLE_DEFAULT,
     .enable_tagged = MPIDI_OFI_ENABLE_TAGGED_DEFAULT,
     .enable_am = MPIDI_OFI_ENABLE_AM_DEFAULT,
     .enable_rma = MPIDI_OFI_ENABLE_RMA_DEFAULT,
     .enable_atomics = MPIDI_OFI_ENABLE_ATOMICS_DEFAULT,
     .enable_data_auto_progress = MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_DEFAULT,
     .enable_control_auto_progress = MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_DEFAULT,
     .enable_pt2pt_nopack = MPIDI_OFI_ENABLE_PT2PT_NOPACK_DEFAULT,
     .num_am_buffers = MPIDI_OFI_NUM_AM_BUFFERS_DEFAULT,
     .max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_DEFAULT,
     .max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_DEFAULT,
     .fetch_atomic_iovecs = MPIDI_OFI_FETCH_ATOMIC_IOVECS_DEFAULT,
     .context_bits = MPIDI_OFI_CONTEXT_BITS_DEFAULT,
     .source_bits = MPIDI_OFI_SOURCE_BITS_DEFAULT,
     .tag_bits = MPIDI_OFI_TAG_BITS_DEFAULT,
     .major_version = MPIDI_OFI_MAJOR_VERSION_DEFAULT,
     .minor_version = MPIDI_OFI_MINOR_VERSION_DEFAULT}
    ,
    {   /* minimal required capability */
     .enable_av_table = MPIDI_OFI_ENABLE_AV_TABLE_MINIMAL,
     .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_MINIMAL,
     .enable_shared_contexts = MPIDI_OFI_ENABLE_SHARED_CONTEXTS_MINIMAL,
     .enable_mr_scalable = MPIDI_OFI_ENABLE_MR_SCALABLE_MINIMAL,
     .enable_tagged = MPIDI_OFI_ENABLE_TAGGED_MINIMAL,
     .enable_am = MPIDI_OFI_ENABLE_AM_MINIMAL,
     .enable_rma = MPIDI_OFI_ENABLE_RMA_MINIMAL,
     .enable_atomics = MPIDI_OFI_ENABLE_ATOMICS_MINIMAL,
     .enable_data_auto_progress = MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_MINIMAL,
     .enable_control_auto_progress = MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_MINIMAL,
     .enable_pt2pt_nopack = MPIDI_OFI_ENABLE_PT2PT_NOPACK_MINIMAL,
     .num_am_buffers = MPIDI_OFI_NUM_AM_BUFFERS_MINIMAL,
     .max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_MINIMAL,
     .max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_MINIMAL,
     .fetch_atomic_iovecs = MPIDI_OFI_FETCH_ATOMIC_IOVECS_MINIMAL,
     .context_bits = MPIDI_OFI_CONTEXT_BITS_MINIMAL,
     .source_bits = MPIDI_OFI_SOURCE_BITS_MINIMAL,
     .tag_bits = MPIDI_OFI_TAG_BITS_MINIMAL,
     .major_version = MPIDI_OFI_MAJOR_VERSION_MINIMAL,
     .minor_version = MPIDI_OFI_MINOR_VERSION_MINIMAL}
    ,
    {   /* psm2 */
     .enable_av_table = MPIDI_OFI_ENABLE_AV_TABLE_PSM2,
     .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_PSM2,
     .enable_shared_contexts = MPIDI_OFI_ENABLE_SHARED_CONTEXTS_PSM2,
     .enable_mr_scalable = MPIDI_OFI_ENABLE_MR_SCALABLE_PSM2,
     .enable_tagged = MPIDI_OFI_ENABLE_TAGGED_PSM2,
     .enable_am = MPIDI_OFI_ENABLE_AM_PSM2,
     .enable_rma = MPIDI_OFI_ENABLE_RMA_PSM2,
     .enable_atomics = MPIDI_OFI_ENABLE_ATOMICS_PSM2,
     .enable_data_auto_progress = MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_PSM2,
     .enable_control_auto_progress = MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_PSM2,
     .enable_pt2pt_nopack = MPIDI_OFI_ENABLE_PT2PT_NOPACK_PSM2,
     .num_am_buffers = MPIDI_OFI_NUM_AM_BUFFERS_PSM2,
     .max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_PSM2,
     .max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_PSM2,
     .fetch_atomic_iovecs = MPIDI_OFI_FETCH_ATOMIC_IOVECS_PSM2,
     .context_bits = MPIDI_OFI_CONTEXT_BITS_PSM2,
     .source_bits = MPIDI_OFI_SOURCE_BITS_PSM2,
     .tag_bits = MPIDI_OFI_TAG_BITS_PSM2,
     .major_version = MPIDI_OFI_MAJOR_VERSION_MINIMAL,
     .minor_version = MPIDI_OFI_MINOR_VERSION_MINIMAL}
    ,
    {   /* sockets */
     .enable_av_table = MPIDI_OFI_ENABLE_AV_TABLE_SOCKETS,
     .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_SOCKETS,
     .enable_shared_contexts = MPIDI_OFI_ENABLE_SHARED_CONTEXTS_SOCKETS,
     .enable_mr_scalable = MPIDI_OFI_ENABLE_MR_SCALABLE_SOCKETS,
     .enable_tagged = MPIDI_OFI_ENABLE_TAGGED_SOCKETS,
     .enable_am = MPIDI_OFI_ENABLE_AM_SOCKETS,
     .enable_rma = MPIDI_OFI_ENABLE_RMA_SOCKETS,
     .enable_atomics = MPIDI_OFI_ENABLE_ATOMICS_SOCKETS,
     .enable_data_auto_progress = MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_SOCKETS,
     .enable_control_auto_progress = MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_SOCKETS,
     .enable_pt2pt_nopack = MPIDI_OFI_ENABLE_PT2PT_NOPACK_SOCKETS,
     .num_am_buffers = MPIDI_OFI_NUM_AM_BUFFERS_SOCKETS,
     .max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_SOCKETS,
     .max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_SOCKETS,
     .fetch_atomic_iovecs = MPIDI_OFI_FETCH_ATOMIC_IOVECS_SOCKETS,
     .context_bits = MPIDI_OFI_CONTEXT_BITS_SOCKETS,
     .source_bits = MPIDI_OFI_SOURCE_BITS_SOCKETS,
     .tag_bits = MPIDI_OFI_TAG_BITS_SOCKETS,
     .major_version = MPIDI_OFI_MAJOR_VERSION_MINIMAL,
     .minor_version = MPIDI_OFI_MINOR_VERSION_MINIMAL}
    ,
    {   /* bgq */
     .enable_av_table = MPIDI_OFI_ENABLE_AV_TABLE_BGQ,
     .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_BGQ,
     .enable_shared_contexts = MPIDI_OFI_ENABLE_SHARED_CONTEXTS_BGQ,
     .enable_mr_scalable = MPIDI_OFI_ENABLE_MR_SCALABLE_BGQ,
     .enable_tagged = MPIDI_OFI_ENABLE_TAGGED_BGQ,
     .enable_am = MPIDI_OFI_ENABLE_AM_BGQ,
     .enable_rma = MPIDI_OFI_ENABLE_RMA_BGQ,
     .enable_atomics = MPIDI_OFI_ENABLE_ATOMICS_BGQ,
     .enable_data_auto_progress = MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_BGQ,
     .enable_control_auto_progress = MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_BGQ,
     .enable_pt2pt_nopack = MPIDI_OFI_ENABLE_PT2PT_NOPACK_BGQ,
     .num_am_buffers = MPIDI_OFI_NUM_AM_BUFFERS_BGQ,
     .max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_BGQ,
     .max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_BGQ,
     .fetch_atomic_iovecs = MPIDI_OFI_FETCH_ATOMIC_IOVECS_BGQ,
     .context_bits = MPIDI_OFI_CONTEXT_BITS_BGQ,
     .source_bits = MPIDI_OFI_SOURCE_BITS_BGQ,
     .tag_bits = MPIDI_OFI_TAG_BITS_BGQ,
     .major_version = MPIDI_OFI_MAJOR_VERSION_MINIMAL,
     .minor_version = MPIDI_OFI_MINOR_VERSION_MINIMAL}
    ,
    {   /* RxM */
     .enable_av_table = MPIDI_OFI_ENABLE_AV_TABLE_RXM,
     .enable_scalable_endpoints = MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_RXM,
     .enable_shared_contexts = MPIDI_OFI_ENABLE_SHARED_CONTEXTS_RXM,
     .enable_mr_scalable = MPIDI_OFI_ENABLE_MR_SCALABLE_RXM,
     .enable_tagged = MPIDI_OFI_ENABLE_TAGGED_RXM,
     .enable_am = MPIDI_OFI_ENABLE_AM_RXM,
     .enable_rma = MPIDI_OFI_ENABLE_RMA_RXM,
     .enable_atomics = MPIDI_OFI_ENABLE_ATOMICS_RXM,
     .enable_data_auto_progress = MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS_RXM,
     .enable_control_auto_progress = MPIDI_OFI_ENABLE_CONTROL_AUTO_PROGRESS_RXM,
     .enable_pt2pt_nopack = MPIDI_OFI_ENABLE_PT2PT_NOPACK_RXM,
     .num_am_buffers = MPIDI_OFI_NUM_AM_BUFFERS_RXM,
     .max_endpoints = MPIDI_OFI_MAX_ENDPOINTS_RXM,
     .max_endpoints_bits = MPIDI_OFI_MAX_ENDPOINTS_BITS_RXM,
     .fetch_atomic_iovecs = MPIDI_OFI_FETCH_ATOMIC_IOVECS_RXM,
     .context_bits = MPIDI_OFI_CONTEXT_BITS_RXM,
     .source_bits = MPIDI_OFI_SOURCE_BITS_RXM,
     .tag_bits = MPIDI_OFI_TAG_BITS_RXM,
     .major_version = MPIDI_OFI_MAJOR_VERSION_MINIMAL,
     .minor_version = MPIDI_OFI_MINOR_VERSION_MINIMAL}
};
