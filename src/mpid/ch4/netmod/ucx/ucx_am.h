/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_AM_H_INCLUDED
#define UCX_AM_H_INCLUDED

#include "ucx_impl.h"

#define MPIDI_UCX_SHORT_DATA_SZ  16 * 1024

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_isend_callback(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_AM_ISEND_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_AM_ISEND_CALLBACK);

    if (ucp_request->u.am_send.pack_buffer) {
        MPL_free(ucp_request->u.am_send.pack_buffer);
    }
    if (ucp_request->u.am_send.sreq) {
        MPIDIG_global.origin_cbs[ucp_request->u.am_send.handler_id] (ucp_request->u.am_send.sreq);
    }
    MPIDI_UCX_UCP_REQUEST_RELEASE(ucp_request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_ISEND_CALLBACK);
}

/* internal routine for sending data. `send_buf` will be freed on completion. If there is
   `payload` (when not NULL), an iov of length 2 will be used to avoid extra copy. */
MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_am_send_internal(int rank, MPIR_Comm * comm, int handler_id,
                                                        void *send_buf, MPI_Aint send_size,
                                                        void *payload, MPI_Aint payload_size,
                                                        MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, rank);
    uint64_t ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    MPIDI_UCX_ucp_request_t *ucp_request;
    if (payload == NULL) {
        ucp_request = (void *) ucp_tag_send_nb(ep, send_buf, send_size,
                                               ucp_dt_make_contig(1), ucx_tag,
                                               &MPIDI_UCX_am_isend_callback);
    } else {
        MPIR_Assert(sreq);
        ucp_dt_iov_t *iov = MPIDI_UCX_AM_REQUEST(sreq, iov);
        iov[0].buffer = send_buf;
        iov[0].length = send_size;
        iov[1].buffer = payload;
        iov[1].length = payload_size;
        ucp_request = (void *) ucp_tag_send_nb(ep, iov, 2,
                                               ucp_dt_make_iov(), ucx_tag,
                                               &MPIDI_UCX_am_isend_callback);
    }
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        /* immediately completed */
        MPL_free(send_buf);
        if (sreq) {
            MPIDIG_global.origin_cbs[handler_id] (sreq);
        }
    } else {
        /* let callback to complete */
        ucp_request->u.am_send.sreq = sreq;
        ucp_request->u.am_send.handler_id = handler_id;
        ucp_request->u.am_send.pack_buffer = send_buf;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_do_am_isend(int rank,
                                                   MPIR_Comm * comm,
                                                   int handler_id,
                                                   const void *am_hdr,
                                                   size_t am_hdr_sz,
                                                   const void *data,
                                                   MPI_Count count, MPI_Datatype datatype,
                                                   MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    int dt_contig;
    MPIDI_UCX_am_header_t ucx_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = data_sz;

    char *send_buf;
    MPI_Aint send_size;
    if (dt_contig) {
        send_size = sizeof(ucx_hdr) + am_hdr_sz + data_sz;
        if (send_size <= MPIDI_UCX_SHORT_DATA_SZ) {
            /* just pack and send for now */
            send_buf = MPL_malloc(send_size, MPL_MEM_BUFFER);
            MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
            MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);
            MPIR_Memcpy(send_buf + am_hdr_sz + sizeof(ucx_hdr), (char *) data + dt_true_lb,
                        data_sz);
            mpi_errno = MPIDI_UCX_am_send_internal(rank, comm, handler_id, send_buf, send_size,
                                                   NULL, 0, sreq);
        } else {
            /* pack headers and send payload as a 2nd iov */
            send_size = sizeof(ucx_hdr) + am_hdr_sz;
            send_buf = MPL_malloc(send_size, MPL_MEM_BUFFER);
            MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
            MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);
            mpi_errno = MPIDI_UCX_am_send_internal(rank, comm, handler_id, send_buf, send_size,
                                                   (char *) data + dt_true_lb, data_sz, sreq);
        }
    } else {
        send_size = sizeof(ucx_hdr) + am_hdr_sz + data_sz;
        send_buf = MPL_malloc(send_size, MPL_MEM_BUFFER);

        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

        MPI_Aint actual_pack_bytes;
        mpi_errno = MPIR_Typerep_pack(data, count, datatype, 0,
                                      send_buf + am_hdr_sz + sizeof(ucx_hdr), data_sz,
                                      &actual_pack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_pack_bytes == data_sz);
        mpi_errno = MPIDI_UCX_am_send_internal(rank, comm, handler_id, send_buf, send_size,
                                               NULL, 0, sreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank,
                                               MPIR_Comm * comm,
                                               int handler_id,
                                               const void *am_hdr,
                                               size_t am_hdr_sz,
                                               const void *data,
                                               MPI_Count count, MPI_Datatype datatype,
                                               MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND);

    mpi_errno = MPIDI_UCX_do_am_isend(rank, comm, handler_id, am_hdr, am_hdr_sz,
                                      data, count, datatype, sreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isendv(int rank,
                                                MPIR_Comm * comm,
                                                int handler_id,
                                                struct iovec *am_hdr,
                                                size_t iov_len,
                                                const void *data,
                                                MPI_Count count, MPI_Datatype datatype,
                                                MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t am_hdr_sz = 0, i;
    char *send_buf;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    int dt_contig;
    MPIDI_UCX_am_header_t ucx_hdr;


    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISENDV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISENDV);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    for (i = 0; i < iov_len; i++) {
        am_hdr_sz += am_hdr[i].iov_len;
    }

    /* copy headers to send buffer */
    send_buf = MPL_malloc(data_sz + am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_BUFFER);
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = data_sz;
    MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
    am_hdr_sz = 0;
    for (i = 0; i < iov_len; i++) {
        MPIR_Memcpy(send_buf + am_hdr_sz + sizeof(ucx_hdr), am_hdr[i].iov_base, am_hdr[i].iov_len);
        am_hdr_sz += am_hdr[i].iov_len;
    }

    if (dt_contig) {
        MPIR_Memcpy(send_buf + am_hdr_sz + sizeof(ucx_hdr), (char *) data + dt_true_lb, data_sz);
    } else {
        MPI_Aint actual_pack_bytes;
        mpi_errno =
            MPIR_Typerep_pack(data, count, datatype, 0, send_buf + sizeof(ucx_hdr) + am_hdr_sz,
                              data_sz, &actual_pack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_pack_bytes == data_sz);
    }

    int send_size = data_sz + am_hdr_sz + sizeof(ucx_hdr);
    mpi_errno = MPIDI_UCX_am_send_internal(rank, comm, handler_id, send_buf, send_size,
                                           NULL, 0, sreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISENDV);
    return mpi_errno;

  fn_fail:
    goto fn_exit;

}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(MPIR_Context_id_t context_id,
                                                     int src_rank,
                                                     int handler_id,
                                                     const void *am_hdr,
                                                     size_t am_hdr_sz,
                                                     const void *data, MPI_Count count,
                                                     MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);

    MPIR_Comm *comm = MPIDIG_context_id_to_comm(context_id);
    mpi_errno = MPIDI_UCX_do_am_isend(src_rank, comm, handler_id, am_hdr, am_hdr_sz,
                                      data, count, datatype, sreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_hdr_max_sz(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);

    ret = (MPIDI_UCX_MAX_AM_EAGER_SZ - sizeof(MPIDI_UCX_am_header_t));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);
    return ret;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_eager_limit(void)
{
    return (MPIDI_UCX_MAX_AM_EAGER_SZ - sizeof(MPIDI_UCX_am_header_t));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank,
                                                  MPIR_Comm * comm,
                                                  int handler_id, const void *am_hdr,
                                                  size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR);

    mpi_errno =
        MPIDI_UCX_do_am_isend(rank, comm, handler_id, am_hdr, am_hdr_sz, NULL, 0, MPI_BYTE, NULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                        int src_rank,
                                                        int handler_id, const void *am_hdr,
                                                        size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);

    MPIR_Comm *comm = MPIDIG_context_id_to_comm(context_id);
    MPIDI_NM_am_send_hdr(src_rank, comm, handler_id, am_hdr, am_hdr_sz);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* UCX_AM_H_INCLUDED */
