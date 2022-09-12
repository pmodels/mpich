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


int MPID_Isend_enqueue(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                       int dest, int tag, MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    return MPIR_Isend_enqueue_impl(buf, count, datatype, dest, tag, comm_ptr, req);
}
                       
int MPID_Irecv_enqueue(void *buf, MPI_Aint count, MPI_Datatype datatype,
                       int source, int tag, MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    return MPIR_Irecv_enqueue_impl(buf, count, datatype, source, tag, comm_ptr, req);
}

int MPID_Wait_enqueue(MPIR_Request * req_ptr, MPI_Status * status)
{
    return MPIR_Wait_enqueue_impl(req_ptr, status);
}

int MPID_Waitall_enqueue(int count, MPI_Request * array_of_requests,
                         MPI_Status * array_of_statuses)
{
    return MPIR_Waitall_enqueue_impl(count, array_of_requests, array_of_statuses);
}
