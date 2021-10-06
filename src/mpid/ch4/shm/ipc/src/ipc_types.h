/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_TYPES_H_INCLUDED
#define IPC_TYPES_H_INCLUDED

#include "mpiimpl.h"

typedef struct {
    MPIR_Group *node_group_ptr; /* cache node group, used at win_create. */
} MPIDI_IPCI_global_t;

/* memory handle definition
 * MPIDI_IPCI_ipc_handle_t: local memory handle
 * MPIDI_IPCI_ipc_attr_t: local memory attributes including available handle,
 *                        IPC type, and thresholds */
typedef union MPIDI_IPCI_ipc_handle {
    MPIDI_XPMEM_ipc_handle_t xpmem;
    MPIDI_GPU_ipc_handle_t gpu;
} MPIDI_IPCI_ipc_handle_t;

typedef struct MPIDI_IPCI_ipc_attr {
    MPIDI_IPCI_type_t ipc_type;
    MPIDI_IPCI_ipc_handle_t ipc_handle;
    MPL_pointer_attr_t gpu_attr;
    struct {
        size_t send_lmt_sz;
    } threshold;
} MPIDI_IPCI_ipc_attr_t;

/* ctrl packet header types */
typedef struct MPIDI_IPC_rndv_hdr {
    MPIDI_IPCI_type_t ipc_type;
    MPIDI_IPCI_ipc_handle_t ipc_handle;
    uint64_t is_contig:8;
    uint64_t flattened_sz:24;   /* only if it's non-contig, flattened type
                                 * will be attached after this header. */
    MPI_Aint count;             /* only if it's non-contig */
} MPIDI_IPC_hdr;

typedef struct MPIDI_IPC_rts {
    MPIDIG_hdr_t hdr;
    MPIDI_IPC_hdr ipc_hdr;
} MPIDI_IPC_rts_t;

typedef struct MPIDI_IPC_ack {
    MPIDI_IPCI_type_t ipc_type;
    MPIR_Request *req_ptr;
} MPIDI_IPC_ack_t;

#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_IPCI_DBG_GENERAL;
#endif
#define IPC_TRACE(...) \
    MPL_DBG_MSG_FMT(MPIDI_IPCI_DBG_GENERAL,VERBOSE,(MPL_DBG_FDEST, "IPC "__VA_ARGS__))

#define MPIDI_IPCI_REQUEST(req, field)      ((req)->dev.ch4.am.shm_am.ipc.field)

extern MPIDI_IPCI_global_t MPIDI_IPCI_global;

#endif /* IPC_TYPES_H_INCLUDED */
