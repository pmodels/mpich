/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef POSIX_PRE_H_INCLUDED
#define POSIX_PRE_H_INCLUDED

#include <mpi.h>

struct MPIR_Request;
struct MPIR_Segment;

typedef struct {
    struct MPIR_Request *next;
    struct MPIR_Request *pending;
    int dest;
    int rank;
    int tag;
    int context_id;
    char *user_buf;
    size_t data_sz;
    int type;
    int user_count;
    MPI_Datatype datatype;
    struct MPIR_Segment *segment_ptr;
    size_t segment_first;
    size_t segment_size;
} MPIDI_POSIX_request_t;

typedef struct {
    int dummy;
} MPIDI_POSIX_comm_t;

#include "posix_coll_params.h"
#include "posix_coll_containers.h"
#endif /* POSIX_PRE_H_INCLUDED */
