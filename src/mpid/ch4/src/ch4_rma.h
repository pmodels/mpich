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
#ifndef CH4_RMA_H_INCLUDED
#define CH4_RMA_H_INCLUDED

#include "ch4_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_put_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_put_unsafe(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PUT_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PUT_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_put(origin_addr, origin_count, origin_datatype,
                                 target_rank, target_disp, target_count, target_datatype, win,
                                 NULL);
#else
    int r;
    MPIDI_av_entry_t *av = NULL;

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_put(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count, target_datatype, win);
    else
        mpi_errno = MPIDI_NM_mpi_put(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, win,
                                     NULL);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PUT_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_get_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_get_unsafe(void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GET_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GET_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_get(origin_addr, origin_count, origin_datatype,
                                 target_rank, target_disp, target_count, target_datatype, win,
                                 NULL);
#else
    int r;
    MPIDI_av_entry_t *av = NULL;

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_get(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count, target_datatype, win);
    else
        mpi_errno = MPIDI_NM_mpi_get(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, win,
                                     NULL);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GET_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_accumulate_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_accumulate_unsafe(const void *origin_addr,
                                                     int origin_count,
                                                     MPI_Datatype origin_datatype,
                                                     int target_rank,
                                                     MPI_Aint target_disp,
                                                     int target_count,
                                                     MPI_Datatype target_datatype, MPI_Op op,
                                                     MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ACCUMULATE_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ACCUMULATE_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                        target_rank, target_disp, target_count,
                                        target_datatype, op, win, NULL);
#else
    int r;
    MPIDI_av_entry_t *av = NULL;

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)) && !MPIDI_CH4U_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIDI_SHM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win);
    else
        mpi_errno = MPIDI_NM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, op, win, NULL);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ACCUMULATE_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_compare_and_swap_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_compare_and_swap_unsafe(const void *origin_addr,
                                                           const void *compare_addr,
                                                           void *result_addr,
                                                           MPI_Datatype datatype,
                                                           int target_rank, MPI_Aint target_disp,
                                                           MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_COMPARE_AND_SWAP_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMPARE_AND_SWAP_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                              datatype, target_rank, target_disp, win, NULL);
#else
    int r;
    MPIDI_av_entry_t *av = NULL;

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)) && !MPIDI_CH4U_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIDI_SHM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                   datatype, target_rank, target_disp, win);
    else
        mpi_errno = MPIDI_NM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                  datatype, target_rank, target_disp, win, NULL);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMPARE_AND_SWAP_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_raccumulate_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_raccumulate_unsafe(const void *origin_addr,
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RACCUMULATE_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_RACCUMULATE_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                         target_rank, target_disp, target_count,
                                         target_datatype, op, win, NULL, request);
#else
    int r;
    MPIDI_av_entry_t *av = NULL;

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        /* create a completed request for user. */
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT(*request == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)) && !MPIDI_CH4U_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIDI_SHM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win, request);
    else
        mpi_errno = MPIDI_NM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win, NULL, request);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RACCUMULATE_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_rget_accumulate_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_rget_accumulate_unsafe(const void *origin_addr,
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RGET_ACCUMULATE_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_RGET_ACCUMULATE_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                             result_addr, result_count, result_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win, NULL, request);
#else
    int r;
    MPIDI_av_entry_t *av = NULL;

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        /* create a completed request for user. */
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT(*request == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)) && !MPIDI_CH4U_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIDI_SHM_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                  result_addr, result_count, result_datatype,
                                                  target_rank, target_disp, target_count,
                                                  target_datatype, op, win, request);
    else
        mpi_errno = MPIDI_NM_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                 result_addr, result_count, result_datatype,
                                                 target_rank, target_disp, target_count,
                                                 target_datatype, op, win, NULL, request);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RGET_ACCUMULATE_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_fetch_and_op_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_fetch_and_op_unsafe(const void *origin_addr,
                                                       void *result_addr,
                                                       MPI_Datatype datatype,
                                                       int target_rank,
                                                       MPI_Aint target_disp, MPI_Op op,
                                                       MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_FETCH_AND_OP_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_FETCH_AND_OP_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_fetch_and_op(origin_addr, result_addr,
                                          datatype, target_rank, target_disp, op, win, NULL);
#else
    int r;
    MPIDI_av_entry_t *av = NULL;

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)) && !MPIDI_CH4U_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIDI_SHM_mpi_fetch_and_op(origin_addr, result_addr,
                                               datatype, target_rank, target_disp, op, win);
    else
        mpi_errno = MPIDI_NM_mpi_fetch_and_op(origin_addr, result_addr,
                                              datatype, target_rank, target_disp, op, win, NULL);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_FETCH_AND_OP_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_rget_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_rget_unsafe(void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win,
                                               MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RGET_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_RGET_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count,
                                  target_datatype, win, NULL, request);
#else
    int r;
    MPIDI_av_entry_t *av = NULL;

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        /* create a completed request for user. */
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT(*request == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count,
                                       target_datatype, win, request);
    else
        mpi_errno = MPIDI_NM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, win, NULL, request);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RGET_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_rput_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_rput_unsafe(const void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win,
                                               MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RPUT_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_RPUT_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count,
                                  target_datatype, win, NULL, request);
#else
    int r;
    MPIDI_av_entry_t *av = NULL;

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        /* create a completed request for user. */
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT(*request == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count,
                                       target_datatype, win, request);
    else
        mpi_errno = MPIDI_NM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, win, NULL, request);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RPUT_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_get_accumulate_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_get_accumulate_unsafe(const void *origin_addr,
                                                         int origin_count,
                                                         MPI_Datatype origin_datatype,
                                                         void *result_addr,
                                                         int result_count,
                                                         MPI_Datatype result_datatype,
                                                         int target_rank,
                                                         MPI_Aint target_disp,
                                                         int target_count,
                                                         MPI_Datatype target_datatype, MPI_Op op,
                                                         MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GET_ACCUMULATE_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GET_ACCUMULATE_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                            result_addr, result_count, result_datatype,
                                            target_rank, target_disp, target_count, target_datatype,
                                            op, win, NULL);
#else
    int r;
    MPIDI_av_entry_t *av = NULL;

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)) && !MPIDI_CH4U_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIDI_SHM_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                 result_addr, result_count, result_datatype,
                                                 target_rank, target_disp, target_count,
                                                 target_datatype, op, win);
    else
        mpi_errno = MPIDI_NM_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                result_addr, result_count, result_datatype,
                                                target_rank, target_disp, target_count,
                                                target_datatype, op, win, NULL);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GET_ACCUMULATE_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_put_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_put_safe(const void *origin_addr,
                                            int origin_count,
                                            MPI_Datatype origin_datatype,
                                            int target_rank,
                                            MPI_Aint target_disp,
                                            int target_count, MPI_Datatype target_datatype,
                                            MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PUT_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PUT_SAFE);

    MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);

    if (!cs_acq) {
        MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
        MPIR_Datatype_add_ref_if_not_builtin(target_datatype);
        MPIDI_workq_rma_enqueue(PUT, origin_addr, origin_count, origin_datatype, NULL, NULL, 0,
                                MPI_DATATYPE_NULL, target_rank, target_disp, target_count,
                                target_datatype, MPI_OP_NULL, win, NULL, NULL);
    } else {
        MPIDI_workq_vni_progress_unsafe();
        mpi_errno = MPIDI_put_unsafe(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, win);
    }

  fn_exit:
    MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PUT_SAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_get_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_get_safe(void *origin_addr,
                                            int origin_count,
                                            MPI_Datatype origin_datatype,
                                            int target_rank,
                                            MPI_Aint target_disp,
                                            int target_count, MPI_Datatype target_datatype,
                                            MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GET_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GET_SAFE);

    MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);

    if (!cs_acq) {
        MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
        MPIR_Datatype_add_ref_if_not_builtin(target_datatype);
        /* For MPI_Get, "origin" has to be passed to the "result" parameter
         * field, because `origin_addr` is declared as `const void *`. */
        MPIDI_workq_rma_enqueue(GET, NULL, origin_count, origin_datatype, NULL, origin_addr, 0,
                                MPI_DATATYPE_NULL, target_rank, target_disp, target_count,
                                target_datatype, MPI_OP_NULL, win, NULL, NULL);
    } else {
        MPIDI_workq_vni_progress_unsafe();
        mpi_errno = MPIDI_get_unsafe(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, win);
    }

  fn_exit:
    MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GET_SAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_accumulate_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_accumulate_safe(const void *origin_addr,
                                                   int origin_count,
                                                   MPI_Datatype origin_datatype,
                                                   int target_rank,
                                                   MPI_Aint target_disp,
                                                   int target_count,
                                                   MPI_Datatype target_datatype, MPI_Op op,
                                                   MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ACCUMULATE_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ACCUMULATE_SAFE);

    MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    MPIDI_workq_vni_progress_unsafe();

    mpi_errno = MPIDI_accumulate_unsafe(origin_addr, origin_count, origin_datatype, target_rank,
                                        target_disp, target_count, target_datatype, op, win);

    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);

  fn_exit:
    MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ACCUMULATE_SAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_compare_and_swap_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_compare_and_swap_safe(const void *origin_addr,
                                                         const void *compare_addr,
                                                         void *result_addr,
                                                         MPI_Datatype datatype,
                                                         int target_rank, MPI_Aint target_disp,
                                                         MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_COMPARE_AND_SWAP_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMPARE_AND_SWAP_SAFE);

    MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    MPIDI_workq_vni_progress_unsafe();

    mpi_errno =
        MPIDI_compare_and_swap_unsafe(origin_addr, compare_addr, result_addr, datatype,
                                      target_rank, target_disp, win);

    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);

  fn_exit:
    MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMPARE_AND_SWAP_SAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_raccumulate_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_raccumulate_safe(const void *origin_addr,
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RACCUMULATE_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_RACCUMULATE_SAFE);

    MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    MPIDI_workq_vni_progress_unsafe();

    mpi_errno = MPIDI_raccumulate_unsafe(origin_addr, origin_count, origin_datatype, target_rank,
                                         target_disp, target_count, target_datatype, op, win,
                                         request);

    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RACCUMULATE_SAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_rget_accumulate_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_rget_accumulate_safe(const void *origin_addr,
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RGET_ACCUMULATE_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_RGET_ACCUMULATE_SAFE);

    MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    MPIDI_workq_vni_progress_unsafe();

    mpi_errno =
        MPIDI_rget_accumulate_unsafe(origin_addr, origin_count, origin_datatype, result_addr,
                                     result_count, result_datatype, target_rank, target_disp,
                                     target_count, target_datatype, op, win, request);

    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RGET_ACCUMULATE_SAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_fetch_and_op_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_fetch_and_op_safe(const void *origin_addr,
                                                     void *result_addr,
                                                     MPI_Datatype datatype,
                                                     int target_rank,
                                                     MPI_Aint target_disp, MPI_Op op,
                                                     MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_FETCH_AND_OP_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_FETCH_AND_OP_SAFE);

    MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    MPIDI_workq_vni_progress_unsafe();

    mpi_errno = MPIDI_fetch_and_op_unsafe(origin_addr, result_addr, datatype, target_rank,
                                          target_disp, op, win);

    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);

  fn_exit:
    MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_FETCH_AND_OP_SAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_rget_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_rget_safe(void *origin_addr,
                                             int origin_count,
                                             MPI_Datatype origin_datatype,
                                             int target_rank,
                                             MPI_Aint target_disp,
                                             int target_count,
                                             MPI_Datatype target_datatype, MPIR_Win * win,
                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RGET_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_RGET_SAFE);

    MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    MPIDI_workq_vni_progress_unsafe();

    mpi_errno =
        MPIDI_rget_unsafe(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                          target_count, target_datatype, win, request);

    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RGET_SAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_rput_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_rput_safe(const void *origin_addr,
                                             int origin_count,
                                             MPI_Datatype origin_datatype,
                                             int target_rank,
                                             MPI_Aint target_disp,
                                             int target_count,
                                             MPI_Datatype target_datatype, MPIR_Win * win,
                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_RPUT_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_RPUT_SAFE);

    MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    MPIDI_workq_vni_progress_unsafe();

    mpi_errno =
        MPIDI_rput_unsafe(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                          target_count, target_datatype, win, request);

    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_RPUT_SAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_get_accumulate_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_get_accumulate_safe(const void *origin_addr,
                                                       int origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       void *result_addr,
                                                       int result_count,
                                                       MPI_Datatype result_datatype,
                                                       int target_rank,
                                                       MPI_Aint target_disp,
                                                       int target_count,
                                                       MPI_Datatype target_datatype, MPI_Op op,
                                                       MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GET_ACCUMULATE_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GET_ACCUMULATE_SAFE);

    MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);
    MPIDI_workq_vni_progress_unsafe();

    mpi_errno = MPIDI_get_accumulate_unsafe(origin_addr, origin_count, origin_datatype, result_addr,
                                            result_count, result_datatype, target_rank, target_disp,
                                            target_count, target_datatype, op, win);

    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);

  fn_exit:
    MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GET_ACCUMULATE_SAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Put(const void *origin_addr,
                                      int origin_count,
                                      MPI_Datatype origin_datatype,
                                      int target_rank,
                                      MPI_Aint target_disp,
                                      int target_count, MPI_Datatype target_datatype,
                                      MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PUT);

    mpi_errno = MPIDI_put_safe(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count, target_datatype, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Get(void *origin_addr,
                                      int origin_count,
                                      MPI_Datatype origin_datatype,
                                      int target_rank,
                                      MPI_Aint target_disp,
                                      int target_count, MPI_Datatype target_datatype,
                                      MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET);

    mpi_errno = MPIDI_get_safe(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count, target_datatype, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Accumulate(const void *origin_addr,
                                             int origin_count,
                                             MPI_Datatype origin_datatype,
                                             int target_rank,
                                             MPI_Aint target_disp,
                                             int target_count,
                                             MPI_Datatype target_datatype, MPI_Op op,
                                             MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ACCUMULATE);

    mpi_errno = MPIDI_accumulate_safe(origin_addr, origin_count, origin_datatype, target_rank,
                                      target_disp, target_count, target_datatype, op, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Compare_and_swap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Compare_and_swap(const void *origin_addr,
                                                   const void *compare_addr,
                                                   void *result_addr,
                                                   MPI_Datatype datatype,
                                                   int target_rank, MPI_Aint target_disp,
                                                   MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMPARE_AND_SWAP);

    mpi_errno = MPIDI_compare_and_swap_safe(origin_addr, compare_addr, result_addr,
                                            datatype, target_rank, target_disp, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMPARE_AND_SWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Raccumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Raccumulate(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count,
                                              MPI_Datatype target_datatype,
                                              MPI_Op op, MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RACCUMULATE);

    mpi_errno = MPIDI_raccumulate_safe(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_disp, target_count, target_datatype, op, win,
                                       request);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Rget_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Rget_accumulate(const void *origin_addr,
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RGET_ACCUMULATE);

    mpi_errno = MPIDI_rget_accumulate_safe(origin_addr, origin_count, origin_datatype, result_addr,
                                           result_count, result_datatype, target_rank, target_disp,
                                           target_count, target_datatype, op, win, request);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RGET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Fetch_and_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Fetch_and_op(const void *origin_addr,
                                               void *result_addr,
                                               MPI_Datatype datatype,
                                               int target_rank,
                                               MPI_Aint target_disp, MPI_Op op, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_FETCH_AND_OP);

    mpi_errno = MPIDI_fetch_and_op_safe(origin_addr, result_addr,
                                        datatype, target_rank, target_disp, op, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_FETCH_AND_OP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Rget
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Rget(void *origin_addr,
                                       int origin_count,
                                       MPI_Datatype origin_datatype,
                                       int target_rank,
                                       MPI_Aint target_disp,
                                       int target_count,
                                       MPI_Datatype target_datatype, MPIR_Win * win,
                                       MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RGET);

    mpi_errno = MPIDI_rget_safe(origin_addr, origin_count, origin_datatype, target_rank,
                                target_disp, target_count, target_datatype, win, request);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RGET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Rput
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Rput(const void *origin_addr,
                                       int origin_count,
                                       MPI_Datatype origin_datatype,
                                       int target_rank,
                                       MPI_Aint target_disp,
                                       int target_count,
                                       MPI_Datatype target_datatype, MPIR_Win * win,
                                       MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RPUT);


    mpi_errno = MPIDI_rput_safe(origin_addr, origin_count, origin_datatype, target_rank,
                                target_disp, target_count, target_datatype, win, request);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RPUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Get_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Get_accumulate(const void *origin_addr,
                                                 int origin_count,
                                                 MPI_Datatype origin_datatype,
                                                 void *result_addr,
                                                 int result_count,
                                                 MPI_Datatype result_datatype,
                                                 int target_rank,
                                                 MPI_Aint target_disp,
                                                 int target_count,
                                                 MPI_Datatype target_datatype, MPI_Op op,
                                                 MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET_ACCUMULATE);

    mpi_errno = MPIDI_get_accumulate_safe(origin_addr, origin_count, origin_datatype,
                                          result_addr, result_count, result_datatype, target_rank,
                                          target_disp, target_count, target_datatype, op, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_RMA_H_INCLUDED */
