/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_RMA_H_INCLUDED
#define UCX_RMA_H_INCLUDED

#include "ucx_impl.h"
#include "mpir_op_util.h"

/* UCP does not guarantee ordering of atomics, thus we need fence if
 * ordering is required */
MPL_STATIC_INLINE_PREFIX ucs_status_t MPIDI_UCX_atomic_force_ordering(MPIR_Win * win,
                                                                      int order_needed)
{
    ucs_status_t fence_status = UCS_OK;
    /* TODO: assume the global fence is a no-op for most transports. Otherwise
     * it might be faster to wait completion of the ucp_request with ucp progress polling.*/
    if (MPIDI_CH4U_WIN(win, info_args).accumulate_ordering & order_needed)
        fence_status = ucp_worker_fence(MPIDI_UCX_global.worker);
  fn_exit:
    return fence_status;
  fn_fail:
    goto fn_exit;
}

/* Wait remote completion of all outstanding atomics on this target.
 * It is needed to ensure ordering and atomicity with AM. */
MPL_STATIC_INLINE_PREFIX ucs_status_t MPIDI_UCX_atomic_force_cmpl(MPIR_Win * win, ucp_ep_h ep,
                                                                  int target_rank)
{
    ucs_status_t flush_status = UCS_OK;
    if (MPIDI_UCX_WIN(win).target_sync[target_rank].outstanding_atomics) {
        flush_status = ucp_ep_flush(ep);
        MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_UNSET;
        MPIDI_UCX_WIN(win).target_sync[target_rank].outstanding_atomics = false;
    }
  fn_exit:
    return flush_status;
  fn_fail:
    goto fn_exit;
}

/* Most atomic operations except SWAP, CSWAP only support integer type.
 * Return true if it is integer, otherwise false. */
MPL_STATIC_INLINE_PREFIX bool MPIDI_UCX_check_integer_dtype(MPI_Datatype datatype)
{
    switch (datatype) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_, type_name_) case mpi_type_:
            MPIR_OP_TYPE_GROUP(C_INTEGER)
                MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
                MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)
                MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA)
                return true;
            break;
#undef MPIR_OP_TYPE_MACRO
        default:
            return false;
            break;
    }
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_UCX_query_ucp_atomic_fetch_op(MPI_Op op,
                                                                  MPI_Datatype datatype,
                                                                  ucp_atomic_fetch_op_t * ucp_op)
{
    bool supported = false;
    switch (op) {
        case MPI_SUM:
        case MPI_NO_OP:        /* NO_OP can be treaded as ADD with zero value. */
            if (MPIDI_UCX_check_integer_dtype(datatype)) {
                *ucp_op = UCP_ATOMIC_FETCH_OP_FADD;
                supported = true;
            }
            break;
        case MPI_REPLACE:
            *ucp_op = UCP_ATOMIC_FETCH_OP_SWAP;
            supported = true;
            break;
        default:
            break;
    }
    return supported;
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_UCX_query_ucp_atomic_post_op(MPI_Op op,
                                                                 MPI_Datatype datatype,
                                                                 ucp_atomic_post_op_t * ucp_op)
{
    bool supported = false;
    switch (op) {
        case MPI_SUM:
            if (MPIDI_UCX_check_integer_dtype(datatype)) {
                *ucp_op = UCP_ATOMIC_POST_OP_ADD;
                supported = true;
            }
            break;
        default:
            break;
    }
    return supported;
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_UCX_check_addr_aligned(uint64_t addr, size_t align)
{
    return addr == ((addr + (align - 1)) & (~(align - 1)));
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_UCX_check_ucp_atomic_dbyte_support(size_t data_bytes)
{
    bool supported = false;

    /* UCP atomic supports only 4 or 8 bytes (FIXME: Is it true ?) */
    if ((MPIR_CVAR_CH4_UCX_ENABLE_AMO32 && data_bytes == 4) ||
        (MPIR_CVAR_CH4_UCX_ENABLE_AMO64 && data_bytes == 8))
        supported = true;
    return supported;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_rma_cmpl_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_rma_cmpl_cb(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *req = ucp_request->req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_RMA_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_RMA_CMPL_CB);

    MPIR_Assert(status == UCS_OK);
    if (req) {
        MPID_Request_complete(req);
        ucp_request->req = NULL;
    }
    ucp_request_free(ucp_request);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_RMA_CMPL_CB);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_contig_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_contig_put(const void *origin_addr,
                                                  size_t size,
                                                  int target_rank,
                                                  MPI_Aint target_disp, MPI_Aint true_lb,
                                                  MPIR_Win * win, MPIDI_av_entry_t * addr,
                                                  MPIR_Request ** reqptr)
{

    MPIDI_UCX_win_info_t *win_info = &(MPIDI_UCX_WIN_INFO(win, target_rank));
    size_t offset;
    uint64_t base;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request ATTRIBUTE((unused)) = NULL;
    MPIR_Comm *comm = win->comm_ptr;
    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, target_rank);

    base = win_info->addr;
    offset = target_disp * win_info->disp + true_lb;

    /* Put without request */
    if (likely(!reqptr)) {
        ucs_status_t status;
        status = ucp_put_nbi(ep, origin_addr, size, base + offset, win_info->rkey);
        if (status == UCS_INPROGRESS)
            MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL;
        else if (status == UCS_OK)
            /* UCX 1.4 spec: completed immediately if returns UCS_OK.
             * FIXME: is it local completion or remote ? Now we assume local,
             * so we need flush to ensure remote completion.*/
            MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH;
        else
            MPIDI_UCX_CHK_STATUS(status);
        goto fn_exit;
    }
#ifdef HAVE_UCP_PUT_NB  /* ucp_put_nb is provided since UCX 1.4 */
    /* Put with request */
    ucp_request = ucp_put_nb(ep, origin_addr, size, base + offset, win_info->rkey,
                             MPIDI_UCX_rma_cmpl_cb);
    if (ucp_request == UCS_OK) {
        /* UCX 1.4 spec: completed immediately if returns UCS_OK.
         * FIXME: is it local completion or remote ? Now we assume local,
         * so we need flush to ensure remote completion.*/
        MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH;
    } else {
        MPIDI_UCX_CHK_REQUEST(ucp_request);

        /* Create an MPI request and return. The completion cb will complete
         * the request and release ucp_request. */
        MPIR_Request *req = NULL;
        req = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT(req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Request_add_ref(req);
        ucp_request->req = req;
        *reqptr = req;

        MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL;
    }
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_noncontig_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_noncontig_put(const void *origin_addr,
                                                     int origin_count, MPI_Datatype origin_datatype,
                                                     int target_rank, size_t size,
                                                     MPI_Aint target_disp, MPI_Aint true_lb,
                                                     MPIR_Win * win, MPIDI_av_entry_t * addr,
                                                     MPIR_Request ** reqptr ATTRIBUTE((unused)))
{
    MPIDI_UCX_win_info_t *win_info = &(MPIDI_UCX_WIN_INFO(win, target_rank));
    MPI_Aint segment_first, last;
    size_t base, offset;
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t status;
    struct MPIR_Segment *segment_ptr;
    char *buffer = NULL;
    MPIR_Comm *comm = win->comm_ptr;
    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, target_rank);

    segment_ptr = MPIR_Segment_alloc();
    MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                         MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send MPIR_Segment_alloc");
    MPIR_Segment_init(origin_addr, origin_count, origin_datatype, segment_ptr);
    segment_first = 0;
    last = size;

    buffer = MPL_malloc(size, MPL_MEM_BUFFER);
    MPIR_Assert(buffer);
    MPIR_Segment_pack(segment_ptr, segment_first, &last, buffer);
    MPIR_Segment_free(segment_ptr);

    base = win_info->addr;
    offset = target_disp * win_info->disp + true_lb;
    /* We use the blocking put here - should be faster than send/recv - ucp_put returns when it is
     * locally completed. In reality this means, when the data are copied to the internal UCP-buffer */
    status = ucp_put(ep, buffer, size, base + offset, win_info->rkey);
    MPIDI_UCX_CHK_STATUS(status);

    /* Only need remote flush */
    MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH;

  fn_exit:
    MPL_free(buffer);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_contig_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_contig_get(void *origin_addr,
                                                  size_t size,
                                                  int target_rank,
                                                  MPI_Aint target_disp, MPI_Aint true_lb,
                                                  MPIR_Win * win, MPIDI_av_entry_t * addr,
                                                  MPIR_Request ** reqptr)
{

    MPIDI_UCX_win_info_t *win_info = &(MPIDI_UCX_WIN_INFO(win, target_rank));
    size_t base, offset;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request ATTRIBUTE((unused)) = NULL;
    MPIR_Comm *comm = win->comm_ptr;
    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, target_rank);

    base = win_info->addr;
    offset = target_disp * win_info->disp + true_lb;

    /* Get without request */
    if (likely(!reqptr)) {
        ucs_status_t status;
        status = ucp_get_nbi(ep, origin_addr, size, base + offset, win_info->rkey);
        MPIDI_UCX_CHK_STATUS(status);

        /* UCX 1.4 spec: ucp_get_nbi always returns immediately and does not
         * guarantee completion */
        MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL;
        goto fn_exit;
    }
#ifdef HAVE_UCP_GET_NB  /* ucp_get_nb is provided since UCX 1.4 */
    /* Get with request */
    ucp_request = ucp_get_nb(ep, origin_addr, size, base + offset, win_info->rkey,
                             MPIDI_UCX_rma_cmpl_cb);
    if (ucp_request != UCS_OK) {
        MPIDI_UCX_CHK_REQUEST(ucp_request);

        /* Create an MPI request and return. The completion cb will complete
         * the request and release ucp_request. */
        MPIR_Request *req = NULL;
        req = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT(req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Request_add_ref(req);
        ucp_request->req = req;
        *reqptr = req;

        MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL;
    }
    /* otherwise completed immediately */
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_do_accumulate(const void *origin_addr,
                                                     int origin_count,
                                                     MPI_Datatype origin_datatype,
                                                     int target_rank,
                                                     MPI_Aint target_disp,
                                                     int target_count,
                                                     MPI_Datatype target_datatype, MPI_Op op,
                                                     MPIR_Win * win, MPIR_Request ** reqptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_win_info_t *win_info = &(MPIDI_UCX_WIN_INFO(win, target_rank));
    MPIR_Comm *comm = win->comm_ptr;
    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, target_rank);
    ucs_status_t status;
    int target_contig, origin_contig;
    size_t target_bytes, origin_bytes;
    MPI_Aint origin_true_lb, target_true_lb ATTRIBUTE((unused));
    bool supported_op = false;
    ucp_atomic_post_op_t ucp_op;
    int order_needed = MPIDI_CH4I_ACCU_ORDER_RAW | MPIDI_CH4I_ACCU_ORDER_WAW;   /* ACC is WRITE */
    uint64_t remote_addr = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_DO_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_DO_ACCUMULATE);

    MPIDI_CH4U_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_contig_size_lb(origin_datatype, origin_count, origin_contig,
                                        origin_bytes, origin_true_lb);
    MPIDI_Datatype_check_contig_size_lb(target_datatype, target_count, target_contig,
                                        target_bytes, target_true_lb);
    if (origin_bytes == 0 || target_bytes == 0)
        goto fn_exit;

    /* Check whether <op, data_bytes> is supported with post or it is
     * MPI_REPLACE which we replace with blocking SWAP */
    supported_op = MPIDI_UCX_query_ucp_atomic_post_op(op, origin_datatype, &ucp_op);
    remote_addr = win_info->addr + (uint64_t) (target_disp * win_info->disp);
    if (!origin_contig || !target_contig || (!supported_op && op != MPI_REPLACE) ||
        !MPIDI_UCX_check_ucp_atomic_dbyte_support(origin_bytes) ||
        !MPIDI_UCX_check_addr_aligned(remote_addr, origin_bytes))
        goto am_fallback;

    /* Ensure all outstanding AM atomics are done */
    MPIDI_CH4U_wait_am_acc(win, target_rank, order_needed);

    if (unlikely(op == MPI_REPLACE)) {
        if (origin_bytes == 4) {
            uint32_t result;
            MPIR_Assert(MPIR_CVAR_CH4_UCX_ENABLE_AMO32);
            status = ucp_atomic_swap32(ep, *(uint32_t *) ((char *) origin_addr + origin_true_lb),
                                       remote_addr, win_info->rkey, &result);
        } else {
            uint64_t result;
            MPIR_Assert(MPIR_CVAR_CH4_UCX_ENABLE_AMO64);
            status = ucp_atomic_swap64(ep, *(uint64_t *) origin_addr,
                                       remote_addr, win_info->rkey, &result);
        }

        MPIDI_UCX_CHK_STATUS(status);
        /* Return from atomic_swap guarantees completion,
         * thus skip force ordering and completion set.*/

    } else {
        status = ucp_atomic_post(ep, ucp_op, *(uint64_t *) origin_addr, origin_bytes,
                                 remote_addr, win_info->rkey);
        MPIDI_UCX_CHK_STATUS(status);

        /* Return from atomic_post does not guarantee completion no matter
         * what status is returned */

        if (unlikely(reqptr)) {
            /* Force remote completion for racc, because we cannot generate
             * individual request. */
            status = ucp_ep_flush(ep);
            MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_UNSET;
            MPIDI_UCX_WIN(win).target_sync[target_rank].outstanding_atomics = false;
        } else {
            status = MPIDI_UCX_atomic_force_ordering(win, order_needed);
            MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL;
            MPIDI_UCX_WIN(win).target_sync[target_rank].outstanding_atomics = true;
        }
        MPIDI_UCX_CHK_STATUS(status);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_DO_ACCUMULATE);
    return mpi_errno;
  am_fallback:
    /* Force remote completion of all outstanding atomics on this target
     * to ensure ordering and atomicity with AM. */
    status = MPIDI_UCX_atomic_force_cmpl(win, ep, target_rank);
    MPIDI_UCX_CHK_STATUS(status);

    if (unlikely(reqptr))
        mpi_errno = MPIDI_CH4U_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                               target_rank, target_disp, target_count,
                                               target_datatype, op, win, reqptr);
    else
        mpi_errno = MPIDI_CH4U_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win);
    goto fn_exit;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_do_get_accumulate(const void *origin_addr,
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
                                                         MPIR_Request ** reqptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_win_info_t *win_info = &(MPIDI_UCX_WIN_INFO(win, target_rank));
    MPIR_Comm *comm = win->comm_ptr;
    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, target_rank);
    MPIDI_UCX_ucp_request_t *ucp_request = NULL;
    ucs_status_t status;
    int target_contig, origin_contig, result_contig;
    size_t target_bytes, origin_bytes, result_bytes;
    MPI_Aint origin_true_lb, target_true_lb ATTRIBUTE((unused)), result_true_lb;
    bool supported_op = false;
    ucp_atomic_fetch_op_t ucp_op;
    int order_needed = 0;
    uint64_t origin_value = 0;
    uint64_t remote_addr = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_DO_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_DO_GET_ACCUMULATE);

    MPIDI_CH4U_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_contig_size_lb(origin_datatype, origin_count, origin_contig,
                                        origin_bytes, origin_true_lb);
    MPIDI_Datatype_check_contig_size_lb(target_datatype, target_count, target_contig,
                                        target_bytes, target_true_lb);
    MPIDI_Datatype_check_contig_size_lb(result_datatype, result_count, result_contig,
                                        result_bytes, result_true_lb);
    if (target_bytes == 0 || (origin_bytes == 0 && result_bytes == 0))
        goto fn_exit;

    /* Check whether op is supported with fetch */
    supported_op = MPIDI_UCX_query_ucp_atomic_fetch_op(op, origin_datatype, &ucp_op);
    remote_addr = win_info->addr + (uint64_t) (target_disp * win_info->disp);
    if (!origin_contig || !target_contig || !result_contig || !supported_op ||
        !MPIDI_UCX_check_ucp_atomic_dbyte_support(origin_bytes) ||
        !MPIDI_UCX_check_addr_aligned(remote_addr, origin_bytes))
        goto am_fallback;

    /* GET_ACC with NO_OP is READ, otherwise READ and WRITE */
    if (unlikely(op == MPI_NO_OP))
        order_needed = MPIDI_CH4I_ACCU_ORDER_WAR | MPIDI_CH4I_ACCU_ORDER_RAR;
    else
        order_needed = MPIDI_CH4I_ACCU_ORDER_RAW | MPIDI_CH4I_ACCU_ORDER_WAW |
            MPIDI_CH4I_ACCU_ORDER_WAR | MPIDI_CH4I_ACCU_ORDER_RAR;

    /* Ensure all outstanding AM atomics are done */
    MPIDI_CH4U_wait_am_acc(win, target_rank, order_needed);

    if (unlikely(op == MPI_NO_OP)) {
        origin_value = 0;       /* atomic get = FADD with 0 */
    } else
        origin_value = *(uint64_t *) ((char *) origin_addr + origin_true_lb);
    result_addr = (void *) ((char *) result_addr + result_true_lb);

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_atomic_fetch_nb(ep, ucp_op, origin_value,
                                                                  result_addr, origin_bytes,
                                                                  remote_addr, win_info->rkey,
                                                                  MPIDI_UCX_rma_cmpl_cb);
    if (ucp_request == UCS_OK)  /* completed immediately, skip force ordering and completion set. */
        goto fn_exit;
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (unlikely(reqptr)) {
        /* Create an MPI request and return. The completion cb will complete
         * the request and release ucp_request. */
        MPIR_Request *req = NULL;
        req = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT(req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Request_add_ref(req);
        ucp_request->req = req;
        *reqptr = req;
    } else
        ucp_request->req = NULL;

    status = MPIDI_UCX_atomic_force_ordering(win, order_needed);
    MPIDI_UCX_CHK_STATUS(status);

    MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL;
    MPIDI_UCX_WIN(win).target_sync[target_rank].outstanding_atomics = true;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_DO_GET_ACCUMULATE);
    return mpi_errno;
  am_fallback:
    /* Force remote completion of all outstanding atomics on this target
     * to ensure ordering and atomicity with AM. */
    status = MPIDI_UCX_atomic_force_cmpl(win, ep, target_rank);
    MPIDI_UCX_CHK_STATUS(status);

    if (unlikely(reqptr))
        mpi_errno = MPIDI_CH4U_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                   result_addr, result_count, result_datatype,
                                                   target_rank, target_disp, target_count,
                                                   target_datatype, op, win, reqptr);
    else
        mpi_errno = MPIDI_CH4U_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                  result_addr, result_count, result_datatype,
                                                  target_rank, target_disp, target_count,
                                                  target_datatype, op, win);
    goto fn_exit;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_do_put(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * addr,
                                              MPIR_Request ** reqptr)
{
    int mpi_errno = MPI_SUCCESS;
    int target_contig, origin_contig;
    size_t target_bytes, origin_bytes;
    MPI_Aint origin_true_lb, target_true_lb;
    size_t offset;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_DO_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_DO_PUT);

    MPIDI_CH4U_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_contig_size_lb(target_datatype, target_count,
                                        target_contig, target_bytes, target_true_lb);
    MPIDI_Datatype_check_contig_size_lb(origin_datatype, origin_count,
                                        origin_contig, origin_bytes, origin_true_lb);
    MPIR_ERR_CHKANDJUMP((origin_bytes != target_bytes), mpi_errno, MPI_ERR_SIZE, "**rmasize");

    if (unlikely(origin_bytes == 0))
        goto fn_exit;

    if (target_rank == win->comm_ptr->rank) {
        offset = win->disp_unit * target_disp;
        mpi_errno = MPIR_Localcopy(origin_addr,
                                   origin_count,
                                   origin_datatype,
                                   (char *) win->base + offset, target_count, target_datatype);
        goto fn_exit;
    }

    if (origin_contig && target_contig) {
        mpi_errno = MPIDI_UCX_contig_put((char *) origin_addr + origin_true_lb, origin_bytes,
                                         target_rank, target_disp, target_true_lb, win, addr,
                                         reqptr);
    } else if (target_contig) {
        mpi_errno = MPIDI_UCX_noncontig_put(origin_addr, origin_count, origin_datatype, target_rank,
                                            target_bytes, target_disp, target_true_lb, win, addr,
                                            reqptr);
    } else {
        mpi_errno = MPIDI_CH4U_mpi_put(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count, target_datatype,
                                       win);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_DO_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_do_get(void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * addr,
                                              MPIR_Request ** reqptr)
{
    int mpi_errno = MPI_SUCCESS;
    int origin_contig, target_contig;
    size_t origin_bytes, target_bytes ATTRIBUTE((unused));
    size_t offset;
    MPI_Aint origin_true_lb, target_true_lb;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_DO_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_DO_GET);

    MPIDI_CH4U_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_contig_size_lb(target_datatype, target_count,
                                        target_contig, target_bytes, target_true_lb);
    MPIDI_Datatype_check_contig_size_lb(origin_datatype, origin_count,
                                        origin_contig, origin_bytes, origin_true_lb);

    if (unlikely(origin_bytes == 0))
        goto fn_exit;

    if (target_rank == win->comm_ptr->rank) {
        offset = target_disp * win->disp_unit;
        mpi_errno = MPIR_Localcopy((char *) win->base + offset,
                                   target_count,
                                   target_datatype, origin_addr, origin_count, origin_datatype);
        goto fn_exit;
    }

    if (origin_contig && target_contig) {
        mpi_errno = MPIDI_UCX_contig_get((char *) origin_addr + origin_true_lb, origin_bytes,
                                         target_rank, target_disp, target_true_lb, win, addr,
                                         reqptr);
    } else {
        mpi_errno = MPIDI_CH4U_mpi_get(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count, target_datatype,
                                       win);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_DO_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_put(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_PUT);

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_CH4U_mpi_put(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win);
#else
    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDI_CH4U_mpi_put(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count, target_datatype,
                                       win);
    } else {
        mpi_errno = MPIDI_UCX_do_put(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype,
                                     win, addr, NULL);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get(void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_GET);

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_CH4U_mpi_get(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win);
#else
    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDI_CH4U_mpi_get(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count, target_datatype,
                                       win);
    } else {
        mpi_errno = MPIDI_UCX_do_get(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype,
                                     win, addr, NULL);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_rput
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rput(const void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count,
                                               MPI_Datatype target_datatype,
                                               MPIR_Win * win,
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_RGET);

    /* request based PUT relies on UCX 1.4 function ucp_put_nb */
#if defined(MPICH_UCX_AM_ONLY) || !defined(HAVE_UCP_PUT_NB)
    mpi_errno = MPIDI_CH4U_mpi_rput(origin_addr, origin_count, origin_datatype,
                                    target_rank, target_disp, target_count, target_datatype, win,
                                    request);
#else
    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDI_CH4U_mpi_rput(origin_addr, origin_count, origin_datatype,
                                        target_rank, target_disp, target_count, target_datatype,
                                        win, request);
    } else {
        MPIR_Request *sreq = NULL;

        mpi_errno = MPIDI_UCX_do_put(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype,
                                     win, addr, &sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (sreq == NULL) {
            /* create a completed request for user if issuing is completed immediately. */
            sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
            MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
            MPIR_Request_add_ref(sreq);
            MPID_Request_complete(sreq);
        }
        *request = sreq;
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_RGET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_compare_and_swap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_compare_and_swap(const void *origin_addr,
                                                           const void *compare_addr,
                                                           void *result_addr,
                                                           MPI_Datatype datatype,
                                                           int target_rank, MPI_Aint target_disp,
                                                           MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_COMPARE_AND_SWAP);

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_CH4U_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                datatype, target_rank, target_disp, win);
#else
    size_t data_sz;
    MPIDI_UCX_win_info_t *win_info = &(MPIDI_UCX_WIN_INFO(win, target_rank));
    MPIR_Comm *comm = win->comm_ptr;
    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, target_rank);
    MPIDI_UCX_ucp_request_t *ucp_request = NULL;
    ucs_status_t status;
    int order_needed = MPIDI_CH4I_ACCU_ORDER_RAW | MPIDI_CH4I_ACCU_ORDER_WAW | MPIDI_CH4I_ACCU_ORDER_WAR | MPIDI_CH4I_ACCU_ORDER_RAR;   /* CAS is READ and WRITE */
    uint64_t remote_addr = 0;

    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno =
            MPIDI_CH4U_mpi_compare_and_swap(origin_addr, compare_addr, result_addr, datatype,
                                            target_rank, target_disp, win);
        goto fn_exit;
    }

    MPIDI_CH4U_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_size(datatype, 1, data_sz);
    if (data_sz == 0)
        goto fn_exit;

    remote_addr = win_info->addr + (uint64_t) (target_disp * win_info->disp);
    if (!MPIDI_UCX_check_ucp_atomic_dbyte_support(data_sz) ||
        !MPIDI_UCX_check_addr_aligned(remote_addr, data_sz))
        goto am_fallback;

    /* Ensure all outstanding AM atomics are done */
    MPIDI_CH4U_wait_am_acc(win, target_rank, order_needed);

    /* CSWAP: value in result_addr will be swapped into remote_addr when cond is true. */
    *(uint64_t *) result_addr = *(uint64_t *) origin_addr;

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_atomic_fetch_nb(ep, UCP_ATOMIC_FETCH_OP_CSWAP,
                                                                  *(uint64_t *) compare_addr,
                                                                  result_addr, data_sz, remote_addr,
                                                                  win_info->rkey,
                                                                  MPIDI_UCX_rma_cmpl_cb);
    if (ucp_request == UCS_OK)  /* completed immediately, skip force ordering and completion set. */
        goto fn_exit;

    MPIDI_UCX_CHK_REQUEST(ucp_request);
    ucp_request->req = NULL;

    status = MPIDI_UCX_atomic_force_ordering(win, order_needed);
    MPIDI_UCX_CHK_STATUS(status);

    MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL;
    MPIDI_UCX_WIN(win).target_sync[target_rank].outstanding_atomics = true;
    goto fn_exit;       /* skip am_fallback */

  am_fallback:
    /* Force remote completion of all outstanding atomics on this target
     * to ensure ordering and atomicity with AM. */
    status = MPIDI_UCX_atomic_force_cmpl(win, ep, target_rank);
    MPIDI_UCX_CHK_STATUS(status);

    mpi_errno = MPIDI_CH4U_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                datatype, target_rank, target_disp, win);
    goto fn_exit;
#endif /* MPICH_UCX_AM_ONLY */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_COMPARE_AND_SWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_raccumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_raccumulate(const void *origin_addr,
                                                      int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      int target_rank,
                                                      MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype,
                                                      MPI_Op op, MPIR_Win * win,
                                                      MPIDI_av_entry_t * addr,
                                                      MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_RACCUMULATE);

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_CH4U_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                           target_rank, target_disp, target_count,
                                           target_datatype, op, win, request);
#else
    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDI_CH4U_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                               target_rank, target_disp, target_count,
                                               target_datatype, op, win, request);
    } else {
        MPIR_Request *sreq = NULL;
        mpi_errno = MPIDI_UCX_do_accumulate(origin_addr, origin_count, origin_datatype,
                                            target_rank, target_disp, target_count, target_datatype,
                                            op, win, &sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (sreq == NULL) {
            /* always create a completed request for user. */
            sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
            MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
            MPIR_Request_add_ref(sreq);
            MPID_Request_complete(sreq);
        }
        *request = sreq;
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_RACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_rget_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rget_accumulate(const void *origin_addr,
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
                                                          MPIDI_av_entry_t * addr,
                                                          MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_RGET_ACCUMULATE);

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_CH4U_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                               result_addr, result_count, result_datatype,
                                               target_rank, target_disp, target_count,
                                               target_datatype, op, win, request);
#else
    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDI_CH4U_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                   result_addr, result_count, result_datatype,
                                                   target_rank, target_disp, target_count,
                                                   target_datatype, op, win, request);
    } else {
        MPIR_Request *sreq = NULL;
        mpi_errno = MPIDI_UCX_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                result_addr, result_count, result_datatype,
                                                target_rank, target_disp, target_count,
                                                target_datatype, op, win, &sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (sreq == NULL) {
            /* always create a completed request for user. */
            sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
            MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
            MPIR_Request_add_ref(sreq);
            MPID_Request_complete(sreq);
        }
        *request = sreq;
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_RGET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_fetch_and_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_fetch_and_op(const void *origin_addr,
                                                       void *result_addr,
                                                       MPI_Datatype datatype,
                                                       int target_rank,
                                                       MPI_Aint target_disp, MPI_Op op,
                                                       MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_FETCH_AND_OP);

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_CH4U_mpi_fetch_and_op(origin_addr, result_addr, datatype,
                                            target_rank, target_disp, op, win);
#else
    size_t data_sz;
    MPIDI_UCX_win_info_t *win_info = &(MPIDI_UCX_WIN_INFO(win, target_rank));
    MPIR_Comm *comm = win->comm_ptr;
    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, target_rank);
    MPIDI_UCX_ucp_request_t *ucp_request = NULL;
    ucs_status_t status;
    bool supported_op = false;
    ucp_atomic_fetch_op_t ucp_op;
    uint64_t origin_value = 0;
    int order_needed = 0;
    uint64_t remote_addr = 0;

    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDI_CH4U_mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank,
                                                target_disp, op, win);
        goto fn_exit;
    }

    MPIDI_CH4U_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_size(datatype, 1, data_sz);
    if (data_sz == 0)
        goto fn_exit;

    supported_op = MPIDI_UCX_query_ucp_atomic_fetch_op(op, datatype, &ucp_op);
    remote_addr = win_info->addr + (uint64_t) (target_disp * win_info->disp);
    if (!MPIDI_UCX_check_ucp_atomic_dbyte_support(data_sz) || !supported_op ||
        !MPIDI_UCX_check_addr_aligned(remote_addr, data_sz))
        goto am_fallback;

    /* FOP with NO_OP is READ, otherwise READ and WRITE */
    if (unlikely(op == MPI_NO_OP))
        order_needed = MPIDI_CH4I_ACCU_ORDER_WAR | MPIDI_CH4I_ACCU_ORDER_RAR;
    else
        order_needed = MPIDI_CH4I_ACCU_ORDER_RAW | MPIDI_CH4I_ACCU_ORDER_WAW |
            MPIDI_CH4I_ACCU_ORDER_WAR | MPIDI_CH4I_ACCU_ORDER_RAR;

    /* Ensure all outstanding AM atomics are done */
    MPIDI_CH4U_wait_am_acc(win, target_rank, order_needed);

    if (unlikely(op == MPI_NO_OP)) {
        origin_value = 0;       /* atomic get = FADD with 0 */
    } else
        origin_value = *(uint64_t *) origin_addr;

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_atomic_fetch_nb(ep, ucp_op, origin_value,
                                                                  result_addr, data_sz, remote_addr,
                                                                  win_info->rkey,
                                                                  MPIDI_UCX_rma_cmpl_cb);
    if (ucp_request == UCS_OK)  /* completed immediately, skip force ordering and completion set. */
        goto fn_exit;

    MPIDI_UCX_CHK_REQUEST(ucp_request);
    ucp_request->req = NULL;

    status = MPIDI_UCX_atomic_force_ordering(win, order_needed);
    MPIDI_UCX_CHK_STATUS(status);

    MPIDI_UCX_WIN(win).target_sync[target_rank].need_sync = MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL;
    MPIDI_UCX_WIN(win).target_sync[target_rank].outstanding_atomics = true;
    goto fn_exit;       /* skip am_fallback */

  am_fallback:
    /* Force remote completion of all outstanding atomics on this target
     * to ensure ordering and atomicity with AM. */
    status = MPIDI_UCX_atomic_force_cmpl(win, ep, target_rank);
    MPIDI_UCX_CHK_STATUS(status);

    mpi_errno = MPIDI_CH4U_mpi_fetch_and_op(origin_addr, result_addr, datatype,
                                            target_rank, target_disp, op, win);
    goto fn_exit;
#endif /* MPICH_UCX_AM_ONLY */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_FETCH_AND_OP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_rget
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rget(void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count,
                                               MPI_Datatype target_datatype,
                                               MPIR_Win * win,
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_RGET);

    /* request based GET relies on UCX 1.4 function ucp_get_nb */
#if defined(MPICH_UCX_AM_ONLY) || !defined(HAVE_UCP_GET_NB)
    mpi_errno = MPIDI_CH4U_mpi_rget(origin_addr, origin_count, origin_datatype,
                                    target_rank, target_disp, target_count, target_datatype, win,
                                    request);
#else
    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDI_CH4U_mpi_rget(origin_addr, origin_count, origin_datatype,
                                        target_rank, target_disp, target_count, target_datatype,
                                        win, request);
    } else {
        MPIR_Request *sreq = NULL;

        mpi_errno = MPIDI_UCX_do_get(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype,
                                     win, addr, &sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (sreq == NULL) {
            /* create a completed request for user if issuing is completed immediately. */
            sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
            MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
            MPIR_Request_add_ref(sreq);
            MPID_Request_complete(sreq);
        }
        *request = sreq;
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_RGET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_get_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get_accumulate(const void *origin_addr,
                                                         int origin_count,
                                                         MPI_Datatype origin_datatype,
                                                         void *result_addr,
                                                         int result_count,
                                                         MPI_Datatype result_datatype,
                                                         int target_rank,
                                                         MPI_Aint target_disp,
                                                         int target_count,
                                                         MPI_Datatype target_datatype, MPI_Op op,
                                                         MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_GET_ACCUMULATE);

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_CH4U_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                              result_addr, result_count, result_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win);
#else
    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDI_CH4U_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                  result_addr, result_count, result_datatype,
                                                  target_rank, target_disp, target_count,
                                                  target_datatype, op, win);
    } else {
        mpi_errno = MPIDI_UCX_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                result_addr, result_count, result_datatype,
                                                target_rank, target_disp, target_count,
                                                target_datatype, op, win, NULL);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_GET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_accumulate(const void *origin_addr,
                                                     int origin_count,
                                                     MPI_Datatype origin_datatype,
                                                     int target_rank,
                                                     MPI_Aint target_disp,
                                                     int target_count,
                                                     MPI_Datatype target_datatype, MPI_Op op,
                                                     MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_UCX_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_UCX_ACCUMULATE);

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDI_CH4U_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                          target_rank, target_disp, target_count, target_datatype,
                                          op, win);
#else
    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDI_CH4U_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win);
    } else {
        mpi_errno = MPIDI_UCX_do_accumulate(origin_addr, origin_count, origin_datatype,
                                            target_rank, target_disp, target_count, target_datatype,
                                            op, win, NULL);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_UCX_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* UCX_RMA_H_INCLUDED */
