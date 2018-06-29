/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_PT2PT_H_INCLUDED
#define MPIR_PT2PT_H_INCLUDED

int MPIR_Ibsend_impl(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                     MPIR_Comm * comm_ptr, MPI_Request * request);

#endif /* MPIR_PT2PT_H_INCLUDED */
