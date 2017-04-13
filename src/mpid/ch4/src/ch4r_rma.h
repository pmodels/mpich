/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4R_RMA_H_INCLUDED
#define CH4R_RMA_H_INCLUDED

#include "ch4_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_do_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_do_put(const void *origin_addr,
                               int origin_count,
                               MPI_Datatype origin_datatype,
                               int target_rank,
                               MPI_Aint target_disp,
                               int target_count,
                               MPI_Datatype target_datatype, MPIR_Win * win,
                               MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS, n_iov, c;
    MPIR_Request *sreq = NULL;
    MPIDI_CH4U_put_msg_t am_hdr;
    uint64_t offset;
    size_t data_sz;
    MPI_Aint last, num_iov;
    MPID_Segment *segment_ptr;
    struct iovec *dt_iov, am_iov[2];

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_DO_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_DO_PUT);

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);

    MPIDI_Datatype_check_size(origin_datatype, origin_count, data_sz);
    if (data_sz == 0 || target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    if (target_rank == win->comm_ptr->rank) {
        offset = win->disp_unit * target_disp;
        mpi_errno = MPIR_Localcopy(origin_addr,
                                   origin_count,
                                   origin_datatype,
                                   (char *) win->base + offset, target_count, target_datatype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* Only create request when issuing is not completed.
     * We initialize two ref_count for progress engine and request-based OP,
     * then put needs to free the second ref_count.*/
    sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__RMA, 2);
    MPIR_Assert(sreq);
    MPIDI_CH4U_REQUEST(sreq, req->preq.win_ptr) = win;

    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);
    MPIR_cc_incr(sreq->cc_ptr, &c);
    am_hdr.src_rank = win->comm_ptr->rank;
    am_hdr.target_disp = target_disp;
    am_hdr.count = target_count;
    am_hdr.datatype = target_datatype;
    am_hdr.preq_ptr = (uint64_t) sreq;
    am_hdr.win_id = MPIDI_CH4U_WIN(win, win_id);

    /* Increase local and remote completion counters and set the local completion
     * counter in request, thus it can be decreased at request completion. */
    MPIDI_win_cmpl_cnts_incr(win, target_rank, &sreq->completion_notification);
    MPIDI_CH4U_REQUEST(sreq, rank) = target_rank;

    if (HANDLE_GET_KIND(target_datatype) == HANDLE_KIND_BUILTIN) {
        am_hdr.n_iov = 0;
        MPIDI_CH4U_REQUEST(sreq, req->preq.dt_iov) = NULL;

        /* FIXIME: we need to choose between NM and SHM */
        mpi_errno = MPIDI_NM_am_isend(target_rank, win->comm_ptr, MPIDI_CH4U_PUT_REQ,
                                      &am_hdr, sizeof(am_hdr), origin_addr,
                                      origin_count, origin_datatype, sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    segment_ptr = MPIDU_Segment_alloc();
    MPIR_Assert(segment_ptr);

    MPIDU_Segment_init(NULL, target_count, target_datatype, segment_ptr, 0);
    last = data_sz;
    MPIDU_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
    n_iov = (int) num_iov;
    MPIR_Assert(n_iov > 0);
    am_hdr.n_iov = n_iov;
    dt_iov = (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec));
    MPIR_Assert(dt_iov);

    last = data_sz;
    MPIDU_Segment_pack_vector(segment_ptr, 0, &last, dt_iov, &n_iov);
    MPIR_Assert(last == (MPI_Aint) data_sz);
    MPL_free(segment_ptr);

    am_iov[0].iov_base = &am_hdr;
    am_iov[0].iov_len = sizeof(am_hdr);
    am_iov[1].iov_base = dt_iov;
    am_iov[1].iov_len = sizeof(struct iovec) * am_hdr.n_iov;

    MPIDI_CH4U_REQUEST(sreq, req->preq.dt_iov) = dt_iov;

    /* FIXIME: MPIDI_NM_am_hdr_max_sz should be removed */
    if ((am_iov[0].iov_len + am_iov[1].iov_len) <= MPIDI_NM_am_hdr_max_sz()) {
        /* FIXIME: we need to choose between NM and SHM */
        mpi_errno = MPIDI_NM_am_isendv(target_rank, win->comm_ptr, MPIDI_CH4U_PUT_REQ,
                                       &am_iov[0], 2, origin_addr, origin_count, origin_datatype,
                                       sreq);
    }
    else {
        MPIDI_CH4U_REQUEST(sreq, req->preq.origin_addr) = (void *) origin_addr;
        MPIDI_CH4U_REQUEST(sreq, req->preq.origin_count) = origin_count;
        MPIDI_CH4U_REQUEST(sreq, req->preq.origin_datatype) = origin_datatype;
        dtype_add_ref_if_not_builtin(origin_datatype);

        /* FIXIME: we need to choose between NM and SHM */
        mpi_errno = MPIDI_NM_am_isend(target_rank, win->comm_ptr, MPIDI_CH4U_PUT_IOV_REQ,
                                      &am_hdr, sizeof(am_hdr), am_iov[1].iov_base,
                                      am_iov[1].iov_len, MPI_BYTE, sreq);
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    *sreq_ptr = sreq;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_DO_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_do_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_do_get(void *origin_addr,
                               int origin_count,
                               MPI_Datatype origin_datatype,
                               int target_rank,
                               MPI_Aint target_disp,
                               int target_count,
                               MPI_Datatype target_datatype, MPIR_Win * win,
                               MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS, n_iov, c;
    size_t offset;
    MPIR_Request *sreq = NULL;
    MPIDI_CH4U_get_req_msg_t am_hdr;
    size_t data_sz;
    MPI_Aint last, num_iov;
    MPID_Segment *segment_ptr;
    struct iovec *dt_iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_DO_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_DO_GET);

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);

    MPIDI_Datatype_check_size(origin_datatype, origin_count, data_sz);
    if (data_sz == 0 || target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    if (target_rank == win->comm_ptr->rank) {
        offset = win->disp_unit * target_disp;
        mpi_errno = MPIR_Localcopy((char *) win->base + offset,
                                   target_count,
                                   target_datatype, origin_addr, origin_count, origin_datatype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* Only create request when issuing is not completed.
     * We initialize two ref_count for progress engine and request-based OP,
     * then get needs to free the second ref_count.*/
    sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__RMA, 2);
    MPIR_Assert(sreq);

    MPIDI_CH4U_REQUEST(sreq, req->greq.win_ptr) = win;
    MPIDI_CH4U_REQUEST(sreq, req->greq.addr) = (uint64_t) ((char *) origin_addr);
    MPIDI_CH4U_REQUEST(sreq, req->greq.count) = origin_count;
    MPIDI_CH4U_REQUEST(sreq, req->greq.datatype) = origin_datatype;
    MPIDI_CH4U_REQUEST(sreq, rank) = target_rank;

    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);
    MPIR_cc_incr(sreq->cc_ptr, &c);
    am_hdr.target_disp = target_disp;
    am_hdr.count = target_count;
    am_hdr.datatype = target_datatype;
    am_hdr.greq_ptr = (uint64_t) sreq;
    am_hdr.win_id = MPIDI_CH4U_WIN(win, win_id);
    am_hdr.src_rank = win->comm_ptr->rank;

    /* Increase local and remote completion counters and set the local completion
     * counter in request, thus it can be decreased at request completion. */
    MPIDI_win_cmpl_cnts_incr(win, target_rank, &sreq->completion_notification);

    if (HANDLE_GET_KIND(target_datatype) == HANDLE_KIND_BUILTIN) {
        am_hdr.n_iov = 0;
        MPIDI_CH4U_REQUEST(sreq, req->greq.dt_iov) = NULL;

        /* FIXIME: we need to choose between NM and SHM */
        mpi_errno = MPIDI_NM_am_isend(target_rank, win->comm_ptr,
                                      MPIDI_CH4U_GET_REQ, &am_hdr, sizeof(am_hdr),
                                      NULL, 0, MPI_DATATYPE_NULL, sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    segment_ptr = MPIDU_Segment_alloc();
    MPIR_Assert(segment_ptr);

    MPIDU_Segment_init(NULL, target_count, target_datatype, segment_ptr, 0);
    last = data_sz;
    MPIDU_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
    n_iov = (int) num_iov;
    MPIR_Assert(n_iov > 0);
    am_hdr.n_iov = n_iov;
    dt_iov = (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec));
    MPIR_Assert(dt_iov);

    last = data_sz;
    MPIDU_Segment_pack_vector(segment_ptr, 0, &last, dt_iov, &n_iov);
    MPIR_Assert(last == (MPI_Aint) data_sz);
    MPL_free(segment_ptr);

    MPIDI_CH4U_REQUEST(sreq, req->greq.dt_iov) = dt_iov;
    /* FIXIME: we need to choose between NM and SHM */
    mpi_errno = MPIDI_NM_am_isend(target_rank, win->comm_ptr, MPIDI_CH4U_GET_REQ,
                                  &am_hdr, sizeof(am_hdr), dt_iov,
                                  sizeof(struct iovec) * am_hdr.n_iov, MPI_BYTE, sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    *sreq_ptr = sreq;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_DO_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_do_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_do_accumulate(const void *origin_addr,
                                                 int origin_count,
                                                 MPI_Datatype origin_datatype,
                                                 int target_rank,
                                                 MPI_Aint target_disp,
                                                 int target_count,
                                                 MPI_Datatype target_datatype,
                                                 MPI_Op op, MPIR_Win * win,
                                                 MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS, c, n_iov;
    MPIR_Request *sreq = NULL;
    size_t basic_type_size;
    MPIDI_CH4U_acc_req_msg_t am_hdr;
    uint64_t data_sz, target_data_sz;
    MPI_Aint last, num_iov;
    MPID_Segment *segment_ptr;
    struct iovec *dt_iov, am_iov[2];
    MPIR_Datatype *dt_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_DO_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_DO_ACCUMULATE);

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);

    MPIDI_Datatype_get_size_dt_ptr(origin_count, origin_datatype, data_sz, dt_ptr);
    MPIDI_Datatype_check_size(target_datatype, target_count, target_data_sz);

    if (data_sz == 0 || target_rank == MPI_PROC_NULL || target_data_sz == 0) {
        goto fn_exit;
    }

    /* Only create request when issuing is not completed.
     * We initialize two ref_count for progress engine and request-based OP,
     * then acc needs to free the second ref_count.*/
    sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__RMA, 2);
    MPIR_Assert(sreq);
    MPIDI_CH4U_REQUEST(sreq, req->areq.win_ptr) = win;

    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);
    MPIR_cc_incr(sreq->cc_ptr, &c);

    am_hdr.req_ptr = (uint64_t) sreq;
    am_hdr.origin_count = origin_count;

    if (HANDLE_GET_KIND(origin_datatype) == HANDLE_KIND_BUILTIN) {
        am_hdr.origin_datatype = origin_datatype;
    }
    else {
        am_hdr.origin_datatype = (dt_ptr) ? dt_ptr->basic_type : MPI_DATATYPE_NULL;
        MPID_Datatype_get_size_macro(am_hdr.origin_datatype, basic_type_size);
        am_hdr.origin_count = (basic_type_size > 0) ? data_sz / basic_type_size : 0;
    }

    am_hdr.target_count = target_count;
    am_hdr.target_datatype = target_datatype;
    am_hdr.target_disp = target_disp;
    am_hdr.op = op;
    am_hdr.win_id = MPIDI_CH4U_WIN(win, win_id);
    am_hdr.src_rank = win->comm_ptr->rank;

    /* Increase local and remote completion counters and set the local completion
     * counter in request, thus it can be decreased at request completion. */
    MPIDI_win_cmpl_cnts_incr(win, target_rank, &sreq->completion_notification);

    MPIDI_CH4U_REQUEST(sreq, rank) = target_rank;
    MPIDI_CH4U_REQUEST(sreq, req->areq.data_sz) = data_sz;
    if (HANDLE_GET_KIND(target_datatype) == HANDLE_KIND_BUILTIN) {
        am_hdr.n_iov = 0;
        MPIDI_CH4U_REQUEST(sreq, req->areq.dt_iov) = NULL;

        /* FIXIME: we need to choose between NM and SHM */
        mpi_errno = MPIDI_NM_am_isend(target_rank, win->comm_ptr, MPIDI_CH4U_ACC_REQ,
                                      &am_hdr, sizeof(am_hdr), origin_addr,
                                      (op == MPI_NO_OP) ? 0 : origin_count,
                                      origin_datatype, sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    MPIDI_Datatype_get_size_dt_ptr(target_count, target_datatype, data_sz, dt_ptr);
    am_hdr.target_datatype = dt_ptr->basic_type;
    am_hdr.target_count = dt_ptr->n_builtin_elements;

    segment_ptr = MPIDU_Segment_alloc();
    MPIR_Assert(segment_ptr);


    MPIDU_Segment_init(NULL, target_count, target_datatype, segment_ptr, 0);
    last = data_sz;
    MPIDU_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
    n_iov = (int) num_iov;
    MPIR_Assert(n_iov > 0);
    am_hdr.n_iov = n_iov;
    dt_iov = (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec));
    MPIR_Assert(dt_iov);

    last = data_sz;
    MPIDU_Segment_pack_vector(segment_ptr, 0, &last, dt_iov, &n_iov);
    MPIR_Assert(last == (MPI_Aint) data_sz);
    MPL_free(segment_ptr);

    am_iov[0].iov_base = &am_hdr;
    am_iov[0].iov_len = sizeof(am_hdr);
    am_iov[1].iov_base = dt_iov;
    am_iov[1].iov_len = sizeof(struct iovec) * am_hdr.n_iov;
    MPIDI_CH4U_REQUEST(sreq, req->areq.dt_iov) = dt_iov;

    /* FIXIME: MPIDI_NM_am_hdr_max_sz should be removed */
    if ((am_iov[0].iov_len + am_iov[1].iov_len) <= MPIDI_NM_am_hdr_max_sz()) {
        /* FIXIME: we need to choose between NM and SHM */
        mpi_errno = MPIDI_NM_am_isendv(target_rank, win->comm_ptr, MPIDI_CH4U_ACC_REQ,
                                       &am_iov[0], 2, origin_addr,
                                       (op == MPI_NO_OP) ? 0 : origin_count,
                                       origin_datatype, sreq);
    }
    else {
        MPIDI_CH4U_REQUEST(sreq, req->areq.origin_addr) = (void *) origin_addr;
        MPIDI_CH4U_REQUEST(sreq, req->areq.origin_count) = origin_count;
        MPIDI_CH4U_REQUEST(sreq, req->areq.origin_datatype) = origin_datatype;
        dtype_add_ref_if_not_builtin(origin_datatype);

        /* FIXIME: we need to choose between NM and SHM */
        mpi_errno = MPIDI_NM_am_isend(target_rank, win->comm_ptr, MPIDI_CH4U_ACC_IOV_REQ,
                                      &am_hdr, sizeof(am_hdr), am_iov[1].iov_base,
                                      am_iov[1].iov_len, MPI_BYTE, sreq);
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    *sreq_ptr = sreq;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_DO_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_do_get_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_do_get_accumulate(const void *origin_addr,
                                                     int origin_count,
                                                     MPI_Datatype origin_datatype,
                                                     void *result_addr,
                                                     int result_count,
                                                     MPI_Datatype result_datatype,
                                                     int target_rank,
                                                     MPI_Aint target_disp,
                                                     int target_count,
                                                     MPI_Datatype target_datatype,
                                                     MPI_Op op, MPIR_Win * win,
                                                     MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS, c, n_iov;
    MPIR_Request *sreq = NULL;
    size_t basic_type_size;
    MPIDI_CH4U_get_acc_req_msg_t am_hdr;
    uint64_t data_sz, result_data_sz, target_data_sz;
    MPI_Aint last, num_iov;
    MPID_Segment *segment_ptr;
    struct iovec *dt_iov, am_iov[2];
    MPIR_Datatype *dt_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_DO_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_DO_GET_ACCUMULATE);

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);

    MPIDI_Datatype_get_size_dt_ptr(origin_count, origin_datatype, data_sz, dt_ptr);
    MPIDI_Datatype_check_size(target_datatype, target_count, target_data_sz);
    MPIDI_Datatype_check_size(result_datatype, result_count, result_data_sz);

    if (target_rank == MPI_PROC_NULL || target_data_sz == 0 ||
        (data_sz == 0 && result_data_sz == 0)) {
        goto fn_exit;
    }

    /* Only create request when issuing is not completed.
     * We initialize two ref_count for progress engine and request-based OP,
     * then get_acc needs to free the second ref_count.*/
    sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__RMA, 2);
    MPIR_Assert(sreq);

    MPIDI_CH4U_REQUEST(sreq, req->areq.win_ptr) = win;
    MPIDI_CH4U_REQUEST(sreq, req->areq.result_addr) = result_addr;
    MPIDI_CH4U_REQUEST(sreq, req->areq.result_count) = result_count;
    MPIDI_CH4U_REQUEST(sreq, req->areq.result_datatype) = result_datatype;
    dtype_add_ref_if_not_builtin(result_datatype);

    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);
    MPIR_cc_incr(sreq->cc_ptr, &c);

    /* TODO: have common routine for accumulate/get_accumulate */
    am_hdr.req_ptr = (uint64_t) sreq;
    am_hdr.origin_count = origin_count;

    if (HANDLE_GET_KIND(origin_datatype) == HANDLE_KIND_BUILTIN) {
        am_hdr.origin_datatype = origin_datatype;
    }
    else {
        am_hdr.origin_datatype = (dt_ptr) ? dt_ptr->basic_type : MPI_DATATYPE_NULL;
        MPID_Datatype_get_size_macro(am_hdr.origin_datatype, basic_type_size);
        am_hdr.origin_count = (basic_type_size > 0) ? data_sz / basic_type_size : 0;
    }

    am_hdr.target_count = target_count;
    am_hdr.target_datatype = target_datatype;
    am_hdr.target_disp = target_disp;
    am_hdr.op = op;
    am_hdr.win_id = MPIDI_CH4U_WIN(win, win_id);
    am_hdr.src_rank = win->comm_ptr->rank;

    am_hdr.result_data_sz = result_data_sz;

    /* Increase local and remote completion counters and set the local completion
     * counter in request, thus it can be decreased at request completion. */
    MPIDI_win_cmpl_cnts_incr(win, target_rank, &sreq->completion_notification);

    MPIDI_CH4U_REQUEST(sreq, rank) = target_rank;
    MPIDI_CH4U_REQUEST(sreq, req->areq.data_sz) = data_sz;
    if (HANDLE_GET_KIND(target_datatype) == HANDLE_KIND_BUILTIN) {
        am_hdr.n_iov = 0;
        MPIDI_CH4U_REQUEST(sreq, req->areq.dt_iov) = NULL;

        /* FIXIME: we need to choose between NM and SHM */
        mpi_errno = MPIDI_NM_am_isend(target_rank, win->comm_ptr, MPIDI_CH4U_GET_ACC_REQ,
                                      &am_hdr, sizeof(am_hdr), origin_addr,
                                      (op == MPI_NO_OP) ? 0 : origin_count,
                                      origin_datatype, sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    MPIDI_Datatype_get_size_dt_ptr(target_count, target_datatype, data_sz, dt_ptr);
    am_hdr.target_datatype = dt_ptr->basic_type;
    am_hdr.target_count = dt_ptr->n_builtin_elements;

    segment_ptr = MPIDU_Segment_alloc();
    MPIR_Assert(segment_ptr);


    MPIDU_Segment_init(NULL, target_count, target_datatype, segment_ptr, 0);
    last = data_sz;
    MPIDU_Segment_count_contig_blocks(segment_ptr, 0, &last, &num_iov);
    n_iov = (int) num_iov;
    MPIR_Assert(n_iov > 0);
    am_hdr.n_iov = n_iov;
    dt_iov = (struct iovec *) MPL_malloc(n_iov * sizeof(struct iovec));
    MPIR_Assert(dt_iov);

    last = data_sz;
    MPIDU_Segment_pack_vector(segment_ptr, 0, &last, dt_iov, &n_iov);
    MPIR_Assert(last == (MPI_Aint) data_sz);
    MPL_free(segment_ptr);

    am_iov[0].iov_base = &am_hdr;
    am_iov[0].iov_len = sizeof(am_hdr);
    am_iov[1].iov_base = dt_iov;
    am_iov[1].iov_len = sizeof(struct iovec) * am_hdr.n_iov;
    MPIDI_CH4U_REQUEST(sreq, req->areq.dt_iov) = dt_iov;

    /* FIXIME: MPIDI_NM_am_hdr_max_sz should be removed */
    if ((am_iov[0].iov_len + am_iov[1].iov_len) <= MPIDI_NM_am_hdr_max_sz()) {
        /* FIXIME: we need to choose between NM and SHM */
        mpi_errno = MPIDI_NM_am_isendv(target_rank, win->comm_ptr, MPIDI_CH4U_GET_ACC_REQ,
                                       &am_iov[0], 2, origin_addr,
                                       (op == MPI_NO_OP) ? 0 : origin_count,
                                       origin_datatype, sreq);
    }
    else {
        MPIDI_CH4U_REQUEST(sreq, req->areq.origin_addr) = (void *) origin_addr;
        MPIDI_CH4U_REQUEST(sreq, req->areq.origin_count) = origin_count;
        MPIDI_CH4U_REQUEST(sreq, req->areq.origin_datatype) = origin_datatype;
        dtype_add_ref_if_not_builtin(origin_datatype);

        /* FIXIME: we need to choose between NM and SHM */
        mpi_errno = MPIDI_NM_am_isend(target_rank, win->comm_ptr, MPIDI_CH4U_GET_ACC_IOV_REQ,
                                      &am_hdr, sizeof(am_hdr), am_iov[1].iov_base,
                                      am_iov[1].iov_len, MPI_BYTE, sreq);
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    *sreq_ptr = sreq;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_DO_GET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_put(const void *origin_addr,
                                                int origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                int target_count, MPI_Datatype target_datatype,
                                                MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_PUT);

    mpi_errno = MPIDI_do_put(origin_addr, origin_count, origin_datatype,
                             target_rank, target_disp, target_count, target_datatype, win, &sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    /* always release my ref, only request-ops track. */
    if (sreq != NULL)
        MPIR_Request_free(sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_rput
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_rput(const void *origin_addr,
                                                 int origin_count,
                                                 MPI_Datatype origin_datatype,
                                                 int target_rank,
                                                 MPI_Aint target_disp,
                                                 int target_count,
                                                 MPI_Datatype target_datatype,
                                                 MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_RPUT);

    mpi_errno = MPIDI_do_put(origin_addr, origin_count, origin_datatype,
                             target_rank, target_disp, target_count, target_datatype, win, &sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (sreq == NULL) {
        /* create a completed request for user if issuing is completed immediately. */
        sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Assert(sreq);

        MPIR_Request_add_ref(sreq);
        MPID_Request_complete(sreq);
    }

  fn_exit:
    *request = sreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_RPUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_get(void *origin_addr,
                                                int origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                int target_count, MPI_Datatype target_datatype,
                                                MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_GET);

    mpi_errno = MPIDI_do_get(origin_addr, origin_count, origin_datatype,
                             target_rank, target_disp, target_count, target_datatype, win, &sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    /* always release my ref, only request-ops track. */
    if (sreq != NULL)
        MPIR_Request_free(sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_rget
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_rget(void *origin_addr,
                                                 int origin_count,
                                                 MPI_Datatype origin_datatype,
                                                 int target_rank,
                                                 MPI_Aint target_disp,
                                                 int target_count,
                                                 MPI_Datatype target_datatype,
                                                 MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_RGET);

    mpi_errno = MPIDI_do_get(origin_addr, origin_count, origin_datatype,
                             target_rank, target_disp, target_count, target_datatype, win, &sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (sreq == NULL) {
        /* create a completed request for user if issuing is completed immediately. */
        sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Assert(sreq);

        MPIR_Request_add_ref(sreq);
        MPID_Request_complete(sreq);
    }

  fn_exit:
    *request = sreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_RGET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_raccumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_raccumulate(const void *origin_addr,
                                                        int origin_count,
                                                        MPI_Datatype origin_datatype,
                                                        int target_rank,
                                                        MPI_Aint target_disp,
                                                        int target_count,
                                                        MPI_Datatype target_datatype,
                                                        MPI_Op op, MPIR_Win * win,
                                                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_RACCUMULATE);

    mpi_errno = MPIDI_do_accumulate(origin_addr, origin_count, origin_datatype,
                                    target_rank, target_disp, target_count, target_datatype, op,
                                    win, &sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (sreq == NULL) {
        /* create a completed request for user if issuing is completed immediately. */
        sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Assert(sreq);

        MPIR_Request_add_ref(sreq);
        MPID_Request_complete(sreq);
    }

  fn_exit:
    *request = sreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_RACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_accumulate(const void *origin_addr,
                                                       int origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       int target_rank,
                                                       MPI_Aint target_disp,
                                                       int target_count,
                                                       MPI_Datatype target_datatype, MPI_Op op,
                                                       MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_ACCUMULATE);

    mpi_errno = MPIDI_do_accumulate(origin_addr, origin_count, origin_datatype,
                                    target_rank, target_disp, target_count, target_datatype, op,
                                    win, &sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    /* always release my ref, only request-ops track. */
    if (sreq != NULL)
        MPIR_Request_free(sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_rget_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_rget_accumulate(const void *origin_addr,
                                                            int origin_count,
                                                            MPI_Datatype origin_datatype,
                                                            void *result_addr,
                                                            int result_count,
                                                            MPI_Datatype result_datatype,
                                                            int target_rank,
                                                            MPI_Aint target_disp,
                                                            int target_count,
                                                            MPI_Datatype target_datatype,
                                                            MPI_Op op, MPIR_Win * win,
                                                            MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_RGET_ACCUMULATE);

    mpi_errno = MPIDI_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                        result_addr, result_count, result_datatype,
                                        target_rank, target_disp, target_count, target_datatype, op,
                                        win, &sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (sreq == NULL) {
        /* create a completed request for user if issuing is completed immediately. */
        sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_Assert(sreq);

        MPIR_Request_add_ref(sreq);
        MPID_Request_complete(sreq);
    }

  fn_exit:
    *request = sreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_RGET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_get_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_get_accumulate(const void *origin_addr,
                                                           int origin_count,
                                                           MPI_Datatype origin_datatype,
                                                           void *result_addr,
                                                           int result_count,
                                                           MPI_Datatype result_datatype,
                                                           int target_rank,
                                                           MPI_Aint target_disp,
                                                           int target_count,
                                                           MPI_Datatype target_datatype,
                                                           MPI_Op op, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_GET_ACCUMULATE);

    mpi_errno = MPIDI_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                        result_addr, result_count, result_datatype,
                                        target_rank, target_disp, target_count, target_datatype, op,
                                        win, &sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    /* always release my ref, only request-ops track. */
    if (sreq != NULL)
        MPIR_Request_free(sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_GET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_compare_and_swap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_compare_and_swap(const void *origin_addr,
                                                             const void *compare_addr,
                                                             void *result_addr,
                                                             MPI_Datatype datatype,
                                                             int target_rank,
                                                             MPI_Aint target_disp, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIR_Request *sreq = NULL;
    MPIDI_CH4U_cswap_req_msg_t am_hdr;
    size_t data_sz;
    void *p_data;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_COMPARE_AND_SWAP);

    MPIDI_CH4U_EPOCH_CHECK_SYNC(win, mpi_errno, goto fn_fail);

    MPIDI_Datatype_check_size(datatype, 1, data_sz);
    if (data_sz == 0 || target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    p_data = MPL_malloc(data_sz * 2);
    MPIR_Assert(p_data);
    MPIR_Memcpy(p_data, (char *) origin_addr, data_sz);
    MPIR_Memcpy((char *) p_data + data_sz, (char *) compare_addr, data_sz);

    sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__RMA, 1);
    MPIR_Assert(sreq);

    MPIDI_CH4U_REQUEST(sreq, req->creq.win_ptr) = win;
    MPIDI_CH4U_REQUEST(sreq, req->creq.addr) = (uint64_t) ((char *) result_addr);
    MPIDI_CH4U_REQUEST(sreq, req->creq.datatype) = datatype;
    MPIDI_CH4U_REQUEST(sreq, req->creq.result_addr) = result_addr;
    MPIDI_CH4U_REQUEST(sreq, req->creq.data) = p_data;
    MPIDI_CH4U_REQUEST(sreq, rank) = target_rank;

    MPIDI_CH4U_EPOCH_START_CHECK(win, mpi_errno, goto fn_fail);
    MPIR_cc_incr(sreq->cc_ptr, &c);

    am_hdr.target_disp = target_disp;
    am_hdr.datatype = datatype;
    am_hdr.req_ptr = (uint64_t) sreq;
    am_hdr.win_id = MPIDI_CH4U_WIN(win, win_id);
    am_hdr.src_rank = win->comm_ptr->rank;

    MPIDI_win_cmpl_cnts_incr(win, target_rank, &sreq->completion_notification);

    /* FIXIME: we need to choose between NM and SHM */
    mpi_errno = MPIDI_NM_am_isend(target_rank, win->comm_ptr, MPIDI_CH4U_CSWAP_REQ,
                                  &am_hdr, sizeof(am_hdr), (char *) p_data, 2, datatype, sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_COMPARE_AND_SWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_fetch_and_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_fetch_and_op(const void *origin_addr,
                                                         void *result_addr,
                                                         MPI_Datatype datatype,
                                                         int target_rank,
                                                         MPI_Aint target_disp, MPI_Op op,
                                                         MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_FETCH_AND_OP);

    mpi_errno = MPIDI_CH4U_mpi_get_accumulate(origin_addr, 1, datatype,
                                              result_addr, 1, datatype,
                                              target_rank, target_disp, 1, datatype, op, win);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_FETCH_AND_OP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4R_RMA_H_INCLUDED */
