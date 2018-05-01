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
#ifndef STUBSHM_SEND_H_INCLUDED
#define STUBSHM_SEND_H_INCLUDED

#include "stubshm_impl.h"

static inline int MPIDI_STUBSHM_mpi_send(const void *buf,
                                         MPI_Aint count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_SEND);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_SEND);
    return MPI_SUCCESS;
}




static inline int MPIDI_STUBSHM_irsend(const void *buf,
                                       MPI_Aint count,
                                       MPI_Datatype datatype,
                                       int rank,
                                       int tag,
                                       MPIR_Comm * comm, int context_offset,
                                       MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_IRSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_IRSEND);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_IRSEND);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ssend(const void *buf,
                                          MPI_Aint count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_SSEND);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_SSEND);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_send_init(const void *buf,
                                              int count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_SEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_SEND_INIT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_SEND_INIT);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ssend_init(const void *buf,
                                               int count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm,
                                               int context_offset, MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_SSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_SSEND_INIT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_SSEND_INIT);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_bsend_init(const void *buf,
                                               int count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm,
                                               int context_offset, MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_BSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_BSEND_INIT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_BSEND_INIT);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_rsend_init(const void *buf,
                                               int count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm,
                                               int context_offset, MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_RSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_RSEND_INIT);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_RSEND_INIT);
    return MPI_SUCCESS;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_STUBSHM_mpi_isend)
static inline int MPIDI_STUBSHM_mpi_isend(const void *buf,
                                          MPI_Aint count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ISEND);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ISEND);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_issend(const void *buf,
                                           MPI_Aint count,
                                           MPI_Datatype datatype,
                                           int rank,
                                           int tag,
                                           MPIR_Comm * comm, int context_offset,
                                           MPIR_Request ** request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ISSEND);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ISSEND);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_cancel_send(MPIR_Request * sreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_CANCEL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_CANCEL_SEND);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_CANCEL_SEND);
    return MPI_SUCCESS;
}

#endif /* STUBSHM_SEND_H_INCLUDED */
