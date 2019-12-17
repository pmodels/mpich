/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDI_AM_RNDV_PRE_H_INCLUDED
#define MPIDI_AM_RNDV_PRE_H_INCLUDED

typedef struct MPIDI_am_rndv_req {
    const void *src_buf;
    MPI_Count count;
    MPI_Datatype datatype;
    int rank;
    int tag;
    MPIR_Context_id_t context_id;
} MPIDI_am_rndv_req_t;

#endif /* MPIDI_AM_RNDV_PRE_H_INCLUDED */
