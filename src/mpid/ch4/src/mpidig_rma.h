/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_RMA_H_INCLUDED
#define MPIDIG_RMA_H_INCLUDED

#include "ch4_impl.h"

extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_amhdr_set ATTRIBUTE((unused));

/* Create a completed RMA request. Used when a request-based operation (e.g. RPUT)
 * completes immediately (=without actually issuing active messages) */
#define MPIDI_RMA_REQUEST_CREATE_COMPLETE(sreq_)                        \
    do {                                                                \
        /* create a completed request for user if issuing is completed immediately. */ \
        (sreq_) = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA); \
        MPIR_ERR_CHKANDSTMT((sreq_) == NULL, mpi_errno, MPIX_ERR_NOREQ, \
                            goto fn_fail, "**nomemreq");                \
    } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDIG_do_put(const void *origin_addr, MPI_Aint origin_count,
                                           MPI_Datatype origin_datatype, int target_rank,
                                           MPI_Aint target_disp, MPI_Aint target_count,
                                           MPI_Datatype target_datatype, MPIR_Win * win,
                                           int vci, MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIDIG_put_msg_t am_hdr;
    uint64_t offset;
    MPI_Aint origin_data_sz, target_data_sz;

    MPIR_FUNC_ENTER;

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(origin_datatype, origin_count, origin_data_sz);
    MPIDI_Datatype_check_size(target_datatype, target_count, target_data_sz);
    if (origin_data_sz == 0)
        goto immed_cmpl;

    if (target_rank == win->comm_ptr->rank) {
        offset = win->disp_unit * target_disp;
        mpi_errno = MPIR_Localcopy(origin_addr,
                                   origin_count,
                                   origin_datatype,
                                   (char *) win->base + offset, target_count, target_datatype);
        MPIR_ERR_CHECK(mpi_errno);
        goto immed_cmpl;
    }

    /* Only create request when issuing is not completed.
     * We initialize two ref_count for progress engine and request-based OP,
     * then put needs to free the second ref_count.*/
    int vci_target = MPIDI_WIN_TARGET_VCI(win, target_rank);
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 2, vci, vci_target);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    sreq->u.rma.win = win;

    MPIR_cc_inc(sreq->cc_ptr);
    MPIR_T_PVAR_TIMER_START(RMA, rma_amhdr_set);
    am_hdr.src_rank = win->comm_ptr->rank;
    am_hdr.target_disp = target_disp;
    am_hdr.preq_ptr = sreq;
    am_hdr.win_id = MPIDIG_WIN(win, win_id);
    am_hdr.origin_data_sz = origin_data_sz;

    /* Increase local and remote completion counters and set the local completion
     * counter in request, thus it can be decreased at request completion. */
    MPIDIG_win_cmpl_cnts_incr(win, target_rank, &sreq->dev.completion_notification);
    MPIDIG_REQUEST(sreq, u.origin.target_rank) = target_rank;

    int is_contig;
    MPIR_Datatype_is_contig(target_datatype, &is_contig);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype) || is_contig) {
        if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            am_hdr.target_count = target_count;
            am_hdr.target_datatype = target_datatype;
        } else {
            am_hdr.target_count = target_data_sz;
            am_hdr.target_datatype = MPI_BYTE;
        }
        am_hdr.flattened_sz = 0;
        MPIR_Datatype_get_true_lb(target_datatype, &am_hdr.target_true_lb);
        MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

        CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_PUT_REQ, &am_hdr, sizeof(am_hdr),
                          origin_addr, origin_count, origin_datatype, vci, vci_target, sreq),
                 MPIDI_rank_is_local(target_rank, win->comm_ptr), mpi_errno);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* sending flattened datatype. We'll send MPIDIG_PUT_REQ or MPIDIG_PUT_DT_REQ
     * depending on whether flattened_sz fits in the header.
     */
    am_hdr.target_count = target_count;
    am_hdr.target_datatype = MPI_DATATYPE_NULL;

    int flattened_sz;
    void *flattened_dt;
    MPIR_Datatype_get_flattened(target_datatype, &flattened_dt, &flattened_sz);
    am_hdr.flattened_sz = flattened_sz;

    MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

    size_t am_hdr_max_size;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_local = MPIDI_rank_is_local(target_rank, win->comm_ptr);
    am_hdr_max_size = is_local ? MPIDI_SHM_am_hdr_max_sz() : MPIDI_NM_am_hdr_max_sz();
#else
    am_hdr_max_size = MPIDI_NM_am_hdr_max_sz();
#endif

    int hdr_size = sizeof(am_hdr) + flattened_sz;
    if (hdr_size <= am_hdr_max_size) {
        void *header = MPL_malloc(hdr_size, MPL_MEM_OTHER);
        MPIR_Assert(header);
        MPIR_Memcpy(header, &am_hdr, sizeof(am_hdr));
        MPIR_Memcpy((char *) header + sizeof(am_hdr), flattened_dt, flattened_sz);
        CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_PUT_REQ, header, hdr_size,
                          origin_addr, origin_count, origin_datatype, vci, vci_target, sreq),
                 is_local, mpi_errno);
        MPL_free(header);
    } else {
        MPIDIG_REQUEST(sreq, buffer) = (void *) origin_addr;
        MPIDIG_REQUEST(sreq, count) = origin_count;
        MPIDIG_REQUEST(sreq, datatype) = origin_datatype;
        MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
        /* add reference to ensure the flattened_dt buffer does not get freed */
        MPIDIG_REQUEST(sreq, u.origin.target_datatype) = target_datatype;
        MPIR_Datatype_add_ref_if_not_builtin(target_datatype);

        CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_PUT_DT_REQ, &am_hdr, sizeof(am_hdr),
                          flattened_dt, flattened_sz, MPI_BYTE, vci, vci_target, sreq), is_local,
                 mpi_errno);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (sreq_ptr)
        *sreq_ptr = sreq;
    else if (sreq != NULL)
        MPIDI_CH4_REQUEST_FREE(sreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;

  immed_cmpl:
    if (sreq_ptr)
        MPIDI_RMA_REQUEST_CREATE_COMPLETE(sreq);
    goto fn_exit;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_do_get(void *origin_addr, MPI_Aint origin_count,
                                           MPI_Datatype origin_datatype, int target_rank,
                                           MPI_Aint target_disp, MPI_Aint target_count,
                                           MPI_Datatype target_datatype, MPIR_Win * win,
                                           int vci, MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    size_t offset;
    MPIR_Request *sreq = NULL;
    MPIDIG_get_msg_t am_hdr;
    MPI_Aint target_data_sz;

    MPIR_FUNC_ENTER;

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(target_datatype, target_count, target_data_sz);
    if (target_data_sz == 0)
        goto immed_cmpl;

    if (target_rank == win->comm_ptr->rank) {
        offset = win->disp_unit * target_disp;
        mpi_errno = MPIR_Localcopy((char *) win->base + offset,
                                   target_count,
                                   target_datatype, origin_addr, origin_count, origin_datatype);
        MPIR_ERR_CHECK(mpi_errno);
        goto immed_cmpl;
    }

    /* Only create request when issuing is not completed.
     * We initialize two ref_count for progress engine and request-based OP,
     * then get needs to free the second ref_count.*/
    int vci_target = MPIDI_WIN_TARGET_VCI(win, target_rank);
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 2, vci, vci_target);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    sreq->u.rma.win = win;
    MPIDIG_REQUEST(sreq, buffer) = origin_addr;
    MPIDIG_REQUEST(sreq, count) = origin_count;
    MPIDIG_REQUEST(sreq, datatype) = origin_datatype;
    MPIDIG_REQUEST(sreq, u.origin.target_datatype) = target_datatype;
    MPIDIG_REQUEST(sreq, u.origin.target_rank) = target_rank;
    MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
    MPIR_Datatype_add_ref_if_not_builtin(target_datatype);

    MPIR_cc_inc(sreq->cc_ptr);
    MPIR_T_PVAR_TIMER_START(RMA, rma_amhdr_set);
    am_hdr.target_disp = target_disp;
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        am_hdr.target_count = target_count;
        am_hdr.target_datatype = target_datatype;
    } else {
        am_hdr.target_count = target_data_sz;
        am_hdr.target_datatype = MPI_BYTE;
    }
    am_hdr.greq_ptr = sreq;
    am_hdr.win_id = MPIDIG_WIN(win, win_id);
    am_hdr.src_rank = win->comm_ptr->rank;

    /* Increase local and remote completion counters and set the local completion
     * counter in request, thus it can be decreased at request completion. */
    MPIDIG_win_cmpl_cnts_incr(win, target_rank, &sreq->dev.completion_notification);

    int is_contig;
    MPIR_Datatype_is_contig(target_datatype, &is_contig);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype) || is_contig) {
        am_hdr.flattened_sz = 0;
        MPIR_Datatype_get_true_lb(target_datatype, &am_hdr.target_true_lb);
        MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

        CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_GET_REQ, &am_hdr, sizeof(am_hdr),
                          NULL, 0, MPI_DATATYPE_NULL, vci, vci_target, sreq),
                 MPIDI_rank_is_local(target_rank, win->comm_ptr), mpi_errno);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    int flattened_sz;
    void *flattened_dt;
    MPIR_Datatype_get_flattened(target_datatype, &flattened_dt, &flattened_sz);
    am_hdr.flattened_sz = flattened_sz;
    MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

    CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_GET_REQ, &am_hdr, sizeof(am_hdr),
                      flattened_dt, flattened_sz, MPI_BYTE, vci, vci_target, sreq),
             MPIDI_rank_is_local(target_rank, win->comm_ptr), mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (sreq_ptr)
        *sreq_ptr = sreq;
    else if (sreq != NULL)
        MPIDI_CH4_REQUEST_FREE(sreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;

  immed_cmpl:
    if (sreq_ptr)
        MPIDI_RMA_REQUEST_CREATE_COMPLETE(sreq);
    goto fn_exit;

  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_do_accumulate(const void *origin_addr, MPI_Aint origin_count,
                                                  MPI_Datatype origin_datatype, int target_rank,
                                                  MPI_Aint target_disp, MPI_Aint target_count,
                                                  MPI_Datatype target_datatype,
                                                  MPI_Op op, MPIR_Win * win,
                                                  int vci, MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    size_t basic_type_size;
    MPIDIG_acc_req_msg_t am_hdr;
    MPI_Aint origin_data_sz, target_data_sz;
    MPIR_Datatype *dt_ptr;

    MPIR_FUNC_ENTER;

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_get_size_dt_ptr(origin_count, origin_datatype, origin_data_sz, dt_ptr);
    MPIDI_Datatype_check_size(target_datatype, target_count, target_data_sz);
    if (origin_data_sz == 0 || target_data_sz == 0) {
        goto immed_cmpl;
    }

    /* Only create request when issuing is not completed.
     * We initialize two ref_count for progress engine and request-based OP,
     * then acc needs to free the second ref_count.*/
    int vci_target = MPIDI_WIN_TARGET_VCI(win, target_rank);
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 2, vci, vci_target);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    sreq->u.rma.win = win;

    MPIR_cc_inc(sreq->cc_ptr);

    MPIR_T_PVAR_TIMER_START(RMA, rma_amhdr_set);
    am_hdr.req_ptr = sreq;
    am_hdr.origin_count = origin_count;

    if (HANDLE_IS_BUILTIN(origin_datatype)) {
        am_hdr.origin_datatype = origin_datatype;
    } else {
        am_hdr.origin_datatype = (dt_ptr) ? dt_ptr->basic_type : MPI_DATATYPE_NULL;
        MPIR_Datatype_get_size_macro(am_hdr.origin_datatype, basic_type_size);
        am_hdr.origin_count = (basic_type_size > 0) ? origin_data_sz / basic_type_size : 0;
    }

    am_hdr.target_count = target_count;
    am_hdr.target_datatype = target_datatype;
    am_hdr.target_disp = target_disp;
    am_hdr.op = op;
    am_hdr.win_id = MPIDIG_WIN(win, win_id);
    am_hdr.src_rank = win->comm_ptr->rank;

    /* Increase local and remote completion counters and set the local completion
     * counter in request, thus it can be decreased at request completion. */
    MPIDIG_win_cmpl_cnts_incr(win, target_rank, &sreq->dev.completion_notification);
    /* Increase remote completion counter for acc. */
    MPIDIG_win_remote_acc_cmpl_cnt_incr(win, target_rank);

    MPIDIG_REQUEST(sreq, u.origin.target_rank) = target_rank;
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        am_hdr.flattened_sz = 0;
        MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

        CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_ACC_REQ, &am_hdr, sizeof(am_hdr),
                          origin_addr, (op == MPI_NO_OP) ? 0 : origin_count, origin_datatype,
                          vci, vci_target, sreq), MPIDI_rank_is_local(target_rank, win->comm_ptr),
                 mpi_errno);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    int flattened_sz;
    void *flattened_dt;
    MPIR_Datatype_get_flattened(target_datatype, &flattened_dt, &flattened_sz);
    am_hdr.flattened_sz = flattened_sz;

    MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

    size_t am_hdr_max_size;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_local = MPIDI_rank_is_local(target_rank, win->comm_ptr);
    am_hdr_max_size = is_local ? MPIDI_SHM_am_hdr_max_sz() : MPIDI_NM_am_hdr_max_sz();
#else
    am_hdr_max_size = MPIDI_NM_am_hdr_max_sz();
#endif

    int hdr_size = sizeof(am_hdr) + flattened_sz;
    if (hdr_size <= am_hdr_max_size) {
        void *header = MPL_malloc(hdr_size, MPL_MEM_OTHER);
        MPIR_Assert(header);
        MPIR_Memcpy(header, &am_hdr, sizeof(am_hdr));
        MPIR_Memcpy((char *) header + sizeof(am_hdr), flattened_dt, flattened_sz);
        CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_ACC_REQ, header, hdr_size,
                          origin_addr, (op == MPI_NO_OP) ? 0 : origin_count, origin_datatype,
                          vci, vci_target, sreq), is_local, mpi_errno);
        MPL_free(header);
    } else {
        MPIDIG_REQUEST(sreq, buffer) = (void *) origin_addr;
        MPIDIG_REQUEST(sreq, count) = origin_count;
        MPIDIG_REQUEST(sreq, datatype) = origin_datatype;
        MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
        /* add reference to ensure the flattened_dt buffer does not get freed */
        MPIDIG_REQUEST(sreq, u.origin.target_datatype) = target_datatype;
        MPIR_Datatype_add_ref_if_not_builtin(target_datatype);

        CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_ACC_DT_REQ, &am_hdr, sizeof(am_hdr),
                          flattened_dt, flattened_sz, MPI_BYTE, vci, vci_target, sreq), is_local,
                 mpi_errno);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (sreq_ptr)
        *sreq_ptr = sreq;
    else if (sreq != NULL)
        MPIDI_CH4_REQUEST_FREE(sreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;

  immed_cmpl:
    if (sreq_ptr)
        MPIDI_RMA_REQUEST_CREATE_COMPLETE(sreq);
    goto fn_exit;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_do_get_accumulate(const void *origin_addr,
                                                      MPI_Aint origin_count_,
                                                      MPI_Datatype origin_datatype_,
                                                      void *result_addr,
                                                      MPI_Aint result_count,
                                                      MPI_Datatype result_datatype,
                                                      int target_rank,
                                                      MPI_Aint target_disp,
                                                      MPI_Aint target_count,
                                                      MPI_Datatype target_datatype,
                                                      MPI_Op op, MPIR_Win * win,
                                                      int vci, MPIR_Request ** sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    size_t basic_type_size;
    MPIDIG_get_acc_req_msg_t am_hdr;
    MPI_Aint origin_data_sz, result_data_sz, target_data_sz;
    MPIR_Datatype *dt_ptr;
    MPI_Aint origin_count = origin_count_;
    MPI_Datatype origin_datatype = origin_datatype_;

    MPIR_FUNC_ENTER;

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    if (op == MPI_NO_OP) {
        origin_count = 0;
        origin_datatype = MPI_DATATYPE_NULL;
        origin_data_sz = 0;
        dt_ptr = NULL;
    } else {
        MPIDI_Datatype_get_size_dt_ptr(origin_count, origin_datatype, origin_data_sz, dt_ptr);
    }
    MPIDI_Datatype_check_size(target_datatype, target_count, target_data_sz);
    MPIDI_Datatype_check_size(result_datatype, result_count, result_data_sz);

    if (target_data_sz == 0 || (origin_data_sz == 0 && result_data_sz == 0)) {
        goto immed_cmpl;
    }

    /* Only create request when issuing is not completed.
     * We initialize two ref_count for progress engine and request-based OP,
     * then get_acc needs to free the second ref_count.*/
    int vci_target = MPIDI_WIN_TARGET_VCI(win, target_rank);
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 2, vci, vci_target);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    sreq->u.rma.win = win;
    MPIDIG_REQUEST(sreq, req->areq.result_addr) = result_addr;
    MPIDIG_REQUEST(sreq, req->areq.result_count) = result_count;
    MPIDIG_REQUEST(sreq, req->areq.result_datatype) = result_datatype;
    MPIR_Datatype_add_ref_if_not_builtin(result_datatype);
    MPIR_cc_inc(sreq->cc_ptr);

    /* TODO: have common routine for accumulate/get_accumulate */
    MPIR_T_PVAR_TIMER_START(RMA, rma_amhdr_set);
    am_hdr.req_ptr = sreq;
    am_hdr.origin_count = origin_count;

    if (HANDLE_IS_BUILTIN(origin_datatype)) {
        am_hdr.origin_datatype = origin_datatype;
    } else {
        am_hdr.origin_datatype = (dt_ptr) ? dt_ptr->basic_type : MPI_DATATYPE_NULL;
        MPIR_Datatype_get_size_macro(am_hdr.origin_datatype, basic_type_size);
        am_hdr.origin_count = (basic_type_size > 0) ? origin_data_sz / basic_type_size : 0;
    }

    am_hdr.target_count = target_count;
    am_hdr.target_datatype = target_datatype;
    am_hdr.target_disp = target_disp;
    am_hdr.op = op;
    am_hdr.win_id = MPIDIG_WIN(win, win_id);
    am_hdr.src_rank = win->comm_ptr->rank;

    am_hdr.result_data_sz = result_data_sz;

    /* Increase local and remote completion counters and set the local completion
     * counter in request, thus it can be decreased at request completion. */
    MPIDIG_win_cmpl_cnts_incr(win, target_rank, &sreq->dev.completion_notification);
    /* Increase remote completion counter for acc. */
    MPIDIG_win_remote_acc_cmpl_cnt_incr(win, target_rank);

    MPIDIG_REQUEST(sreq, u.origin.target_rank) = target_rank;
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        am_hdr.flattened_sz = 0;
        MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

        CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_GET_ACC_REQ, &am_hdr, sizeof(am_hdr),
                          origin_addr, (op == MPI_NO_OP) ? 0 : origin_count, origin_datatype,
                          vci, vci_target, sreq), MPIDI_rank_is_local(target_rank, win->comm_ptr),
                 mpi_errno);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    int flattened_sz;
    void *flattened_dt;
    MPIR_Datatype_get_flattened(target_datatype, &flattened_dt, &flattened_sz);
    am_hdr.flattened_sz = flattened_sz;

    MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

    int am_hdr_max_size;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_local = MPIDI_rank_is_local(target_rank, win->comm_ptr);
    am_hdr_max_size = is_local ? MPIDI_SHM_am_hdr_max_sz() : MPIDI_NM_am_hdr_max_sz();
#else
    am_hdr_max_size = MPIDI_NM_am_hdr_max_sz();
#endif

    int hdr_size = sizeof(am_hdr) + flattened_sz;
    if (hdr_size <= am_hdr_max_size) {
        void *header = MPL_malloc(hdr_size, MPL_MEM_OTHER);
        MPIR_Assert(header);
        MPIR_Memcpy(header, &am_hdr, sizeof(am_hdr));
        MPIR_Memcpy((char *) header + sizeof(am_hdr), flattened_dt, flattened_sz);
        CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_GET_ACC_REQ, header, hdr_size,
                          origin_addr, (op == MPI_NO_OP) ? 0 : origin_count, origin_datatype,
                          vci, vci_target, sreq), is_local, mpi_errno);
        MPL_free(header);
    } else {
        MPIDIG_REQUEST(sreq, buffer) = (void *) origin_addr;
        MPIDIG_REQUEST(sreq, count) = origin_count;
        MPIDIG_REQUEST(sreq, datatype) = origin_datatype;
        MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
        /* add reference to ensure the flattened_dt buffer does not get freed */
        MPIDIG_REQUEST(sreq, u.origin.target_datatype) = target_datatype;
        MPIR_Datatype_add_ref_if_not_builtin(target_datatype);

        CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_GET_ACC_DT_REQ,
                          &am_hdr, sizeof(am_hdr), flattened_dt, flattened_sz, MPI_BYTE,
                          vci, vci_target, sreq), is_local, mpi_errno);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (sreq_ptr)
        *sreq_ptr = sreq;
    else if (sreq != NULL)
        MPIDI_CH4_REQUEST_FREE(sreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;

  immed_cmpl:
    if (sreq_ptr)
        MPIDI_RMA_REQUEST_CREATE_COMPLETE(sreq);
    goto fn_exit;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_put(const void *origin_addr, MPI_Aint origin_count,
                                            MPI_Datatype origin_datatype, int target_rank,
                                            MPI_Aint target_disp, MPI_Aint target_count,
                                            MPI_Datatype target_datatype, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDIG_do_put(origin_addr, origin_count, origin_datatype,
                              target_rank, target_disp, target_count, target_datatype, win, vci,
                              NULL);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_rput(const void *origin_addr, MPI_Aint origin_count,
                                             MPI_Datatype origin_datatype, int target_rank,
                                             MPI_Aint target_disp, MPI_Aint target_count,
                                             MPI_Datatype target_datatype, MPIR_Win * win,
                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_ENTER;

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDIG_do_put(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                              target_count, target_datatype, win, vci, &sreq);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    *request = sreq;
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_get(void *origin_addr, MPI_Aint origin_count,
                                            MPI_Datatype origin_datatype, int target_rank,
                                            MPI_Aint target_disp, MPI_Aint target_count,
                                            MPI_Datatype target_datatype, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDIG_do_get(origin_addr, origin_count, origin_datatype,
                              target_rank, target_disp, target_count, target_datatype,
                              win, vci, NULL);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_rget(void *origin_addr, MPI_Aint origin_count,
                                             MPI_Datatype origin_datatype, int target_rank,
                                             MPI_Aint target_disp, MPI_Aint target_count,
                                             MPI_Datatype target_datatype, MPIR_Win * win,
                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_ENTER;

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDIG_do_get(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                              target_count, target_datatype, win, vci, &sreq);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    *request = sreq;
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_raccumulate(const void *origin_addr, MPI_Aint origin_count,
                                                    MPI_Datatype origin_datatype, int target_rank,
                                                    MPI_Aint target_disp, MPI_Aint target_count,
                                                    MPI_Datatype target_datatype, MPI_Op op,
                                                    MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_ENTER;

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDIG_do_accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                     target_disp, target_count, target_datatype, op, win, vci,
                                     &sreq);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    *request = sreq;
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_accumulate(const void *origin_addr, MPI_Aint origin_count,
                                                   MPI_Datatype origin_datatype, int target_rank,
                                                   MPI_Aint target_disp, MPI_Aint target_count,
                                                   MPI_Datatype target_datatype, MPI_Op op,
                                                   MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDIG_do_accumulate(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, op,
                                     win, vci, NULL);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_rget_accumulate(const void *origin_addr,
                                                        MPI_Aint origin_count,
                                                        MPI_Datatype origin_datatype,
                                                        void *result_addr, MPI_Aint result_count,
                                                        MPI_Datatype result_datatype,
                                                        int target_rank, MPI_Aint target_disp,
                                                        MPI_Aint target_count,
                                                        MPI_Datatype target_datatype, MPI_Op op,
                                                        MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_ENTER;

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDIG_do_get_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                         result_count, result_datatype, target_rank, target_disp,
                                         target_count, target_datatype, op, win, vci, &sreq);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    *request = sreq;
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_get_accumulate(const void *origin_addr,
                                                       MPI_Aint origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       void *result_addr, MPI_Aint result_count,
                                                       MPI_Datatype result_datatype,
                                                       int target_rank, MPI_Aint target_disp,
                                                       MPI_Aint target_count,
                                                       MPI_Datatype target_datatype,
                                                       MPI_Op op, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDIG_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                         result_addr, result_count, result_datatype,
                                         target_rank, target_disp, target_count, target_datatype,
                                         op, win, vci, NULL);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_compare_and_swap(const void *origin_addr,
                                                         const void *compare_addr,
                                                         void *result_addr, MPI_Datatype datatype,
                                                         int target_rank, MPI_Aint target_disp,
                                                         MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIDIG_cswap_req_msg_t am_hdr;
    size_t data_sz;
    void *p_data;

    MPIR_FUNC_ENTER;
    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(datatype, 1, data_sz);
    if (data_sz == 0)
        goto fn_exit;

    p_data = MPL_malloc(data_sz * 2, MPL_MEM_BUFFER);
    MPIR_Assert(p_data);
    MPIR_Typerep_copy(p_data, (char *) origin_addr, data_sz, MPIR_TYPEREP_FLAG_NONE);
    MPIR_Typerep_copy((char *) p_data + data_sz, (char *) compare_addr, data_sz,
                      MPIR_TYPEREP_FLAG_NONE);

    int vci_target = MPIDI_WIN_TARGET_VCI(win, target_rank);
    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RMA, 1, vci, vci_target);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    sreq->u.rma.win = win;
    MPIDIG_REQUEST(sreq, buffer) = result_addr;
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, req->creq.data) = p_data;
    MPIDIG_REQUEST(sreq, u.origin.target_rank) = target_rank;
    MPIR_cc_inc(sreq->cc_ptr);

    MPIR_T_PVAR_TIMER_START(RMA, rma_amhdr_set);
    am_hdr.target_disp = target_disp;
    am_hdr.datatype = datatype;
    am_hdr.req_ptr = sreq;
    am_hdr.win_id = MPIDIG_WIN(win, win_id);
    am_hdr.src_rank = win->comm_ptr->rank;
    MPIR_T_PVAR_TIMER_END(RMA, rma_amhdr_set);

    MPIDIG_win_cmpl_cnts_incr(win, target_rank, &sreq->dev.completion_notification);
    /* Increase remote completion counter for acc. */
    MPIDIG_win_remote_acc_cmpl_cnt_incr(win, target_rank);

    CH4_CALL(am_isend(target_rank, win->comm_ptr, MPIDIG_CSWAP_REQ, &am_hdr, sizeof(am_hdr),
                      (char *) p_data, 2, datatype, vci, vci_target, sreq),
             MPIDI_rank_is_local(target_rank, win->comm_ptr), mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_fetch_and_op(const void *origin_addr, void *result_addr,
                                                     MPI_Datatype datatype, int target_rank,
                                                     MPI_Aint target_disp, MPI_Op op,
                                                     MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDIG_mpi_get_accumulate(origin_addr, 1, datatype, result_addr, 1, datatype,
                                          target_rank, target_disp, 1, datatype, op, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPIDIG_RMA_H_INCLUDED */
