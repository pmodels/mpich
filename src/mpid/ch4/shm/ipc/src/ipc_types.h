/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_TYPES_H_INCLUDED
#define IPC_TYPES_H_INCLUDED

#include "mpiimpl.h"

typedef enum MPIDI_IPCI_type {
    MPIDI_IPCI_TYPE__NONE,      /* avoid empty enum */
    MPIDI_IPCI_TYPE__XPMEM,
    MPIDI_IPCI_TYPE__GPU
} MPIDI_IPCI_type_t;

typedef struct {
    MPIR_Group *node_group_ptr; /* cache node group, used at win_create. */
} MPIDI_IPCI_global_t;

/* memory handle definition
 * MPIDI_IPCI_mem_handle_t: local memory handle
 * MPIDI_IPCI_mem_attr_t: local memory attributes including available handle,
 *                        IPC type, and thresholds
 * MPIDI_IPCI_mem_seg_t: mapped segment with remote memory handle */
typedef union MPIDI_IPCI_mem_handle {
    MPIDI_XPMEM_mem_handle_t xpmem;
    MPIDI_GPU_mem_handle_t gpu;
} MPIDI_IPCI_mem_handle_t;

typedef struct MPIDI_IPCI_mem_attr {
    MPIDI_IPCI_type_t ipc_type;
    MPIDI_IPCI_mem_handle_t mem_handle;
    struct {
        size_t send_lmt_sz;
    } threshold;
} MPIDI_IPCI_mem_attr_t;

typedef struct MPIDI_IPCI_mem_seg {
    MPIDI_IPCI_type_t ipc_type;
    union {
        MPIDI_XPMEM_mem_seg_t xpmem;
        MPIDI_GPU_mem_seg_t gpu;
    } u;
} MPIDI_IPCI_mem_seg_t;

#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_IPCI_DBG_GENERAL;
#endif
#define IPC_TRACE(...) \
    MPL_DBG_MSG_FMT(MPIDI_IPCI_DBG_GENERAL,VERBOSE,(MPL_DBG_FDEST, "IPC "__VA_ARGS__))

#define MPIDI_IPCI_REQUEST(req, field)      ((req)->dev.ch4.am.shm_am.ipc.field)

extern MPIDI_IPCI_global_t MPIDI_IPCI_global;

#endif /* IPC_TYPES_H_INCLUDED */
