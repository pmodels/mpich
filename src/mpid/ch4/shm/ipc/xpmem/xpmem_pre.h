/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_PRE_H_INCLUDED
#define XPMEM_PRE_H_INCLUDED

/* memory handle definition */
typedef struct {
    uintptr_t src_offset;
    int src_lrank;
    uintptr_t data_sz;
} MPIDI_XPMEM_ipc_handle_t;

/* request extension definition */
typedef struct {
    int dummy;
} MPIDI_XPMEM_am_request_t;

#endif /* XPMEM_PRE_H_INCLUDED */
