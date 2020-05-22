/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_RMA_H_INCLUDED
#define UCX_RMA_H_INCLUDED

#include "ucx_impl.h"

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
    ucp_ep_h ep = MPIDI_UCX_AV_TO_EP(addr);

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
        req = MPIR_Request_create(MPIR_REQUEST_KIND__RMA, 0);
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

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_noncontig_put(const void *origin_addr,
                                                     int origin_count, MPI_Datatype origin_datatype,
                                                     int target_rank, size_t size,
                                                     MPI_Aint target_disp, MPI_Aint true_lb,
                                                     MPIR_Win * win, MPIDI_av_entry_t * addr,
                                                     MPIR_Request ** reqptr ATTRIBUTE((unused)))
{
    MPIDI_UCX_win_info_t *win_info = &(MPIDI_UCX_WIN_INFO(win, target_rank));
    size_t base, offset;
    int mpi_errno = MPI_SUCCESS;
    ucs_status_t status;
    char *buffer = NULL;
    ucp_ep_h ep = MPIDI_UCX_AV_TO_EP(addr);

    buffer = MPL_malloc(size, MPL_MEM_BUFFER);
    MPIR_Assert(buffer);

    MPI_Aint actual_pack_bytes;
    mpi_errno = MPIR_Typerep_pack(origin_addr, origin_count, origin_datatype, 0, buffer, size,
                                  &actual_pack_bytes);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(actual_pack_bytes == size);

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
    ucp_ep_h ep = MPIDI_UCX_AV_TO_EP(addr);

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
        req = MPIR_Request_create(MPIR_REQUEST_KIND__RMA, 0);
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_DO_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_DO_PUT);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_origin_target_contig_size_lb(origin_datatype, target_datatype,
                                                      origin_count, target_count,
                                                      origin_contig, target_contig,
                                                      origin_bytes, target_bytes,
                                                      origin_true_lb, target_true_lb);
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
        mpi_errno = MPIDIG_mpi_put(origin_addr, origin_count, origin_datatype, target_rank,
                                   target_disp, target_count, target_datatype, win);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_DO_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_DO_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_DO_GET);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);
    MPIDI_Datatype_check_origin_target_contig_size_lb(origin_datatype, target_datatype,
                                                      origin_count, target_count,
                                                      origin_contig, target_contig,
                                                      origin_bytes, target_bytes,
                                                      origin_true_lb, target_true_lb);
    MPIR_ERR_CHKANDJUMP((origin_bytes != target_bytes), mpi_errno, MPI_ERR_SIZE, "**rmasize");

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
        mpi_errno = MPIDIG_mpi_get(origin_addr, origin_count, origin_datatype, target_rank,
                                   target_disp, target_count, target_datatype, win);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_DO_GET);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_PUT);

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDIG_mpi_put(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                               target_count, target_datatype, win);
#else
    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDIG_mpi_put(origin_addr, origin_count, origin_datatype, target_rank,
                                   target_disp, target_count, target_datatype, win);
    } else {
        mpi_errno = MPIDI_UCX_do_put(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype,
                                     win, addr, NULL);
    }
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_PUT);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get(void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GET);

#ifdef MPICH_UCX_AM_ONLY
    mpi_errno = MPIDIG_mpi_get(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                               target_count, target_datatype, win);
#else
    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDIG_mpi_get(origin_addr, origin_count, origin_datatype, target_rank,
                                   target_disp, target_count, target_datatype, win);
    } else {
        mpi_errno = MPIDI_UCX_do_get(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype,
                                     win, addr, NULL);
    }
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GET);
    return mpi_errno;
}

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
    /* request based PUT relies on UCX 1.4 function ucp_put_nb */
#if defined(MPICH_UCX_AM_ONLY) || !defined(HAVE_UCP_PUT_NB)
    return MPIDIG_mpi_rput(origin_addr, origin_count, origin_datatype, target_rank,
                           target_disp, target_count, target_datatype, win, request);
#else
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RPUT);

    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDIG_mpi_rput(origin_addr, origin_count, origin_datatype, target_rank,
                                    target_disp, target_count, target_datatype, win, request);
    } else {
        MPIR_Request *sreq = NULL;

        mpi_errno = MPIDI_UCX_do_put(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype,
                                     win, addr, &sreq);
        MPIR_ERR_CHECK(mpi_errno);

        if (sreq == NULL) {
            /* create a completed request for user if issuing is completed immediately. */
            sreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
            MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
        }
        *request = sreq;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RPUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#endif
}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_compare_and_swap(const void *origin_addr,
                                                           const void *compare_addr,
                                                           void *result_addr,
                                                           MPI_Datatype datatype,
                                                           int target_rank, MPI_Aint target_disp,
                                                           MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDIG_mpi_compare_and_swap(origin_addr, compare_addr, result_addr, datatype,
                                       target_rank, target_disp, win);
}

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
    return MPIDIG_mpi_raccumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                  target_disp, target_count, target_datatype, op, win, request);
}

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
    return MPIDIG_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                      result_count, result_datatype, target_rank, target_disp,
                                      target_count, target_datatype, op, win, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_fetch_and_op(const void *origin_addr,
                                                       void *result_addr,
                                                       MPI_Datatype datatype,
                                                       int target_rank,
                                                       MPI_Aint target_disp, MPI_Op op,
                                                       MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDIG_mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank, target_disp, op,
                                   win);
}


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
    /* request based GET relies on UCX 1.4 function ucp_get_nb */
#if defined(MPICH_UCX_AM_ONLY) || !defined(HAVE_UCP_GET_NB)
    return MPIDIG_mpi_rget(origin_addr, origin_count, origin_datatype, target_rank,
                           target_disp, target_count, target_datatype, win, request);
#else
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RGET);

    if (!MPIDI_UCX_is_reachable_target(target_rank, win)) {
        mpi_errno = MPIDIG_mpi_rget(origin_addr, origin_count, origin_datatype, target_rank,
                                    target_disp, target_count, target_datatype, win, request);
    } else {
        MPIR_Request *sreq = NULL;

        mpi_errno = MPIDI_UCX_do_get(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype,
                                     win, addr, &sreq);
        MPIR_ERR_CHECK(mpi_errno);

        if (sreq == NULL) {
            /* create a completed request for user if issuing is completed immediately. */
            sreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
            MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
        }
        *request = sreq;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RGET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#endif
}


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
    return MPIDIG_mpi_get_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                     result_count, result_datatype, target_rank, target_disp,
                                     target_count, target_datatype, op, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_accumulate(const void *origin_addr,
                                                     int origin_count,
                                                     MPI_Datatype origin_datatype,
                                                     int target_rank,
                                                     MPI_Aint target_disp,
                                                     int target_count,
                                                     MPI_Datatype target_datatype, MPI_Op op,
                                                     MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    return MPIDIG_mpi_accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                 target_disp, target_count, target_datatype, op, win);
}

#endif /* UCX_RMA_H_INCLUDED */
