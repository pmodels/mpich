/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XPMEM_PRE_H_INCLUDED
#define XPMEM_PRE_H_INCLUDED

/* memory handle definition */
typedef struct {
    int src_lrank;
    int is_contig;
    const void *addr;
    MPI_Aint true_lb, range;
} MPIDI_XPMEM_ipc_handle_t;

/* local struct used for query and preparing memory handle.
 * The fields are mainly to serve as cache to avoid duplicated code
 * between MPIDI_IPCI_try_lmt_isend and MPIDI_IPCI_send_lmt.
 * TODO: clean it up.
 */
typedef struct {
    const void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
} MPIDI_XPMEM_ipc_attr_t;

#endif /* XPMEM_PRE_H_INCLUDED */
