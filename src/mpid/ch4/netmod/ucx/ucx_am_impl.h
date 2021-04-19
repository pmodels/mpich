/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_AM_IMPL_H_INCLUDED
#define UCX_AM_IMPL_H_INCLUDED

#include "ucx_impl.h"
#include "mpidu_genq.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_isend_short_callback(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *req = ucp_request->req;
    int handler_id = MPIDI_UCX_AMREQUEST(req, handler_id);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_AM_ISEND_SHORT_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_AM_ISEND_SHORT_CALLBACK);

    if (MPIDI_UCX_AMREQUEST(req, is_gpu_pack_buffer)) {
        MPIDU_genq_private_pool_free_cell(MPIDI_UCX_global.pack_buf_pool,
                                          MPIDI_UCX_AMREQUEST(req, pack_buffer));
    } else {
        MPL_free(MPIDI_UCX_AMREQUEST(req, pack_buffer));
    }
    MPIDI_UCX_AMREQUEST(req, pack_buffer) = NULL;
    MPIDIG_global.origin_cbs[handler_id] (req);
    ucp_request->req = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_ISEND_SHORT_CALLBACK);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_isend_bulk_callback(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *req = ucp_request->req;
    int handler_id = MPIDI_UCX_AMREQUEST(req, handler_id);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_AM_ISEND_BULK_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_AM_ISEND_BULK_CALLBACK);

    MPIR_gpu_free_host(MPIDI_UCX_AMREQUEST(req, pack_buffer));
    MPIDI_UCX_AMREQUEST(req, pack_buffer) = NULL;
    MPIDIG_global.origin_cbs[handler_id] (req);
    ucp_request->req = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_ISEND_BULK_CALLBACK);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_do_am_isend_short(int rank, MPIR_Comm * comm, int handler_id,
                                                         const void *am_hdr, MPI_Aint am_hdr_sz,
                                                         const void *data, MPI_Aint count,
                                                         MPI_Datatype datatype, MPIR_Request * sreq,
                                                         bool issue_deferred)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_am_header_t ucx_hdr;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    char *send_buf;
    int dt_contig = 0;
    MPI_Aint data_sz;
    MPI_Aint dt_true_lb, last;
    bool need_packing = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_DO_AM_ISEND_SHORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_DO_AM_ISEND_SHORT);

    if (!issue_deferred) {
        MPIDI_Datatype_check_contig_size(datatype, count, dt_contig, data_sz);

        need_packing = dt_contig ? false : true;

        MPL_pointer_attr_t attr;
        MPIR_GPU_query_pointer_attr(data, &attr);
        if (attr.type == MPL_GPU_POINTER_DEV) {
            /* Force packing of GPU buffer in host memory */
            need_packing = true;
        }

    } else {
        data_sz = MPIDI_UCX_AMREQUEST(sreq, deferred_req)->data_sz;
        need_packing = MPIDI_UCX_AMREQUEST(sreq, deferred_req)->need_packing;
    }

    if (!issue_deferred && MPIDI_UCX_global.deferred_am_isend_q) {
        /* if the deferred queue is not empty, all new ops must be deferred to maintain ordering */
        goto fn_deferred;
    }

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = data_sz;

    if (need_packing) {
        MPIR_Assert(data_sz <= MPIDI_UCX_DEFAULT_SHORT_SEND_SIZE);
        MPIDU_genq_private_pool_alloc_cell(MPIDI_UCX_global.pack_buf_pool, (void **) &send_buf);
        if (send_buf == NULL) {
            if (!issue_deferred) {
                goto fn_deferred;
            } else {
                goto fn_exit;
            }
        }
        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);
        mpi_errno = MPIR_Typerep_pack(data, count, datatype, 0,
                                      send_buf + am_hdr_sz + sizeof(ucx_hdr), data_sz, &last);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(last == data_sz);
    } else {
        send_buf = MPL_malloc(data_sz + am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_OTHER);
        if (send_buf == NULL) {
            mpi_errno = MPI_ERR_OTHER;
            goto fn_fail;
        }
        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);
        MPIDI_Datatype_check_lb(datatype, dt_true_lb);
        MPIR_Memcpy(send_buf + am_hdr_sz + sizeof(ucx_hdr), data + dt_true_lb, data_sz);
    }

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank, 0, 0);

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_am_send_nb(ep, MPIDI_UCX_AM_HANDLER_ID__SHORT,
                                                             send_buf, data_sz + am_hdr_sz +
                                                             sizeof(ucx_hdr), ucp_dt_make_contig(1),
                                                             &MPIDI_UCX_am_isend_short_callback, 0);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        /* send is done. free all resources and complete the request */
        if (need_packing) {
            MPIDU_genq_private_pool_free_cell(MPIDI_UCX_global.pack_buf_pool, (void *) send_buf);
        } else {
            MPL_free(send_buf);
        }
        MPIDIG_global.origin_cbs[handler_id] (sreq);
    } else {
        /* set the ch4r request inside the UCP request */
        MPIDI_UCX_AMREQUEST(sreq, pack_buffer) = send_buf;
        MPIDI_UCX_AMREQUEST(sreq, handler_id) = handler_id;
        MPIDI_UCX_AMREQUEST(sreq, is_gpu_pack_buffer) = need_packing;
        ucp_request->req = sreq;
        ucp_request_release(ucp_request);
    }

    if (issue_deferred) {
        DL_DELETE(MPIDI_UCX_global.deferred_am_isend_q, MPIDI_UCX_AMREQUEST(sreq, deferred_req));
        MPL_free(MPIDI_UCX_AMREQUEST(sreq, deferred_req));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_DO_AM_ISEND_SHORT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_deferred:
    MPIDI_UCX_AMREQUEST(sreq, deferred_req) =
        (MPIDI_UCX_deferred_am_isend_req_t *) MPL_malloc(sizeof(MPIDI_UCX_deferred_am_isend_req_t),
                                                         MPL_MEM_OTHER);
    MPIR_Assert(MPIDI_UCX_AMREQUEST(sreq, deferred_req));
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->rank = rank;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->comm = comm;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->handler_id = handler_id;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->buf = data;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->count = count;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->datatype = datatype;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->sreq = sreq;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->data_sz = data_sz;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->need_packing = need_packing;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->ucx_handler_id = MPIDI_UCX_AM_HANDLER_ID__SHORT;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->am_hdr_sz = am_hdr_sz;
    MPIR_Memcpy(MPIDI_UCX_AMREQUEST(sreq, deferred_req)->am_hdr, am_hdr, am_hdr_sz);
    DL_APPEND(MPIDI_UCX_global.deferred_am_isend_q, MPIDI_UCX_AMREQUEST(sreq, deferred_req));
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_do_am_isend_bulk(int rank, MPIR_Comm * comm, int handler_id,
                                                        const void *am_hdr, MPI_Aint am_hdr_sz,
                                                        const void *data, MPI_Aint count,
                                                        MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    char *send_buf;
    size_t data_sz;
    MPIDI_UCX_am_header_t ucx_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_DO_AM_ISEND_BULK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_DO_AM_ISEND_BULK);

    MPIDI_Datatype_check_size(datatype, count, data_sz);

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank, 0, 0);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = data_sz;

    MPIR_gpu_malloc_host((void **) &send_buf, data_sz + am_hdr_sz + sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

    MPI_Aint actual_pack_bytes;
    mpi_errno =
        MPIR_Typerep_pack(data, count, datatype, 0, send_buf + am_hdr_sz + sizeof(ucx_hdr),
                          data_sz, &actual_pack_bytes);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(actual_pack_bytes == data_sz);

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_am_send_nb(ep, MPIDI_UCX_AM_HANDLER_ID__BULK,
                                                             send_buf, data_sz + am_hdr_sz +
                                                             sizeof(ucx_hdr), ucp_dt_make_contig(1),
                                                             &MPIDI_UCX_am_isend_bulk_callback, 0);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    /* send is done. free all resources and complete the request */
    if (ucp_request == NULL) {
        MPIR_gpu_free_host(send_buf);
        MPIDIG_global.origin_cbs[handler_id] (sreq);
        goto fn_exit;
    }

    /* set the ch4r request inside the UCP request */
    sreq->dev.ch4.am.netmod_am.ucx.pack_buffer = send_buf;
    sreq->dev.ch4.am.netmod_am.ucx.handler_id = handler_id;
    ucp_request->req = sreq;
    ucp_request_release(ucp_request);


  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_DO_AM_ISEND_BULK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* ifndef UCX_AM_IMPL_H_INCLUDED */
