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
MPIDI_OFI_capabilities_t MPIDI_OFI_caps_list[MPIDI_OFI_NUM_SETS] =

/* Initialize a runtime version of all of the capability sets defined in
 * ofi_capability_sets.h so we can refer to it if we want to preload a
 * capability set at runtime */
{
    {
        MPIDI_OFI_ENABLE_DATA_PSM,
        MPIDI_OFI_ENABLE_AV_TABLE_PSM,
        MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_PSM,
        MPIDI_OFI_ENABLE_STX_RMA_PSM,
        MPIDI_OFI_ENABLE_MR_SCALABLE_PSM,
        MPIDI_OFI_ENABLE_TAGGED_PSM,
        MPIDI_OFI_ENABLE_AM_PSM,
        MPIDI_OFI_ENABLE_RMA_PSM,
        MPIDI_OFI_MAX_ENDPOINTS_PSM,
        MPIDI_OFI_MAX_ENDPOINTS_BITS_PSM
    },
    {
        MPIDI_OFI_ENABLE_DATA_PSM2,
        MPIDI_OFI_ENABLE_AV_TABLE_PSM2,
        MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_PSM2,
        MPIDI_OFI_ENABLE_STX_RMA_PSM2,
        MPIDI_OFI_ENABLE_MR_SCALABLE_PSM2,
        MPIDI_OFI_ENABLE_TAGGED_PSM2,
        MPIDI_OFI_ENABLE_AM_PSM2,
        MPIDI_OFI_ENABLE_RMA_PSM2,
        MPIDI_OFI_MAX_ENDPOINTS_PSM2,
        MPIDI_OFI_MAX_ENDPOINTS_BITS_PSM2
    },
    {
        MPIDI_OFI_ENABLE_DATA_GNI,
        MPIDI_OFI_ENABLE_AV_TABLE_GNI,
        MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_GNI,
        MPIDI_OFI_ENABLE_STX_RMA_GNI,
        MPIDI_OFI_ENABLE_MR_SCALABLE_GNI,
        MPIDI_OFI_ENABLE_TAGGED_GNI,
        MPIDI_OFI_ENABLE_AM_GNI,
        MPIDI_OFI_ENABLE_RMA_GNI,
        MPIDI_OFI_MAX_ENDPOINTS_GNI,
        MPIDI_OFI_MAX_ENDPOINTS_BITS_GNI
    },
    {
        MPIDI_OFI_ENABLE_DATA_SOCKETS,
        MPIDI_OFI_ENABLE_AV_TABLE_SOCKETS,
        MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_SOCKETS,
        MPIDI_OFI_ENABLE_STX_RMA_SOCKETS,
        MPIDI_OFI_ENABLE_MR_SCALABLE_SOCKETS,
        MPIDI_OFI_ENABLE_TAGGED_SOCKETS,
        MPIDI_OFI_ENABLE_AM_SOCKETS,
        MPIDI_OFI_ENABLE_RMA_SOCKETS,
        MPIDI_OFI_MAX_ENDPOINTS_SOCKETS,
        MPIDI_OFI_MAX_ENDPOINTS_BITS_SOCKETS
    },
    {
        MPIDI_OFI_ENABLE_DATA_BGQ,
        MPIDI_OFI_ENABLE_AV_TABLE_BGQ,
        MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS_BGQ,
        MPIDI_OFI_ENABLE_STX_RMA_BGQ,
        MPIDI_OFI_ENABLE_MR_SCALABLE_BGQ,
        MPIDI_OFI_ENABLE_TAGGED_BGQ,
        MPIDI_OFI_ENABLE_AM_BGQ,
        MPIDI_OFI_ENABLE_RMA_BGQ,
        MPIDI_OFI_MAX_ENDPOINTS_BGQ,
        MPIDI_OFI_MAX_ENDPOINTS_BITS_BGQ
    }
};