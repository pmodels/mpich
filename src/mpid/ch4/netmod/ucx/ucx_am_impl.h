/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_AM_IMPL_H_INCLUDED
#define UCX_AM_IMPL_H_INCLUDED

#include "ucx_impl.h"
#include "mpidu_genq.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_isend_short_req_cmpl(MPIR_Request * req, int handler_id)
{
    if (MPIDI_UCX_AMREQUEST(req, is_gpu_pack_buffer)) {
        MPIDU_genq_private_pool_free_cell(MPIDI_UCX_global.pack_buf_pool,
                                          MPIDI_UCX_AMREQUEST(req, pack_buffer));
    } else {
        MPL_free(MPIDI_UCX_AMREQUEST(req, pack_buffer));
    }
    MPIDI_UCX_AMREQUEST(req, pack_buffer) = NULL;
    MPIDIG_global.origin_cbs[handler_id] (req);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_isend_short_callback(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *req = ucp_request->req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_AM_ISEND_SHORT_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_AM_ISEND_SHORT_CALLBACK);

    MPIDI_UCX_am_isend_short_req_cmpl(req, MPIDI_UCX_AMREQUEST(req, handler_id));
    ucp_request->req = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_ISEND_SHORT_CALLBACK);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_isend_pipeline_callback(void *request,
                                                                   ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIDI_UCX_am_request_t *req = (MPIDI_UCX_am_request_t *) ucp_request->req;
    int handler_id = req->handler_id;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_AM_ISEND_SHORT_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_AM_ISEND_SHORT_CALLBACK);

    MPID_Request_complete(req->sreq);

    if (req->is_gpu_pack_buffer) {
        MPIDU_genq_private_pool_free_cell(MPIDI_UCX_global.pack_buf_pool, req->pack_buffer);
    } else {
        MPL_free(req->pack_buffer);
    }
    req->pack_buffer = NULL;
    int is_done = MPIDIG_am_send_async_finish_seg(req->sreq);
    if (is_done) {
        MPIDIG_global.origin_cbs[handler_id] (req->sreq);
    }
    MPIDU_genq_private_pool_free_cell(MPIDI_UCX_global.am_hdr_buf_pool, (void *) req);
    ucp_request->req = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_ISEND_SHORT_CALLBACK);
}

MPL_STATIC_INLINE_PREFIX MPIDI_UCX_ucp_request_t
    * MPIDI_UCX_ucp_am_send_buffered(ucp_ep_h ep, const void *ucx_hdr, MPI_Aint ucx_hdr_sz,
                                     const void *am_hdr, MPI_Aint am_hdr_sz, void *data,
                                     MPI_Aint data_sz, MPIDI_UCX_am_request_t * send_req,
                                     int ucx_am_handler_id, ucp_send_callback_t cb)
{
    MPIDI_UCX_ucp_request_t *ucp_request;

    /* if the request does not have a buffer yet, create one and pack data in it.
     * if the request already have a buffer, it means the data is prepacked. */
    if (!send_req->pack_buffer) {
        send_req->pack_buffer = MPL_malloc(ucx_hdr_sz + am_hdr_sz + data_sz, MPL_MEM_OTHER);
        send_req->is_gpu_pack_buffer = false;
        MPIR_Memcpy((char *) send_req->pack_buffer + ucx_hdr_sz + am_hdr_sz, data, data_sz);
    }

    MPIR_Memcpy(send_req->pack_buffer, ucx_hdr, ucx_hdr_sz);
    if (am_hdr_sz) {
        MPIR_Memcpy((char *) send_req->pack_buffer + ucx_hdr_sz, am_hdr, am_hdr_sz);
    }

    ucp_request =
        (MPIDI_UCX_ucp_request_t *) ucp_am_send_nb(ep, ucx_am_handler_id, send_req->pack_buffer,
                                                   data_sz + am_hdr_sz + ucx_hdr_sz,
                                                   ucp_dt_make_contig(1), cb, 0);

  fn_exit:
    return ucp_request;
  fn_fail:
    goto fn_exit;
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

    /* STEP1: check buffer/DT type */
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

    /* STEP2: getting pack buffer if needed */
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
        MPIDI_UCX_AMREQUEST(sreq, pack_buffer) = send_buf;
        MPIDI_UCX_AMREQUEST(sreq, is_gpu_pack_buffer) = true;
        mpi_errno = MPIR_Typerep_pack(data, count, datatype, 0,
                                      send_buf + am_hdr_sz + sizeof(ucx_hdr), data_sz, &last);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(last == data_sz);
    } else {
        MPIDI_Datatype_check_lb(datatype, dt_true_lb);
    }

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank, 0, 0);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = data_sz;

    ucp_request = MPIDI_UCX_ucp_am_send_buffered(ep, &ucx_hdr, sizeof(ucx_hdr), am_hdr,
                                                 am_hdr_sz, (char *) data + dt_true_lb,
                                                 data_sz, &sreq->dev.ch4.am.netmod_am.ucx,
                                                 MPIDI_UCX_AM_HANDLER_ID__SHORT,
                                                 &MPIDI_UCX_am_isend_short_callback);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    /* STEP4: handles immediate send completion or preseve info for callback */
    if (ucp_request == NULL) {
        /* send is done. free all resources and complete the request */
        MPIDI_UCX_am_isend_short_req_cmpl(sreq, handler_id);
    } else {
        /* set the ch4r request inside the UCP request */
        MPIDI_UCX_AMREQUEST(sreq, handler_id) = handler_id;
        ucp_request->req = sreq;
        ucp_request_release(ucp_request);
    }

    /* STEP5: cleanup deferred req if needed */
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

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_do_am_isend_pipeline(int rank, MPIR_Comm * comm,
                                                            int handler_id, const void *am_hdr,
                                                            size_t am_hdr_sz, const void *buf,
                                                            size_t count, MPI_Datatype datatype,
                                                            MPIR_Request * sreq,
                                                            bool issue_deferred)
{
    int dt_contig, mpi_errno = MPI_SUCCESS;
    int c;
    MPIDI_UCX_am_header_t ucx_hdr;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    char *send_buf;
    MPI_Aint packed_size, data_sz;
    MPI_Aint dt_true_lb, send_size, offset;
    MPIDI_UCX_am_request_t *send_req;
    MPI_Aint local_am_hdr_sz;
    bool need_packing = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_DO_AM_ISEND_PIPELINE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_DO_AM_ISEND_PIPELINE);

    /* NOTE: issue_deferred is set to true when progress use this function for deferred operations.
     * we need to skip some code path in the scenario. Also am_hdr, am_hdr_sz and data_sz are
     * ignored when issue_deferred is set to true. They should have been saved in the request. */

    if (!issue_deferred) {
        MPIDI_Datatype_check_contig_size(datatype, count, dt_contig, data_sz);

        send_size = MPIDI_UCX_DEFAULT_SHORT_SEND_SIZE - am_hdr_sz - sizeof(MPIDI_UCX_am_header_t);
        send_size = MPL_MIN(send_size, data_sz);
        MPIDIG_am_send_async_init(sreq, datatype, data_sz);

        need_packing = dt_contig ? false : true;

        MPL_pointer_attr_t attr;
        MPIR_GPU_query_pointer_attr(buf, &attr);
        if (attr.type == MPL_GPU_POINTER_DEV) {
            /* Force packing of GPU buffer in host memory */
            need_packing = true;
        }
        offset = 0;
        local_am_hdr_sz = am_hdr_sz;
    } else {
        /* we are issuing deferred op. If the offset == 0, this is the first segment and we need to
         * send am_hdr */
        offset = MPIDIG_am_send_async_get_offset(sreq);
        local_am_hdr_sz = offset ? 0 : am_hdr_sz;
        send_size = MPIDI_UCX_DEFAULT_SHORT_SEND_SIZE - local_am_hdr_sz
            - sizeof(MPIDI_UCX_am_header_t);
        send_size = MPL_MIN(send_size, MPIDIG_am_send_async_get_data_sz_left(sreq));
        need_packing = MPIDI_UCX_AMREQUEST(sreq, deferred_req)->need_packing;
    }

    if (!issue_deferred && MPIDI_UCX_global.deferred_am_isend_q) {
        /* if the deferred queue is not empty, all new ops must be deferred to maintain ordering */
        goto fn_deferred;
    }

    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = send_size;
    ucx_hdr.src_grank = MPIR_Process.rank;

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank, 0, 0);

    if (need_packing) {
        /* FIXME: currently we always do packing, also for high density types. However,
         * we should not do packing unless needed. Also, for large low-density types
         * we should not allocate the entire buffer and do the packing at once. */
        /* TODO: (1) Skip packing for high-density datatypes; */
        MPIDU_genq_private_pool_alloc_cell(MPIDI_UCX_global.pack_buf_pool, (void **) &send_buf);
        if (send_buf == NULL) {
            if (!issue_deferred) {
                goto fn_deferred;
            } else {
                /* trying to issue deferred op but still cannot get pack buffer, just exit here */
                goto fn_exit;
            }
        }
        char *data_buf = (char *) send_buf + sizeof(ucx_hdr);
        if (local_am_hdr_sz) {
            MPIR_Memcpy(data_buf, am_hdr, local_am_hdr_sz);
            data_buf += local_am_hdr_sz;
        }
        mpi_errno = MPIR_Typerep_pack(buf, count, datatype, offset, (void *) data_buf, send_size,
                                      &packed_size);
        MPIR_ERR_CHECK(mpi_errno);
        send_size = packed_size;

        ucx_hdr.data_sz = send_size;
        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));

        mpi_errno = MPIDU_genq_private_pool_alloc_cell(MPIDI_UCX_global.am_hdr_buf_pool,
                                                       (void **) &send_req);
        MPIR_Assert(send_req);

        MPIR_cc_incr(sreq->cc_ptr, &c);
        ucp_request =
            (MPIDI_UCX_ucp_request_t *) ucp_am_send_nb(ep, MPIDI_UCX_AM_HANDLER_ID__PIPELINE,
                                                       send_buf, send_size + local_am_hdr_sz +
                                                       sizeof(ucx_hdr), ucp_dt_make_contig(1),
                                                       &MPIDI_UCX_am_isend_pipeline_callback, 0);
    } else {
        MPIDI_Datatype_check_lb(datatype, dt_true_lb);
        int hdr_size = sizeof(MPIDI_UCX_am_header_t) + local_am_hdr_sz;
        /* send_buf here is only for ucx_hdr and am_hdr */
        send_buf = MPL_malloc(hdr_size, MPL_MEM_OTHER);
        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        if (local_am_hdr_sz) {
            MPIR_Memcpy(send_buf + sizeof(MPIDI_UCX_am_header_t), am_hdr, local_am_hdr_sz);
        }

        mpi_errno = MPIDU_genq_private_pool_alloc_cell(MPIDI_UCX_global.am_hdr_buf_pool,
                                                       (void **) &send_req);
        MPIR_Assert(send_req);

        send_req->iov[0].buffer = send_buf;
        send_req->iov[0].length = hdr_size;
        send_req->iov[1].buffer = (char *) buf + dt_true_lb + offset;
        send_req->iov[1].length = send_size;

        MPIR_cc_incr(sreq->cc_ptr, &c);
        ucp_request =
            (MPIDI_UCX_ucp_request_t *) ucp_am_send_nb(ep, MPIDI_UCX_AM_HANDLER_ID__PIPELINE,
                                                       send_req->iov, 2, ucp_dt_make_iov(),
                                                       &MPIDI_UCX_am_isend_pipeline_callback, 0);
    }

    MPIDIG_am_send_async_issue_seg(sreq, send_size);
    MPIR_ERR_CHECK(mpi_errno);

    if (ucp_request == NULL) {
        /* send is done. free all resources and complete the request */
        MPID_Request_complete(sreq);
        if (need_packing) {
            MPIDU_genq_private_pool_free_cell(MPIDI_UCX_global.pack_buf_pool, (void *) send_buf);
        } else {
            MPL_free(send_buf);
        }
        MPIDU_genq_private_pool_free_cell(MPIDI_UCX_global.am_hdr_buf_pool, (void *) send_req);
        int is_done = MPIDIG_am_send_async_finish_seg(sreq);
        if (is_done) {
            if (issue_deferred) {
                DL_DELETE(MPIDI_UCX_global.deferred_am_isend_q,
                          MPIDI_UCX_AMREQUEST(sreq, deferred_req));
                MPL_free(MPIDI_UCX_AMREQUEST(sreq, deferred_req));
            }
            mpi_errno = MPIDIG_global.origin_cbs[handler_id] (sreq);
            goto fn_exit;
        }
    } else {
        /* send is not done. remember resources in send_req (not the sreq) */
        send_req->pack_buffer = send_buf;
        send_req->handler_id = handler_id;
        send_req->is_gpu_pack_buffer = need_packing;
        send_req->sreq = sreq;
        ucp_request->req = (MPIR_Request *) send_req;
        ucp_request_release(ucp_request);
    }

    /* if there IS MORE DATA to be sent and we ARE NOT called for issue deferred op, enqueue.
     * if there NO MORE DATA and we ARE called for issuing deferred op, pipeline is done, dequeue
     * skip for all other cases */
    if (!MPIDIG_am_send_async_is_done(sreq)) {
        if (!issue_deferred) {
            goto fn_deferred;
        }
    } else if (issue_deferred) {
        DL_DELETE(MPIDI_UCX_global.deferred_am_isend_q, MPIDI_UCX_AMREQUEST(sreq, deferred_req));
        MPL_free(MPIDI_UCX_AMREQUEST(sreq, deferred_req));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_DO_AM_ISEND_PIPELINE);
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
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->buf = buf;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->count = count;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->datatype = datatype;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->sreq = sreq;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->data_sz = data_sz;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->need_packing = need_packing;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->ucx_handler_id = MPIDI_UCX_AM_HANDLER_ID__PIPELINE;
    MPIDI_UCX_AMREQUEST(sreq, deferred_req)->am_hdr_sz = am_hdr_sz;
    MPIR_Memcpy(MPIDI_UCX_AMREQUEST(sreq, deferred_req)->am_hdr, am_hdr, am_hdr_sz);
    DL_APPEND(MPIDI_UCX_global.deferred_am_isend_q, MPIDI_UCX_AMREQUEST(sreq, deferred_req));
    goto fn_exit;
}

#endif /* ifndef UCX_AM_IMPL_H_INCLUDED */
