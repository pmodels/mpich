/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_PRE_H_INCLUDED
#define IPC_PRE_H_INCLUDED

#include "../xpmem/xpmem_pre.h"
#include "../cma/cma_pre.h"
#include "../gpu/gpu_pre.h"

typedef enum MPIDI_IPCI_type {
    MPIDI_IPCI_TYPE__NONE,      /* avoid empty enum */
    MPIDI_IPCI_TYPE__SKIP,
    MPIDI_IPCI_TYPE__XPMEM,
    MPIDI_IPCI_TYPE__CMA,
    MPIDI_IPCI_TYPE__GPU
} MPIDI_IPCI_type_t;

typedef struct {
    MPIDI_IPCI_type_t ipc_type;
#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    MPIDI_GPU_ipc_attr_t gpu_attr;
#endif
} MPIDI_IPC_am_request_t;

#endif /* IPC_PRE_H_INCLUDED */
