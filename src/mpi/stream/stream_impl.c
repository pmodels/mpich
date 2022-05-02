/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifndef MPIR_STREAM_PREALLOC
#define MPIR_STREAM_PREALLOC 8
#endif

#define GPU_STREAM_USE_SINGLE_VCI

#ifdef GPU_STREAM_USE_SINGLE_VCI
static int gpu_stream_vci = 0;
static int gpu_stream_count = 0;

static int allocate_vci(int *vci, bool is_gpu_stream)
{
    if (is_gpu_stream) {
        int mpi_errno = MPI_SUCCESS;
        if (!gpu_stream_vci) {
            mpi_errno = MPID_Allocate_vci(&gpu_stream_vci);
        }
        gpu_stream_count++;
        *vci = gpu_stream_vci;
        return mpi_errno;
    } else {
        return MPID_Allocate_vci(vci);
    }
}

static int deallocate_vci(int vci)
{
    if (vci == gpu_stream_vci) {
        gpu_stream_count--;
        if (gpu_stream_count == 0) {
            gpu_stream_vci = 0;
            return MPID_Deallocate_vci(vci);
        } else {
            return MPI_SUCCESS;
        }
    } else {
        return MPID_Deallocate_vci(vci);
    }
}

#else
static int allocate_vci(int *vci, bool is_gpu_stream)
{
    return MPID_Allocate_vci(vci);
}

static int deallocate_vci(int *vci)
{
    return MPID_Deallocate_vci(vci);
}

#endif

MPIR_Stream MPIR_Stream_direct[MPIR_STREAM_PREALLOC];

MPIR_Object_alloc_t MPIR_Stream_mem = { 0, 0, 0, 0, 0, 0, MPIR_STREAM,
    sizeof(MPIR_Stream), MPIR_Stream_direct,
    MPIR_STREAM_PREALLOC,
    NULL, {0}
};

void MPIR_stream_comm_init(MPIR_Comm * comm)
{
    comm->stream_comm_type = MPIR_STREAM_COMM_NONE;
}

void MPIR_stream_comm_free(MPIR_Comm * comm)
{
    if (comm->stream_comm_type == MPIR_STREAM_COMM_SINGLE) {
        if (comm->stream_comm.single.stream) {
            int cnt;
            MPIR_Object_release_ref_always(comm->stream_comm.single.stream, &cnt);
        }
        MPL_free(comm->stream_comm.single.vci_table);
    } else if (comm->stream_comm_type == MPIR_STREAM_COMM_MULTIPLEX) {
        int rank = comm->rank;
        int num_local_streams = comm->stream_comm.multiplex.vci_displs[rank + 1] -
            comm->stream_comm.multiplex.vci_displs[rank];
        for (int i = 0; i < num_local_streams; i++) {
            if (comm->stream_comm.multiplex.local_streams[i]) {
                int cnt;
                MPIR_Object_release_ref_always(comm->stream_comm.multiplex.local_streams[i], &cnt);
            }
        }
        MPL_free(comm->stream_comm.multiplex.local_streams);
        MPL_free(comm->stream_comm.multiplex.vci_displs);
        MPL_free(comm->stream_comm.multiplex.vci_table);
    }
}

int MPIR_Stream_create_impl(MPIR_Info * info_ptr, MPIR_Stream ** p_stream_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Stream *stream_ptr;
    stream_ptr = (MPIR_Stream *) MPIR_Handle_obj_alloc(&MPIR_Stream_mem);
    MPIR_ERR_CHKANDJUMP1(!stream_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                         "MPI_Stream");

    MPIR_Object_set_ref(stream_ptr, 0);
    stream_ptr->vci = 0;

    const char *s_type;
    s_type = MPIR_Info_lookup(info_ptr, "type");

    if (s_type && strcmp(s_type, "cudaStream_t") == 0) {
#ifndef MPL_HAVE_CUDA
        MPIR_Assert(0 && "CUDA not enabled");
#endif
        stream_ptr->type = MPIR_STREAM_GPU;

        /* TODO: proper conversion for each gpu stream type */
        const char *s_value = MPIR_Info_lookup(info_ptr, "value");
        MPIR_ERR_CHKANDJUMP(!s_value, mpi_errno, MPI_ERR_OTHER, "**missinggpustream");

        mpi_errno =
            MPIR_Info_decode_hex(s_value, &stream_ptr->u.gpu_stream, sizeof(MPL_gpu_stream_t));
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_ERR_CHKANDJUMP(!MPL_gpu_stream_is_valid(stream_ptr->u.gpu_stream),
                            mpi_errno, MPI_ERR_OTHER, "**invalidgpustream");
    } else {
        stream_ptr->type = MPIR_STREAM_GENERAL;
    }

    mpi_errno = allocate_vci(&stream_ptr->vci, stream_ptr->type == MPIR_STREAM_GPU);
    MPIR_ERR_CHECK(mpi_errno);

    *p_stream_ptr = stream_ptr;
  fn_exit:
    return mpi_errno;
  fn_fail:
    if (stream_ptr) {
        MPIR_Stream_free_impl(stream_ptr);
    }
    goto fn_exit;
}

int MPIR_Stream_free_impl(MPIR_Stream * stream_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    int ref_cnt = MPIR_Object_get_ref(stream_ptr);
    MPIR_ERR_CHKANDJUMP(ref_cnt != 0, mpi_errno, MPI_ERR_OTHER, "**cannotfreestream");

    if (stream_ptr->vci) {
        mpi_errno = deallocate_vci(stream_ptr->vci);
    }
    MPIR_Handle_obj_free(&MPIR_Stream_mem, stream_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Stream_comm_create_impl(MPIR_Comm * comm_ptr, MPIR_Stream * stream_ptr,
                                 MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Comm_dup_impl(comm_ptr, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    int vci, *vci_table;
    if (stream_ptr) {
        vci = stream_ptr->vci;
    } else {
        vci = 0;
    }
    vci_table = MPL_malloc(comm_ptr->local_size * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!vci_table, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIR_Errflag_t errflag;
    errflag = MPIR_ERR_NONE;
    mpi_errno = MPIR_Allgather_impl(&vci, 1, MPI_INT, vci_table, 1, MPI_INT, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    (*newcomm_ptr)->stream_comm_type = MPIR_STREAM_COMM_SINGLE;
    (*newcomm_ptr)->stream_comm.single.stream = stream_ptr;
    (*newcomm_ptr)->stream_comm.single.vci_table = vci_table;

    if (stream_ptr) {
        MPIR_Object_add_ref_always(stream_ptr);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Stream_comm_create_multiplex_impl(MPIR_Comm * comm_ptr,
                                           int num_streams, MPIX_Stream streams[],
                                           MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* if user offers 0 streams, use default MPIX_STREAM_NULL */
    MPIX_Stream stream_default = MPIX_STREAM_NULL;
    if (num_streams == 0) {
        num_streams = 1;
        streams = &stream_default;
    }

    mpi_errno = MPIR_Comm_dup_impl(comm_ptr, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPI_Aint *num_table;
    num_table = MPL_malloc(comm_ptr->local_size * sizeof(MPI_Aint), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!num_table, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPI_Aint *displs;
    /* note: we allocate (size + 1) so the counts can be calculated from displs table */
    displs = MPL_malloc((comm_ptr->local_size + 1) * sizeof(MPI_Aint), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!displs, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPI_Aint num_tmp = num_streams;
    mpi_errno = MPIR_Allgather_impl(&num_tmp, 1, MPI_AINT,
                                    num_table, 1, MPI_AINT, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    MPI_Aint num_total = 0;
    for (int i = 0; i < comm_ptr->local_size; i++) {
        displs[i] = num_total;
        num_total += num_table[i];
    }
    displs[comm_ptr->local_size] = num_total;

    int *vci_table;
    vci_table = MPL_malloc(num_total * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!vci_table, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIR_Stream **local_streams;
    local_streams = MPL_malloc(num_streams * sizeof(MPIR_Stream *), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!local_streams, mpi_errno, MPI_ERR_OTHER, "**nomem");

    int *local_vcis;
    local_vcis = MPL_malloc(num_streams * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!local_vcis, mpi_errno, MPI_ERR_OTHER, "**nomem");

    for (int i = 0; i < num_streams; i++) {
        MPIR_Stream *stream_ptr;
        MPIR_Stream_get_ptr(streams[i], stream_ptr);
        if (stream_ptr) {
            MPIR_Object_add_ref_always(stream_ptr);
        }
        local_streams[i] = stream_ptr;
        local_vcis[i] = stream_ptr ? stream_ptr->vci : 0;
    }

    errflag = MPIR_ERR_NONE;
    mpi_errno = MPIR_Allgatherv_impl(local_vcis, num_streams, MPI_INT,
                                     vci_table, num_table, displs, MPI_INT, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    (*newcomm_ptr)->stream_comm_type = MPIR_STREAM_COMM_MULTIPLEX;
    (*newcomm_ptr)->stream_comm.multiplex.local_streams = local_streams;
    (*newcomm_ptr)->stream_comm.multiplex.vci_displs = displs;
    (*newcomm_ptr)->stream_comm.multiplex.vci_table = vci_table;

    MPL_free(local_vcis);
    MPL_free(num_table);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- CUDA stream send/recv enqueue ---- */

static int get_local_gpu_stream(MPIR_Comm * comm_ptr, MPL_gpu_stream_t * gpu_stream)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Stream *stream_ptr = NULL;
    if (comm_ptr->stream_comm_type == MPIR_STREAM_COMM_SINGLE) {
        stream_ptr = comm_ptr->stream_comm.single.stream;
    } else if (comm_ptr->stream_comm_type == MPIR_STREAM_COMM_MULTIPLEX) {
        stream_ptr = comm_ptr->stream_comm.multiplex.local_streams[comm_ptr->rank];
    }

    MPIR_ERR_CHKANDJUMP(!stream_ptr || stream_ptr->type != MPIR_STREAM_GPU,
                        mpi_errno, MPI_ERR_OTHER, "**notgpustream");
    *gpu_stream = stream_ptr->u.gpu_stream;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int allocate_enqueue_request(MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Stream *stream_ptr = NULL;
    if (comm_ptr->stream_comm_type == MPIR_STREAM_COMM_SINGLE) {
        stream_ptr = comm_ptr->stream_comm.single.stream;
    } else if (comm_ptr->stream_comm_type == MPIR_STREAM_COMM_MULTIPLEX) {
        stream_ptr = comm_ptr->stream_comm.multiplex.local_streams[comm_ptr->rank];
    }
    MPIR_Assert(stream_ptr);

    int vci = stream_ptr->vci;
    MPIR_Assert(vci > 0);

    /* stream vci are only accessed within a serialized context */
    (*req) = MPIR_Request_create_from_pool_safe(MPIR_REQUEST_KIND__ENQUEUE, vci, 1);
    (*req)->u.enqueue.gpu_stream = stream_ptr->u.gpu_stream;
    (*req)->u.enqueue.real_request = NULL;

    return mpi_errno;
}

/* send enqueue */
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
    /* for isend */
    MPIR_Request *req;
};

static void send_enqueue_cb(void *data)
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
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);
    assert(request_ptr != NULL);

    mpi_errno = MPID_Wait(request_ptr, MPI_STATUS_IGNORE);
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);

    MPIR_Request_free(request_ptr);

    if (p->host_buf) {
        MPIR_gpu_free_host(p->host_buf);
    }
    MPL_free(data);
}

int MPIR_Send_enqueue_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                           int dest, int tag, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_stream_t gpu_stream;
    mpi_errno = get_local_gpu_stream(comm_ptr, &gpu_stream);
    MPIR_ERR_CHECK(mpi_errno);

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
                                             &p->actual_pack_bytes, &gpu_stream);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        p->host_buf = NULL;
        p->buf = buf;
        p->count = count;
        p->datatype = datatype;
    }

    MPL_gpu_launch_hostfn(gpu_stream, send_enqueue_cb, p);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- recv enqueue ---- */
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
    /* for irend */
    MPIR_Request *req;
};

static void recv_enqueue_cb(void *data)
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
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);
    assert(request_ptr != NULL);

    mpi_errno = MPID_Wait(request_ptr, MPI_STATUS_IGNORE);
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);

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
                           int source, int tag, MPIR_Comm * comm_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_stream_t gpu_stream;
    mpi_errno = get_local_gpu_stream(comm_ptr, &gpu_stream);
    MPIR_ERR_CHECK(mpi_errno);

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

        MPL_gpu_launch_hostfn(gpu_stream, recv_enqueue_cb, p);

        mpi_errno = MPIR_Typerep_unpack_stream(p->host_buf, p->data_sz, buf, count, datatype, 0,
                                               &p->actual_unpack_bytes, &gpu_stream);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_gpu_launch_hostfn(gpu_stream, recv_stream_cleanup_cb, p);
    } else {
        p->host_buf = NULL;
        p->buf = buf;
        p->count = count;
        p->datatype = datatype;

        MPL_gpu_launch_hostfn(gpu_stream, recv_enqueue_cb, p);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- isend enqueue ---- */
static void isend_enqueue_cb(void *data)
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
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);
    assert(request_ptr != NULL);

    p->req->u.enqueue.real_request = request_ptr;
}

int MPIR_Isend_enqueue_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                            int dest, int tag, MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_stream_t gpu_stream;
    mpi_errno = get_local_gpu_stream(comm_ptr, &gpu_stream);
    MPIR_ERR_CHECK(mpi_errno);

    struct send_data *p;
    p = MPL_malloc(sizeof(struct send_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    mpi_errno = allocate_enqueue_request(comm_ptr, req);
    MPIR_ERR_CHECK(mpi_errno);
    (*req)->u.enqueue.is_send = true;
    (*req)->u.enqueue.data = p;

    p->req = *req;
    p->dest = dest;
    p->tag = tag;
    p->comm_ptr = comm_ptr;

    if (MPIR_GPU_query_pointer_is_dev(buf)) {
        MPI_Aint dt_size;
        MPIR_Datatype_get_size_macro(datatype, dt_size);
        p->data_sz = dt_size * count;

        MPIR_gpu_malloc_host(&p->host_buf, p->data_sz);

        mpi_errno = MPIR_Typerep_pack_stream(buf, count, datatype, 0, p->host_buf, p->data_sz,
                                             &p->actual_pack_bytes, &gpu_stream);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        p->host_buf = NULL;
        p->buf = buf;
        p->count = count;
        p->datatype = datatype;
    }

    MPL_gpu_launch_hostfn(gpu_stream, isend_enqueue_cb, p);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- irecv enqueue ---- */
static void irecv_enqueue_cb(void *data)
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
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);
    assert(request_ptr != NULL);

    p->req->u.enqueue.real_request = request_ptr;
}

int MPIR_Irecv_enqueue_impl(void *buf, MPI_Aint count, MPI_Datatype datatype,
                            int source, int tag, MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_stream_t gpu_stream;
    mpi_errno = get_local_gpu_stream(comm_ptr, &gpu_stream);
    MPIR_ERR_CHECK(mpi_errno);

    struct recv_data *p;
    p = MPL_malloc(sizeof(struct recv_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    mpi_errno = allocate_enqueue_request(comm_ptr, req);
    MPIR_ERR_CHECK(mpi_errno);
    (*req)->u.enqueue.is_send = false;
    (*req)->u.enqueue.data = p;

    p->req = *req;
    p->source = source;
    p->tag = tag;
    p->comm_ptr = comm_ptr;
    p->status = MPI_STATUS_IGNORE;

    if (MPIR_GPU_query_pointer_is_dev(buf)) {
        MPI_Aint dt_size;
        MPIR_Datatype_get_size_macro(datatype, dt_size);
        p->data_sz = dt_size * count;

        MPIR_gpu_malloc_host(&p->host_buf, p->data_sz);

        MPL_gpu_launch_hostfn(gpu_stream, recv_enqueue_cb, p);

        mpi_errno = MPIR_Typerep_unpack_stream(p->host_buf, p->data_sz, buf, count, datatype, 0,
                                               &p->actual_unpack_bytes, &gpu_stream);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_gpu_launch_hostfn(gpu_stream, recv_stream_cleanup_cb, p);
    } else {
        p->host_buf = NULL;
        p->buf = buf;
        p->count = count;
        p->datatype = datatype;

        MPL_gpu_launch_hostfn(gpu_stream, irecv_enqueue_cb, p);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- wait enqueue ---- */
static void wait_enqueue_cb(void *data)
{
    int mpi_errno;
    MPIR_Request *enqueue_req = data;
    MPIR_Request *real_req = enqueue_req->u.enqueue.real_request;

    if (enqueue_req->u.enqueue.is_send) {
        struct send_data *p = enqueue_req->u.enqueue.data;

        mpi_errno = MPID_Wait(real_req, MPI_STATUS_IGNORE);
        MPIR_Assertp(mpi_errno == MPI_SUCCESS);

        MPIR_Request_free(real_req);

        if (p->host_buf) {
            MPIR_gpu_free_host(p->host_buf);
        }
        MPL_free(p);
    } else {
        struct recv_data *p = enqueue_req->u.enqueue.data;

        mpi_errno = MPID_Wait(real_req, MPI_STATUS_IGNORE);
        MPIR_Assertp(mpi_errno == MPI_SUCCESS);

        MPIR_Request_extract_status(real_req, p->status);
        MPIR_Request_free(real_req);

        if (!p->host_buf) {
            MPL_free(p);
        }
    }
    MPIR_Request_free(enqueue_req);
}

int MPIR_Wait_enqueue_impl(MPIR_Request * req_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(req_ptr && req_ptr->kind == MPIR_REQUEST_KIND__ENQUEUE);

    MPL_gpu_stream_t gpu_stream = req_ptr->u.enqueue.gpu_stream;
    if (!req_ptr->u.enqueue.is_send) {
        struct recv_data *p = req_ptr->u.enqueue.data;
        p->status = status;
    }

    MPL_gpu_launch_hostfn(gpu_stream, wait_enqueue_cb, req_ptr);

    return mpi_errno;
}

/* ---- waitall enqueue ---- */
struct waitall_data {
    int count;
    MPI_Request *array_of_requests;
    MPI_Status *array_of_statuses;
};

static void waitall_enqueue_cb(void *data)
{
    struct waitall_data *p = data;

    MPI_Request *reqs = MPL_malloc(p->count * sizeof(MPI_Request), MPL_MEM_OTHER);
    MPIR_Assert(reqs);

    for (int i = 0; i < p->count; i++) {
        MPIR_Request *enqueue_req;
        MPIR_Request_get_ptr(p->array_of_requests[i], enqueue_req);
        reqs[i] = enqueue_req->u.enqueue.real_request->handle;
    }

    MPIR_Waitall(p->count, reqs, p->array_of_statuses);

    for (int i = 0; i < p->count; i++) {
        MPIR_Request *enqueue_req;
        MPIR_Request_get_ptr(p->array_of_requests[i], enqueue_req);

        if (enqueue_req->u.enqueue.is_send) {
            struct send_data *p2 = enqueue_req->u.enqueue.data;
            if (p2->host_buf) {
                MPIR_gpu_free_host(p2->host_buf);
            }
            MPL_free(p2);
        } else {
            struct recv_data *p2 = enqueue_req->u.enqueue.data;
            if (!p2->host_buf) {
                MPL_free(p2);
            }
        }
        MPIR_Request_free(enqueue_req);
    }
    MPL_free(reqs);
    MPL_free(p);
}

int MPIR_Waitall_enqueue_impl(int count, MPI_Request * array_of_requests,
                              MPI_Status * array_of_statuses)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_stream_t gpu_stream = MPL_GPU_STREAM_DEFAULT;
    for (int i = 0; i < count; i++) {
        MPIR_Request *enqueue_req;
        MPIR_Request_get_ptr(array_of_requests[i], enqueue_req);

        MPIR_Assert(enqueue_req && enqueue_req->kind == MPIR_REQUEST_KIND__ENQUEUE);
        if (i == 0) {
            gpu_stream = enqueue_req->u.enqueue.gpu_stream;
        } else {
            MPIR_Assert(gpu_stream == enqueue_req->u.enqueue.gpu_stream);
        }
    }

    struct waitall_data *p;
    p = MPL_malloc(sizeof(struct waitall_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    p->count = count;
    p->array_of_requests = array_of_requests;
    p->array_of_statuses = array_of_statuses;

    MPL_gpu_launch_hostfn(gpu_stream, waitall_enqueue_cb, p);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
