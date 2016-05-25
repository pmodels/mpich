/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef PTL_AM_H_INCLUDED
#define PTL_AM_H_INCLUDED

#include "ptl_impl.h"

static inline int MPIDI_NM_am_isend(int rank,
                                    MPIR_Comm * comm,
                                    int handler_id,
                                    const void *am_hdr,
                                    size_t am_hdr_sz,
                                    const void *data,
                                    MPI_Count count,
                                    MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS, ret, c;
    size_t data_sz;
    MPI_Aint dt_true_lb, last;
    MPIR_Datatype *dt_ptr;
    int dt_contig;
    ptl_hdr_data_t ptl_hdr;
    ptl_match_bits_t match_bits;
    char *send_buf = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_SEND_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_SEND_AM);

    match_bits = MPIDI_PTL_init_tag(comm->context_id, MPIDI_PTL_AM_TAG);
    sreq->dev.ch4.am.netmod_am.portals4.handler_id = handler_id;

    MPIR_cc_incr(sreq->cc_ptr, &c);

    /* fast path: there's no data to be sent */
    if (count == 0) {
        send_buf = MPL_malloc(am_hdr_sz);
        MPIR_Memcpy(send_buf, am_hdr, am_hdr_sz);
        sreq->dev.ch4.am.netmod_am.portals4.pack_buffer = send_buf;

        ptl_hdr = MPIDI_PTL_init_am_hdr(handler_id, 0);

        ret = PtlPut(MPIDI_PTL_global.md, (ptl_size_t) send_buf, am_hdr_sz,
                     PTL_ACK_REQ, MPIDI_PTL_global.addr_table[rank].process,
                     MPIDI_PTL_global.addr_table[rank].pt, match_bits, 0, sreq, ptl_hdr);

        goto fn_exit;
    }

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    ptl_hdr = MPIDI_PTL_init_am_hdr(handler_id, data_sz);

    if (dt_contig) {
        /* create a two element iovec and send */
        ptl_md_t md;
        ptl_iovec_t iovec[2];

        send_buf = MPL_malloc(am_hdr_sz);
        MPIR_Memcpy(send_buf, am_hdr, am_hdr_sz);
        sreq->dev.ch4.am.netmod_am.portals4.pack_buffer = send_buf;

        iovec[0].iov_base = send_buf;
        iovec[0].iov_len = am_hdr_sz;
        iovec[1].iov_base = (char *) data + dt_true_lb;
        iovec[1].iov_len = data_sz;
        md.start = iovec;
        md.length = 2;
        md.options = PTL_IOVEC;
        md.eq_handle = MPIDI_PTL_global.eqs[0];
        md.ct_handle = PTL_CT_NONE;

        ret = PtlMDBind(MPIDI_PTL_global.ni, &md, &sreq->dev.ch4.am.netmod_am.portals4.md);
        ret = PtlPut(sreq->dev.ch4.am.netmod_am.portals4.md, 0, am_hdr_sz + data_sz,
                     PTL_ACK_REQ, MPIDI_PTL_global.addr_table[rank].process,
                     MPIDI_PTL_global.addr_table[rank].pt, match_bits, 0, sreq, ptl_hdr);
    }
    else {
        /* copy everything into pack_buffer */
        MPIR_Segment *segment;
        MPI_Aint last;

        send_buf = MPL_malloc(am_hdr_sz + data_sz);
        MPIR_Memcpy(send_buf, am_hdr, am_hdr_sz);
        segment = MPIR_Segment_alloc();
        MPIR_Segment_init(data, count, datatype, segment, 0);
        last = data_sz;
        MPIR_Segment_pack(segment, 0, &last, send_buf + am_hdr_sz);
        MPIR_Assert(last == data_sz);
        MPIR_Segment_free(segment);
        sreq->dev.ch4.am.netmod_am.portals4.pack_buffer = send_buf;

        ret = PtlPut(MPIDI_PTL_global.md, (ptl_size_t) send_buf, am_hdr_sz + data_sz,
                     PTL_ACK_REQ, MPIDI_PTL_global.addr_table[rank].process,
                     MPIDI_PTL_global.addr_table[rank].pt, match_bits, 0, sreq, ptl_hdr);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_SEND_AM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_am_isendv(int rank,
                                     MPIR_Comm * comm,
                                     int handler_id,
                                     struct iovec *am_hdr,
                                     size_t iov_len,
                                     const void *data,
                                     MPI_Count count,
                                     MPI_Datatype datatype,
                                     MPIR_Request * sreq)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_am_isend_reply(MPIR_Context_id_t context_id,
                                          int src_rank,
                                          int handler_id,
                                          const void *am_hdr,
                                          size_t am_hdr_sz,
                                          const void *data,
                                          MPI_Count count,
                                          MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS, ret, c;
    size_t data_sz;
    MPI_Aint dt_true_lb, last;
    MPIR_Datatype *dt_ptr;
    int dt_contig;
    ptl_hdr_data_t ptl_hdr;
    ptl_match_bits_t match_bits;
    MPIR_Comm *use_comm;
    char *send_buf = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_SEND_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_SEND_AM);

    use_comm = MPIDI_CH4U_context_id_to_comm(context_id);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    match_bits = MPIDI_PTL_init_tag(use_comm->context_id, MPIDI_PTL_AM_TAG);
    ptl_hdr = MPIDI_PTL_init_am_hdr(handler_id, data_sz);
    sreq->dev.ch4.am.netmod_am.portals4.handler_id = handler_id;

    MPIR_cc_incr(sreq->cc_ptr, &c);

    if (dt_contig) {
        /* create a two element iovec and send */
        ptl_md_t md;
        ptl_iovec_t iovec[2];

        send_buf = MPL_malloc(am_hdr_sz);
        MPIR_Memcpy(send_buf, am_hdr, am_hdr_sz);
        sreq->dev.ch4.am.netmod_am.portals4.pack_buffer = send_buf;

        iovec[0].iov_base = send_buf;
        iovec[0].iov_len = am_hdr_sz;
        iovec[1].iov_base = (char *) data + dt_true_lb;
        iovec[1].iov_len = data_sz;
        md.start = iovec;
        md.length = 2;
        md.options = PTL_IOVEC;
        md.eq_handle = MPIDI_PTL_global.eqs[0];
        md.ct_handle = PTL_CT_NONE;

        ret = PtlMDBind(MPIDI_PTL_global.ni, &md, &sreq->dev.ch4.am.netmod_am.portals4.md);
        ret = PtlPut(sreq->dev.ch4.am.netmod_am.portals4.md, 0, am_hdr_sz + data_sz,
                     PTL_ACK_REQ, MPIDI_PTL_global.addr_table[src_rank].process,
                     MPIDI_PTL_global.addr_table[src_rank].pt, match_bits, 0, sreq, ptl_hdr);
    }
    else {
        /* copy everything into pack_buffer */
        MPIR_Segment *segment;
        MPI_Aint last;

        send_buf = MPL_malloc(am_hdr_sz + data_sz);
        MPIR_Memcpy(send_buf, am_hdr, am_hdr_sz);
        segment = MPIR_Segment_alloc();
        MPIR_Segment_init(data, count, datatype, segment, 0);
        last = data_sz;
        MPIR_Segment_pack(segment, 0, &last, send_buf + am_hdr_sz);
        MPIR_Assert(last == data_sz);
        MPIR_Segment_free(segment);
        sreq->dev.ch4.am.netmod_am.portals4.pack_buffer = send_buf;

        ret = PtlPut(MPIDI_PTL_global.md, (ptl_size_t) send_buf, am_hdr_sz + data_sz,
                     PTL_ACK_REQ, MPIDI_PTL_global.addr_table[src_rank].process,
                     MPIDI_PTL_global.addr_table[src_rank].pt, match_bits, 0, sreq, ptl_hdr);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_SEND_AM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline size_t MPIDI_NM_am_hdr_max_sz(void)
{
    MPIR_Assert(0);
    return 0;
}

static inline int MPIDI_NM_am_send_hdr(int rank,
                                       MPIR_Comm * comm,
                                       int handler_id,
                                       const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS, ret, c;
    ptl_hdr_data_t ptl_hdr;
    ptl_match_bits_t match_bits;
    char *send_buf = NULL;
    MPIR_Request *inject_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_SEND_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_SEND_AM);

    ptl_hdr = MPIDI_PTL_init_am_hdr(handler_id, 0);
    match_bits = MPIDI_PTL_init_tag(comm->context_id, MPIDI_PTL_AM_TAG);

    /* create an internal request for the inject */
    inject_req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    MPIDI_NM_am_request_init(inject_req);
    send_buf = MPL_malloc(am_hdr_sz);
    MPIR_Memcpy(send_buf, am_hdr, am_hdr_sz);
    inject_req->dev.ch4.am.netmod_am.portals4.pack_buffer = send_buf;

    ret = PtlPut(MPIDI_PTL_global.md, (ptl_size_t) send_buf, am_hdr_sz,
                 PTL_ACK_REQ, MPIDI_PTL_global.addr_table[rank].process,
                 MPIDI_PTL_global.addr_table[rank].pt, match_bits, 0, inject_req, ptl_hdr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_SEND_AM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                             int src_rank,
                                             int handler_id, const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS, ret, c;
    ptl_hdr_data_t ptl_hdr;
    ptl_match_bits_t match_bits;
    MPIR_Comm *use_comm;
    char *send_buf = NULL;
    MPIR_Request *inject_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_SEND_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_SEND_AM);

    use_comm = MPIDI_CH4U_context_id_to_comm(context_id);

    ptl_hdr = MPIDI_PTL_init_am_hdr(handler_id, 0);
    match_bits = MPIDI_PTL_init_tag(use_comm->context_id, MPIDI_PTL_AM_TAG);

    /* create an internal request for the inject */
    inject_req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    MPIDI_NM_am_request_init(inject_req);
    send_buf = MPL_malloc(am_hdr_sz);
    MPIR_Memcpy(send_buf, am_hdr, am_hdr_sz);
    inject_req->dev.ch4.am.netmod_am.portals4.pack_buffer = send_buf;

    ret = PtlPut(MPIDI_PTL_global.md, (ptl_size_t) send_buf, am_hdr_sz,
                 PTL_ACK_REQ, MPIDI_PTL_global.addr_table[src_rank].process,
                 MPIDI_PTL_global.addr_table[src_rank].pt, match_bits, 0, inject_req, ptl_hdr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_SEND_AM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_am_recv(MPIR_Request * req)
{
    MPIR_Assert(0);
    return 0;
}

#endif /* PTL_AM_H_INCLUDED */
