/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

int MPID_Send_enqueue(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                      int dest, int tag, MPIR_Comm * comm_ptr)
{
    return MPIR_Send_enqueue_impl(buf, count, datatype, dest, tag, comm_ptr);
}

int MPID_Recv_enqueue(void *buf, MPI_Aint count, MPI_Datatype datatype,
                      int source, int tag, MPIR_Comm * comm_ptr, MPI_Status * status)
{
    return MPIR_Recv_enqueue_impl(buf, count, datatype, source, tag, comm_ptr, status);
}
