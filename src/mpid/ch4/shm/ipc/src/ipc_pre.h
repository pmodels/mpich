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
    MPIDI_IPCI_TYPE__GPU,
    /* use DIRECT when sender caches the mapped address */
    MPIDI_IPCI_TYPE__DIRECT
} MPIDI_IPCI_type_t;

typedef struct {
    MPIDI_IPCI_type_t ipc_type;
    void *base_addr;            /* if set, remove the handle at request completion */
} MPIDI_IPC_am_request_t;

#endif /* IPC_PRE_H_INCLUDED */
