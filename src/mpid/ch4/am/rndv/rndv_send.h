/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDI_AM_RNDV_SEND_H_INCLUDED
#define MPIDI_AM_RNDV_SEND_H_INCLUDED

MPL_STATIC_INLINE int MPIDI_am_rndv_send(void *buf, int count, MPI_Datatype datatype, int dest,
                                         int tag, MPIR_Comm * comm_ptr, bool is_local,
                                         MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

#endif /* MPIDI_AM_RNDV_SEND_H_INCLUDED */
