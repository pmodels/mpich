/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_AM_H_INCLUDED
#define UCX_AM_H_INCLUDED

#include "ucx_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank,
                                               MPIR_Comm * comm,
                                               int handler_id,
                                               const void *am_hdr,
                                               MPI_Aint am_hdr_sz,
                                               const void *data,
                                               MPI_Aint count, MPI_Datatype datatype,
                                               int src_vci, int dst_vci, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    MPIDI_UCX_am_header_t ucx_hdr;

    MPIR_FUNC_ENTER;

    MPIR_Assert(src_vci < MPIDI_UCX_global.num_vcis);
    MPIR_Assert(dst_vci < MPIDI_UCX_global.num_vcis);
    ep = MPIDI_UCX_COMM_TO_EP(comm, rank, src_vci, dst_vci);

    int dt_contig;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.src_vci = src_vci;
    ucx_hdr.dst_vci = dst_vci;
    ucx_hdr.data_sz = data_sz;

#ifdef HAVE_UCP_AM_NBX
    size_t header_size = sizeof(ucx_hdr) + am_hdr_sz;
    void *send_buf, *header, *data_ptr;
    /* note: since we are not copying large contig gpu data, it is less useful
     * to use MPIR_gpu_malloc_host */
    if (dt_contig) {
        /* only need copy headers */
        send_buf = MPL_malloc(header_size, MPL_MEM_OTHER);
        MPIR_Assert(send_buf);
        header = send_buf;

        MPIR_Memcpy(header, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy((char *) header + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

        data_ptr = (char *) data + dt_true_lb;
    } else {
        /* need copy headers and pack data */
        send_buf = MPL_malloc(header_size + data_sz, MPL_MEM_OTHER);
        MPIR_Assert(send_buf);
        header = send_buf;
        data_ptr = (char *) send_buf + header_size;

        MPIR_Memcpy(header, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy((char *) header + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

        MPI_Aint actual_pack_bytes;
        mpi_errno = MPIR_Typerep_pack(data, count, datatype, 0, data_ptr, data_sz,
                                      &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_pack_bytes == data_sz);
    }
    ucp_request_param_t param = {
        .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK,
        .cb.send = &MPIDI_UCX_am_isend_callback_nbx,
    };
    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_am_send_nbx(ep, MPIDI_UCX_AM_NBX_HANDLER_ID,
                                                              header, header_size,
                                                              data_ptr, data_sz, &param);
    MPIDI_UCX_CHK_REQUEST(ucp_request);
    /* if send is done, free all resources and complete the request */
    if (ucp_request == NULL) {
        MPL_free(send_buf);
        MPIDIG_global.origin_cbs[handler_id] (sreq);
        goto fn_exit;
    }

    MPIDI_UCX_AM_SEND_REQUEST(sreq, pack_buffer) = send_buf;
    MPIDI_UCX_AM_SEND_REQUEST(sreq, handler_id) = handler_id;
    ucp_request->req = sreq;
    ucp_request_release(ucp_request);

#else /* !HAVE_UCP_AM_NBX */

    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(data, &attr);
    if (attr.type == MPL_GPU_POINTER_DEV) {
        /* Force packing of GPU buffer in host memory */
        dt_contig = 0;
    }

    char *send_buf;
    void *send_buf_p;
    size_t total_sz;
    ucp_datatype_t dt;
    if (dt_contig) {
        send_buf = MPL_malloc(sizeof(ucx_hdr) + am_hdr_sz, MPL_MEM_OTHER);
        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

        ucp_dt_iov_t *iov = MPIDI_UCX_AM_SEND_REQUEST(sreq, iov);
        iov[0].buffer = send_buf;
        iov[0].length = sizeof(ucx_hdr) + am_hdr_sz;
        iov[1].buffer = MPIR_get_contig_ptr(data, dt_true_lb);
        iov[1].length = data_sz;

        send_buf_p = iov;
        dt = ucp_dt_make_iov();
        total_sz = 2;
    } else {
        send_buf = MPL_malloc(data_sz + am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_OTHER);
        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

        MPI_Aint actual_pack_bytes;
        mpi_errno =
            MPIR_Typerep_pack(data, count, datatype, 0, send_buf + am_hdr_sz + sizeof(ucx_hdr),
                              data_sz, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_pack_bytes == data_sz);

        send_buf_p = send_buf;
        dt = ucp_dt_make_contig(1);
        total_sz = data_sz + am_hdr_sz + sizeof(ucx_hdr);
    }

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_am_send_nb(ep, MPIDI_UCX_AM_HANDLER_ID,
                                                             send_buf_p, total_sz, dt,
                                                             &MPIDI_UCX_am_isend_callback, 0);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    /* send is done. free all resources and complete the request */
    if (ucp_request == NULL) {
        MPL_free(send_buf);
        MPIDIG_global.origin_cbs[handler_id] (sreq);
        goto fn_exit;
    }

    /* set the ch4r request inside the UCP request */
    MPIDI_UCX_AM_SEND_REQUEST(sreq, pack_buffer) = send_buf;
    MPIDI_UCX_AM_SEND_REQUEST(sreq, handler_id) = handler_id;
    ucp_request->req = sreq;
    ucp_request_release(ucp_request);

#endif /* HAVE_UCP_AM_NBX */

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(MPIR_Comm * comm,
                                                     int src_rank,
                                                     int handler_id,
                                                     const void *am_hdr,
                                                     MPI_Aint am_hdr_sz,
                                                     const void *data, MPI_Aint count,
                                                     MPI_Datatype datatype,
                                                     int src_vci, int dst_vci, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_NM_am_isend(src_rank, comm, handler_id,
                                  am_hdr, am_hdr_sz, data, count, datatype, src_vci, dst_vci, sreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_hdr_max_sz(void)
{
    return (MPIDI_UCX_MAX_AM_EAGER_SZ - sizeof(MPIDI_UCX_am_header_t));
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_eager_limit(void)
{
    return (MPIDI_UCX_MAX_AM_EAGER_SZ - sizeof(MPIDI_UCX_am_header_t));
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_eager_buf_limit(void)
{
    return MPIDI_UCX_MAX_AM_EAGER_SZ;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank,
                                                  MPIR_Comm * comm,
                                                  int handler_id, const void *am_hdr,
                                                  MPI_Aint am_hdr_sz, int src_vci, int dst_vci)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    char *send_buf;
    MPIDI_UCX_am_header_t ucx_hdr;

    MPIR_FUNC_ENTER;

    MPIR_Assert(src_vci < MPIDI_UCX_global.num_vcis);
    MPIR_Assert(dst_vci < MPIDI_UCX_global.num_vcis);
    ep = MPIDI_UCX_COMM_TO_EP(comm, rank, src_vci, dst_vci);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.src_vci = src_vci;
    ucx_hdr.dst_vci = dst_vci;
    ucx_hdr.data_sz = 0;

    /* just pack and send for now */
    send_buf = MPL_malloc(am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_BUFFER);
    MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_am_send_nb(ep, MPIDI_UCX_AM_HANDLER_ID, send_buf,
                                                             am_hdr_sz + sizeof(ucx_hdr),
                                                             ucp_dt_make_contig(1),
                                                             &MPIDI_UCX_am_send_callback, 0);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        /* inject is done */
        MPL_free(send_buf);
    } else {
        ucp_request->buf = send_buf;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(MPIR_Comm * comm,
                                                        int rank,
                                                        int handler_id, const void *am_hdr,
                                                        MPI_Aint am_hdr_sz,
                                                        int src_vci, int dst_vci)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_NM_am_send_hdr(rank, comm, handler_id, am_hdr, am_hdr_sz, src_vci, dst_vci);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_NM_am_check_eager(MPI_Aint am_hdr_sz, MPI_Aint data_sz,
                                                      const void *data, MPI_Aint count,
                                                      MPI_Datatype datatype, MPIR_Request * sreq)
{
#ifdef HAVE_UCP_AM_NBX
    /* ucx will handle rndv */
    return true;
#else
    return (am_hdr_sz + data_sz) <= (MPIDI_UCX_MAX_AM_EAGER_SZ - sizeof(MPIDI_UCX_am_header_t));
#endif
}

int MPIDI_UCX_do_am_recv(MPIR_Request * rreq);

MPL_STATIC_INLINE_PREFIX MPIDIG_recv_data_copy_cb MPIDI_NM_am_get_data_copy_cb(uint32_t attr)
{
#ifdef HAVE_UCP_AM_NBX
    return MPIDI_UCX_do_am_recv;
#else
    return NULL;
#endif
}

#endif /* UCX_AM_H_INCLUDED */
