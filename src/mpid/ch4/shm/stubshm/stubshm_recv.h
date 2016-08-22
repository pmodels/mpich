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
#ifndef SHM_STUBSHM_RECV_H_INCLUDED
#define SHM_STUBSHM_RECV_H_INCLUDED

#include "stubshm_impl.h"

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_SHM_recv)
static inline int MPIDI_SHM_recv(void *buf,
                                 int count,
                                 MPI_Datatype datatype,
                                 int rank,
                                 int tag,
                                 MPIR_Comm * comm,
                                 int context_offset, MPI_Status * status, MPIR_Request ** request)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_recv_init(void *buf,
                                      int count,
                                      MPI_Datatype datatype,
                                      int rank,
                                      int tag,
                                      MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}


static inline int MPIDI_SHM_imrecv(void *buf,
                                   int count,
                                   MPI_Datatype datatype,
                                   MPIR_Request * message, MPIR_Request ** rreqp)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_SHM_irecv)
static inline int MPIDI_SHM_irecv(void *buf,
                                  int count,
                                  MPI_Datatype datatype,
                                  int rank,
                                  int tag,
                                  MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_cancel_recv(MPIR_Request * rreq)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* SHM_STUBSHM_RECV_H_INCLUDED */
