/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_PRE_H_INCLUDED
#define IPC_PRE_H_INCLUDED

#include "../xpmem/xpmem_pre.h"
#include "../gpu/gpu_pre.h"

typedef enum MPIDI_IPCI_type {
    MPIDI_IPCI_TYPE__NONE,      /* avoid empty enum */
    MPIDI_IPCI_TYPE__XPMEM,
    MPIDI_IPCI_TYPE__GPU
} MPIDI_IPCI_type_t;

#endif /* IPC_PRE_H_INCLUDED */
