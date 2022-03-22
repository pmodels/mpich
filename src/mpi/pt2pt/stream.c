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
    void *host_buf;
    MPI_Aint data_sz;
    MPI_Aint actual_pack_bytes;
};

static void send_stream_cb(void *data)
{
    int mpi_errno;
    MPIR_Request *request_ptr = NULL;

    struct send_data *p = data;
    if (p->host_buf) {
        assert(p->actual_pack_bytes == p->data_sz);

        mpi_errno = MPID_Send(p->host_buf, p->data_sz, MPI_BYTE, p->dest, p->tag, p->comm_ptr,
                              MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    } else {
        mpi_errno = MPID_Send(p->buf, p->count, p->datatype, p->dest, p->tag, p->comm_ptr,
                              MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    }
    assert(mpi_errno == MPI_SUCCESS);
    assert(request_ptr != NULL);

    mpi_errno = MPID_Wait(request_ptr, MPI_STATUS_IGNORE);
    assert(mpi_errno == MPI_SUCCESS);

    MPIR_Request_free(request_ptr);

    if (p->host_buf) {
        MPIR_gpu_free_host(p->host_buf);
    }
    MPL_free(data);
}

int MPIR_Send_enqueue_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                           int dest, int tag, MPIR_Comm * comm_ptr, void *stream)
{
    int mpi_errno = MPI_SUCCESS;

    struct send_data *p;
    p = MPL_malloc(sizeof(struct send_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    p->dest = dest;
    p->tag = tag;
    p->comm_ptr = comm_ptr;

    if (MPIR_GPU_query_pointer_is_dev(buf)) {
        MPI_Aint dt_size;
        MPIR_Datatype_get_size_macro(datatype, dt_size);
        p->data_sz = dt_size * count;

        MPIR_gpu_malloc_host(&p->host_buf, p->data_sz);

        mpi_errno = MPIR_Typerep_pack_stream(buf, count, datatype, 0, p->host_buf, p->data_sz,
                                             &p->actual_pack_bytes, stream);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        p->host_buf = NULL;
        p->buf = buf;
        p->count = count;
        p->datatype = datatype;
    }

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
    void *host_buf;
    MPI_Aint data_sz;
    MPI_Aint actual_unpack_bytes;
};

static void recv_stream_cb(void *data)
{
    int mpi_errno;
    MPIR_Request *request_ptr = NULL;

    struct recv_data *p = data;
    if (p->host_buf) {
        mpi_errno = MPID_Recv(p->host_buf, p->data_sz, MPI_BYTE, p->source, p->tag, p->comm_ptr,
                              MPIR_CONTEXT_INTRA_PT2PT, p->status, &request_ptr);
    } else {
        mpi_errno = MPID_Recv(p->buf, p->count, p->datatype, p->source, p->tag, p->comm_ptr,
                              MPIR_CONTEXT_INTRA_PT2PT, p->status, &request_ptr);
    }
    assert(mpi_errno == MPI_SUCCESS);
    assert(request_ptr != NULL);

    mpi_errno = MPID_Wait(request_ptr, MPI_STATUS_IGNORE);
    assert(mpi_errno == MPI_SUCCESS);

    MPIR_Request_extract_status(request_ptr, p->status);
    MPIR_Request_free(request_ptr);

    if (!p->host_buf) {
        /* we are done */
        MPL_free(p);
    }
}

static void recv_stream_cleanup_cb(void *data)
{
    struct recv_data *p = data;
    assert(p->actual_unpack_bytes == p->data_sz);

    MPIR_gpu_free_host(p->host_buf);
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

    p->source = source;
    p->tag = tag;
    p->comm_ptr = comm_ptr;
    p->status = status;

    if (MPIR_GPU_query_pointer_is_dev(buf)) {
        MPI_Aint dt_size;
        MPIR_Datatype_get_size_macro(datatype, dt_size);
        p->data_sz = dt_size * count;

        MPIR_gpu_malloc_host(&p->host_buf, p->data_sz);

        MPL_gpu_launch_hostfn(stream, recv_stream_cb, p);

        mpi_errno = MPIR_Typerep_unpack_stream(p->host_buf, p->data_sz, buf, count, datatype, 0,
                                               &p->actual_unpack_bytes, stream);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_gpu_launch_hostfn(stream, recv_stream_cleanup_cb, p);
    } else {
        p->host_buf = NULL;
        p->buf = buf;
        p->count = count;
        p->datatype = datatype;

        MPL_gpu_launch_hostfn(stream, recv_stream_cb, p);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
