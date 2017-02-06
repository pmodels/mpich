/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef MPIDI_CH4_COLL_PROGRESS_H_INCLUDED
#define MPIDI_CH4_COLL_PROGRESS_H_INCLUDED
#define MPIDI_COLL_NUM_ENTRIES (8)
MPIDI_COLL_progress_global_t MPIDI_COLL_progress_global;
static inline int MPIDI_COLL_progress_hook()
{
    void *coll_entries[MPIDI_COLL_NUM_ENTRIES];
    int i, coll_count, mpi_errno;
    mpi_errno = MPI_SUCCESS;
    coll_count = MPIDI_COLL_Progress(MPIDI_COLL_NUM_ENTRIES, coll_entries);
    for (i = 0; i < coll_count; i++) {
        MPIDI_COLL_req_t *base = (MPIDI_COLL_req_t *) coll_entries[i];
        MPIR_Request *req = container_of(base, MPIR_Request, dev.ch4_coll);
        MPIDI_CH4U_request_complete(req);
    }
    return mpi_errno;
}

#endif
