/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* ---- Send stream ---- */
struct send_data {
    const void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    int dest;
    int tag;
    MPIR_Comm *comm_ptr;
};

static void send_stream_cb(void *data)
{
    int mpi_errno;
    MPIR_Request *request_ptr = NULL;

    struct send_data *p = data;
    mpi_errno = MPID_Send(p->buf, p->count, p->datatype, p->dest, p->tag, p->comm_ptr,
                          MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    assert(mpi_errno == MPI_SUCCESS);
    assert(request_ptr != NULL);

    mpi_errno = MPID_Wait(request_ptr, MPI_STATUS_IGNORE);
    assert(mpi_errno == MPI_SUCCESS);

    MPIR_Request_free(request_ptr);

    MPL_free(data);
}

int MPIR_Send_enqueue_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                           int dest, int tag, MPIR_Comm * comm_ptr, void *stream)
{
    int mpi_errno = MPI_SUCCESS;

    struct send_data *p;
    p = MPL_malloc(sizeof(struct send_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    p->buf = buf;
    p->count = count;
    p->datatype = datatype;
    p->dest = dest;
    p->tag = tag;
    p->comm_ptr = comm_ptr;

    MPL_gpu_launch_hostfn(stream, send_stream_cb, p);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- Recv stream ---- */
struct recv_data {
    void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    int source;
    int tag;
    MPIR_Comm *comm_ptr;
    MPI_Status *status;
};

static void recv_stream_cb(void *data)
{
    int mpi_errno;
    MPIR_Request *request_ptr = NULL;

    struct recv_data *p = data;
    mpi_errno = MPID_Recv(p->buf, p->count, p->datatype, p->source, p->tag, p->comm_ptr,
                          MPIR_CONTEXT_INTRA_PT2PT, p->status, &request_ptr);
    assert(mpi_errno == MPI_SUCCESS);
    assert(request_ptr != NULL);

    mpi_errno = MPID_Wait(request_ptr, MPI_STATUS_IGNORE);
    assert(mpi_errno == MPI_SUCCESS);

    MPIR_Request_extract_status(request_ptr, p->status);
    MPIR_Request_free(request_ptr);

    MPL_free(data);
}

int MPIR_Recv_enqueue_impl(void *buf, MPI_Aint count, MPI_Datatype datatype,
                           int source, int tag, MPIR_Comm * comm_ptr, MPI_Status * status,
                           void *stream)
{
    int mpi_errno = MPI_SUCCESS;

    struct recv_data *p;
    p = MPL_malloc(sizeof(struct recv_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    p->buf = buf;
    p->count = count;
    p->datatype = datatype;
    p->source = source;
    p->tag = tag;
    p->comm_ptr = comm_ptr;
    p->status = status;

    MPL_gpu_launch_hostfn(stream, recv_stream_cb, p);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
