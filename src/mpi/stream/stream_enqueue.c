/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Allow per-thread level disabling GPU path */
MPL_TLS bool MPIR_disable_gpu = false;

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
        MPIR_Assertp(p->actual_pack_bytes == p->data_sz);

        mpi_errno = MPID_Send(p->host_buf, p->data_sz, MPI_BYTE, p->dest, p->tag, p->comm_ptr,
                              MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    } else {
        mpi_errno = MPID_Send(p->buf, p->count, p->datatype, p->dest, p->tag, p->comm_ptr,
                              MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    }
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);
    MPIR_Assertp(request_ptr != NULL);

    mpi_errno = MPID_Wait(request_ptr, MPI_STATUS_IGNORE);
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);

    MPIR_Request_free(request_ptr);

    if (p->host_buf) {
        MPIR_gpu_free_host(p->host_buf);
    }
    MPIR_Comm_release(p->comm_ptr);
    MPL_free(data);
}

int MPIR_Send_enqueue_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                           int dest, int tag, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_stream_t gpu_stream;
    mpi_errno = MPIR_get_local_gpu_stream(comm_ptr, &gpu_stream);
    MPIR_ERR_CHECK(mpi_errno);

    struct send_data *p;
    p = MPL_malloc(sizeof(struct send_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    p->dest = dest;
    p->tag = tag;
    p->comm_ptr = comm_ptr;
    MPIR_Comm_add_ref(comm_ptr);

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
    MPIR_Assertp(request_ptr != NULL);

    mpi_errno = MPID_Wait(request_ptr, MPI_STATUS_IGNORE);
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);

    MPIR_Request_extract_status(request_ptr, p->status);
    MPIR_Request_free(request_ptr);

    if (!p->host_buf) {
        /* we are done */
        MPIR_Comm_release(p->comm_ptr);
        MPL_free(p);
    }
}

static void recv_stream_cleanup_cb(void *data)
{
    struct recv_data *p = data;
    MPIR_Assertp(p->actual_unpack_bytes == p->data_sz);

    MPIR_gpu_free_host(p->host_buf);
    MPIR_Comm_release(p->comm_ptr);
    MPL_free(p);
}

int MPIR_Recv_enqueue_impl(void *buf, MPI_Aint count, MPI_Datatype datatype,
                           int source, int tag, MPIR_Comm * comm_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_stream_t gpu_stream;
    mpi_errno = MPIR_get_local_gpu_stream(comm_ptr, &gpu_stream);
    MPIR_ERR_CHECK(mpi_errno);

    struct recv_data *p;
    p = MPL_malloc(sizeof(struct recv_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    p->source = source;
    p->tag = tag;
    p->comm_ptr = comm_ptr;
    p->status = status;
    MPIR_Comm_add_ref(comm_ptr);

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
        MPIR_Assertp(p->actual_pack_bytes == p->data_sz);

        mpi_errno = MPID_Send(p->host_buf, p->data_sz, MPI_BYTE, p->dest, p->tag, p->comm_ptr,
                              MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    } else {
        mpi_errno = MPID_Send(p->buf, p->count, p->datatype, p->dest, p->tag, p->comm_ptr,
                              MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    }
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);
    MPIR_Assertp(request_ptr != NULL);

    p->req->u.enqueue.real_request = request_ptr;
}

int MPIR_Isend_enqueue_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                            int dest, int tag, MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_stream_t gpu_stream;
    mpi_errno = MPIR_get_local_gpu_stream(comm_ptr, &gpu_stream);
    MPIR_ERR_CHECK(mpi_errno);

    struct send_data *p;
    p = MPL_malloc(sizeof(struct send_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    mpi_errno = MPIR_allocate_enqueue_request(comm_ptr, req);
    MPIR_ERR_CHECK(mpi_errno);
    (*req)->u.enqueue.is_send = true;
    (*req)->u.enqueue.data = p;

    p->req = *req;
    p->dest = dest;
    p->tag = tag;
    p->comm_ptr = comm_ptr;
    MPIR_Comm_add_ref(comm_ptr);

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
    MPIR_Assertp(request_ptr != NULL);

    p->req->u.enqueue.real_request = request_ptr;
}

int MPIR_Irecv_enqueue_impl(void *buf, MPI_Aint count, MPI_Datatype datatype,
                            int source, int tag, MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_stream_t gpu_stream;
    mpi_errno = MPIR_get_local_gpu_stream(comm_ptr, &gpu_stream);
    MPIR_ERR_CHECK(mpi_errno);

    struct recv_data *p;
    p = MPL_malloc(sizeof(struct recv_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    mpi_errno = MPIR_allocate_enqueue_request(comm_ptr, req);
    MPIR_ERR_CHECK(mpi_errno);
    (*req)->u.enqueue.is_send = false;
    (*req)->u.enqueue.data = p;

    p->req = *req;
    p->source = source;
    p->tag = tag;
    p->comm_ptr = comm_ptr;
    p->status = MPI_STATUS_IGNORE;
    p->buf = buf;
    p->count = count;
    p->datatype = datatype;
    MPIR_Comm_add_ref(comm_ptr);

    if (MPIR_GPU_query_pointer_is_dev(buf)) {
        MPI_Aint dt_size;
        MPIR_Datatype_get_size_macro(datatype, dt_size);
        p->data_sz = dt_size * count;

        MPIR_gpu_malloc_host(&p->host_buf, p->data_sz);

        MPL_gpu_launch_hostfn(gpu_stream, irecv_enqueue_cb, p);
    } else {
        p->host_buf = NULL;

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
    MPIR_Assert(real_req);

    if (enqueue_req->u.enqueue.is_send) {
        struct send_data *p = enqueue_req->u.enqueue.data;

        mpi_errno = MPID_Wait(real_req, MPI_STATUS_IGNORE);
        MPIR_Assertp(mpi_errno == MPI_SUCCESS);

        MPIR_Request_free(real_req);

        if (p->host_buf) {
            MPIR_gpu_free_host(p->host_buf);
        }
        MPIR_Comm_release(p->comm_ptr);
        MPL_free(p);
    } else {
        struct recv_data *p = enqueue_req->u.enqueue.data;

        mpi_errno = MPID_Wait(real_req, MPI_STATUS_IGNORE);
        MPIR_Assertp(mpi_errno == MPI_SUCCESS);

        MPIR_Request_extract_status(real_req, p->status);
        MPIR_Request_free(real_req);

        if (!p->host_buf) {
            MPIR_Comm_release(p->comm_ptr);
            MPL_free(p);
        }
    }
    MPIR_Request_free(enqueue_req);
}

int MPIR_Wait_enqueue_impl(MPIR_Request * req_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(req_ptr && req_ptr->kind == MPIR_REQUEST_KIND__ENQUEUE);

    MPL_gpu_stream_t gpu_stream = req_ptr->u.enqueue.stream_ptr->u.gpu_stream;
    if (!req_ptr->u.enqueue.is_send) {
        struct recv_data *p = req_ptr->u.enqueue.data;
        p->status = status;
    }

    MPL_gpu_launch_hostfn(gpu_stream, wait_enqueue_cb, req_ptr);

    if (!req_ptr->u.enqueue.is_send) {
        struct recv_data *p = req_ptr->u.enqueue.data;
        if (p->host_buf) {
            mpi_errno = MPIR_Typerep_unpack_stream(p->host_buf, p->data_sz,
                                                   p->buf, p->count, p->datatype, 0,
                                                   &p->actual_unpack_bytes, &gpu_stream);
            MPIR_ERR_CHECK(mpi_errno);

            MPL_gpu_launch_hostfn(gpu_stream, recv_stream_cleanup_cb, p);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- waitall enqueue ---- */
struct waitall_data {
    int count;
    MPI_Request *array_of_requests;     /* need malloc and free */
    MPI_Status *array_of_statuses;      /* user allocated */
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
            MPIR_Comm_release(p2->comm_ptr);
            MPL_free(p2);
        } else {
            struct recv_data *p2 = enqueue_req->u.enqueue.data;
            if (!p2->host_buf) {
                MPIR_Comm_release(p2->comm_ptr);
                MPL_free(p2);
            }
        }
        MPIR_Request_free(enqueue_req);
    }
    MPL_free(reqs);
    MPL_free(p->array_of_requests);
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
            gpu_stream = enqueue_req->u.enqueue.stream_ptr->u.gpu_stream;
        } else {
            MPIR_Assert(gpu_stream == enqueue_req->u.enqueue.stream_ptr->u.gpu_stream);
        }
    }

    struct waitall_data *p;
    p = MPL_malloc(sizeof(struct waitall_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    p->count = count;
    /* copy array_of_requests because they are of input semantics. Reset to MPI_REQUEST_NULL
     * to signify that we are taking over the ownerships */
    p->array_of_requests = MPL_malloc(count * sizeof(MPI_Request), MPL_MEM_OTHER);
    for (int i = 0; i < count; i++) {
        p->array_of_requests[i] = array_of_requests[i];
        array_of_requests[i] = MPI_REQUEST_NULL;
    }
    /* User is still responsible for array_of_statuses since they may need to check them */
    p->array_of_statuses = array_of_statuses;

    MPL_gpu_launch_hostfn(gpu_stream, waitall_enqueue_cb, p);

    for (int i = 0; i < count; i++) {
        MPIR_Request *enqueue_req;
        MPIR_Request_get_ptr(p->array_of_requests[i], enqueue_req);
        if (!enqueue_req->u.enqueue.is_send) {
            struct recv_data *p2 = enqueue_req->u.enqueue.data;
            if (p2->host_buf) {
                mpi_errno = MPIR_Typerep_unpack_stream(p2->host_buf, p2->data_sz,
                                                       p2->buf, p2->count, p2->datatype, 0,
                                                       &p2->actual_unpack_bytes, &gpu_stream);
                MPIR_ERR_CHECK(mpi_errno);

                MPL_gpu_launch_hostfn(gpu_stream, recv_stream_cleanup_cb, p2);
            }

        }

    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- collectives --------------------- */

/* Buffer swapping macros for collectives */

/* NOTE: for non-contig datatype, the localcopy is split between "xxx_impl" and "xxx_cb"
 *       because the hostbuf to tmpbuf are host-to-host copy and MPIR_Typerep_(un)pack_stream
 *       won't work and it has to be called from the callback function.
 */
#define HOST_SWAP_STREAM(hostbuf, devbuf, count, datatype, stream, is_contig, tmpbuf, data_sz) \
    do { \
        if (is_contig) { \
            mpi_errno = MPIR_Localcopy_stream(devbuf, count, datatype, hostbuf, count, datatype, stream); \
            MPIR_ERR_CHECK(mpi_errno); \
        } else { \
            if (!tmpbuf) { \
                tmpbuf = MPL_malloc(data_sz, MPL_MEM_OTHER); \
            } \
            MPI_Aint actual_pack_bytes; \
            mpi_errno = MPIR_Typerep_pack_stream(devbuf, count, datatype, 0, tmpbuf, data_sz, &actual_pack_bytes, stream); \
            MPIR_ERR_CHECK(mpi_errno); \
            MPIR_ERR_CHKANDJUMP(actual_pack_bytes != data_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch"); \
            /* unpack in xxx_cb */ \
        } \
    } while (0)

#define HOST_SWAPBACK_STREAM(hostbuf, devbuf, count, datatype, stream, is_contig, tmpbuf, data_sz) \
    do { \
        if (is_contig) { \
            mpi_errno = MPIR_Localcopy_stream(hostbuf, count, datatype, devbuf, count, datatype, stream); \
        } else { \
            /* pack in xxx_cb */ \
            MPI_Aint actual_unpack_bytes; \
            mpi_errno = MPIR_Typerep_unpack_stream(tmpbuf, data_sz, devbuf, count, datatype, 0, &actual_unpack_bytes, stream); \
            MPIR_ERR_CHECK(mpi_errno); \
            MPIR_ERR_CHKANDJUMP(actual_unpack_bytes != data_sz, mpi_errno, MPI_ERR_TYPE, "**dtypemismatch"); \
        } \
        MPIR_ERR_CHECK(mpi_errno); \
    } while (0)

/* macros for part of localcopy called from callback */
#define HOST_SWAP_IN_CB(hostbuf, count, datatype, tmpbuf, data_sz) \
    do { \
        if (tmpbuf) { \
            MPI_Aint actual_unpack_bytes; \
            mpi_errno = MPIR_Typerep_unpack(tmpbuf, data_sz, hostbuf, count, datatype, 0, &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE); \
            MPIR_Assertp(mpi_errno == MPI_SUCCESS); \
            MPIR_Assertp(actual_unpack_bytes == data_sz); \
        } \
    } while (0)

#define HOST_SWAPBACK_IN_CB(hostbuf, count, datatype, tmpbuf, data_sz) \
    do { \
        if (tmpbuf) { \
            MPI_Aint actual_pack_bytes; \
            mpi_errno = MPIR_Typerep_pack(hostbuf, count, datatype, 0, tmpbuf, data_sz, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE); \
            MPIR_Assertp(mpi_errno == MPI_SUCCESS); \
            MPIR_Assertp(actual_pack_bytes == data_sz); \
        } \
    } while (0)

/* allreduce enqueue */
struct allreduce_data {
    const void *sendbuf;
    void *recvbuf;
    MPI_Aint count;
    MPI_Datatype datatype;
    MPI_Op op;
    MPIR_Comm *comm_ptr;

    bool sendbuf_is_dev;
    void *host_recvbuf;
    void *tmp_buf;              /* noncontig datatype may require tmp_buf for localcopy */
    MPI_Aint data_sz;
    MPI_Aint actual_pack_bytes;
};

static void allreduce_enqueue_cb(void *data)
{
    int mpi_errno;

    /* avoid GPU functions */
    MPIR_disable_gpu = true;

    struct allreduce_data *p = data;

    const void *sendbuf = p->sendbuf;
    void *recvbuf = p->recvbuf;
    if (p->sendbuf == MPI_IN_PLACE) {
        if (p->host_recvbuf) {
            HOST_SWAP_IN_CB(p->host_recvbuf, p->count, p->datatype, p->tmp_buf, p->data_sz);
            recvbuf = p->host_recvbuf;
        }
    } else {
        if (p->host_recvbuf) {
            recvbuf = p->host_recvbuf;
        }
        if (p->sendbuf_is_dev) {
            /* sendbuf copy to recvbuf and replace with MPI_IN_PLACE */
            HOST_SWAP_IN_CB(recvbuf, p->count, p->datatype, p->tmp_buf, p->data_sz);
            sendbuf = MPI_IN_PLACE;
        }
    }

    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, p->count, p->datatype, p->op, p->comm_ptr,
                               &errflag);
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);

    if (p->host_recvbuf) {
        HOST_SWAPBACK_IN_CB(p->host_recvbuf, p->count, p->datatype, p->tmp_buf, p->data_sz);
    }

    if (!p->host_recvbuf) {
        /* we are done */
        MPIR_Comm_release(p->comm_ptr);
        MPL_free(p->tmp_buf);
        MPL_free(p);
    }

    MPIR_disable_gpu = false;
}

static void allred_stream_cleanup_cb(void *data)
{
    struct allreduce_data *p = data;

    MPIR_gpu_host_free(p->host_recvbuf, p->count, p->datatype);

    MPIR_Comm_release(p->comm_ptr);
    MPL_free(p->tmp_buf);
    MPL_free(p);
}

int MPIR_Allreduce_enqueue_impl(const void *sendbuf, void *recvbuf,
                                MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_gpu_stream_t gpu_stream;
    mpi_errno = MPIR_get_local_gpu_stream(comm_ptr, &gpu_stream);
    MPIR_ERR_CHECK(mpi_errno);

    int is_contig;
    MPIR_Datatype_is_contig(datatype, &is_contig);

#ifndef MPL_HAS_TLS
    /* FIXME: check for non-contig case and bail. We can't ensure the callback
     *        won't call GPU functions.
     */
    MPIR_ERR_CHKANDJUMP(!is_contig, mpi_errno, MPI_ERR_OTHER, "**gpu_enqueue_noncontig");
#endif

    struct allreduce_data *p;
    p = MPL_malloc(sizeof(struct allreduce_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    p->sendbuf = sendbuf;
    p->recvbuf = recvbuf;
    p->count = count;
    p->datatype = datatype;
    p->op = op;
    p->comm_ptr = comm_ptr;
    MPIR_Comm_add_ref(comm_ptr);

    p->sendbuf_is_dev = false;
    p->host_recvbuf = NULL;
    p->tmp_buf = NULL;

    MPI_Aint dt_size;
    MPIR_Datatype_get_size_macro(datatype, dt_size);
    p->data_sz = dt_size * count;

    /* Buffer swap. We need additional temp buffer size for noncontig datatype */
    if (sendbuf == MPI_IN_PLACE) {
        if (MPIR_GPU_query_pointer_is_dev(recvbuf)) {
            p->host_recvbuf = MPIR_alloc_buffer(count, datatype);
            HOST_SWAP_STREAM(p->host_recvbuf, recvbuf, count, datatype, &gpu_stream,
                             is_contig, p->tmp_buf, p->data_sz);
        }
    } else {
        void *use_recvbuf = recvbuf;
        if (MPIR_GPU_query_pointer_is_dev(recvbuf)) {
            p->host_recvbuf = MPIR_alloc_buffer(count, datatype);
            use_recvbuf = p->host_recvbuf;
        }
        if (MPIR_GPU_query_pointer_is_dev(sendbuf)) {
            /* copy the sendbuf to recvbuf and do IN_PLACE allreduce in callback */
            p->sendbuf_is_dev = true;
            HOST_SWAP_STREAM(use_recvbuf, sendbuf, count, datatype, &gpu_stream,
                             is_contig, p->tmp_buf, p->data_sz);
        }
    }

    MPL_gpu_launch_hostfn(gpu_stream, allreduce_enqueue_cb, p);

    if (p->host_recvbuf) {
        HOST_SWAPBACK_STREAM(p->host_recvbuf, recvbuf, count, datatype, &gpu_stream,
                             is_contig, p->tmp_buf, p->data_sz);

        MPL_gpu_launch_hostfn(gpu_stream, allred_stream_cleanup_cb, p);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
