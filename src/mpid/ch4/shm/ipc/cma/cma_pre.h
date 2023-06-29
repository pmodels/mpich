/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CMA_PRE_H_INCLUDED
#define CMA_PRE_H_INCLUDED

#define MPIDI_CMA_NUM_STATIC_IOVS 10

/* memory handle definition */
typedef struct {
    pid_t pid;
    /* addr & size for contig data, ignored otherwise */
    const void *vaddr;
    MPI_Aint data_sz;
} MPIDI_CMA_ipc_handle_t;

/* local struct used for caching between query and preparing memory handle */
typedef struct {
    const void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
} MPIDI_CMA_ipc_attr_t;

#endif /* CMA_PRE_H_INCLUDED */
