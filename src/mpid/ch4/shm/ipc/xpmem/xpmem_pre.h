/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef XPMEM_PRE_H_INCLUDED
#define XPMEM_PRE_H_INCLUDED

/* memory handle definition */
typedef struct {
    const void *src_offset;
    int src_lrank;
    MPI_Aint data_sz;
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
