/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_PT2PT_H_INCLUDED
#define MPIR_PT2PT_H_INCLUDED

int MPIR_Ibsend_impl(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                     MPIR_Comm *comm_ptr, MPI_Request *request);
int MPIR_Test_impl(MPI_Request *request, int *flag, MPI_Status *status);
int MPIR_Testall_impl(int count, MPI_Request array_of_requests[], int *flag,
                      MPI_Status array_of_statuses[]);
int MPIR_Wait_impl(MPI_Request *request, MPI_Status *status);
int MPIR_Waitall_impl(int count, MPI_Request array_of_requests[],
                      MPI_Status array_of_statuses[]);

#endif /* MPIR_PT2PT_H_INCLUDED */
