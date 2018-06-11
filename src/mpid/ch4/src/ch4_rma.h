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
#define FUNCNAME MPIDI_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_put(int transport,
                                       const void *origin_addr,
                                       int origin_count,
                                       MPI_Datatype origin_datatype,
                                       int target_rank,
                                       MPI_Aint target_disp,
                                       int target_count, MPI_Datatype target_datatype,
                                       MPIR_Win * win, MPIDI_av_entry_t * av)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PUT);

    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
            MPIR_Datatype_add_ref_if_not_builtin(target_datatype);
            /* Increase enq_cnts for RMA communication operations. */
            OPA_incr_int(&MPIDI_CH4U_WIN(win, local_enq_cnts));
            /* result addr/count/datatye is used for Get */
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx),
                                    PUT, origin_addr, origin_count, origin_datatype, NULL, NULL, 0,
                                    MPI_DATATYPE_NULL, target_rank, target_disp, target_count,
                                    target_datatype, MPI_OP_NULL, NULL, 0, 0, win, av, NULL);
            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_put(origin_addr, origin_count, origin_datatype,
                                         target_rank, target_disp, target_count, target_datatype,
                                         win, av);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PUT);
    return mpi_errno;
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
    MPIDI_av_entry_t *av;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PUT);

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_put(MPIDI_CH4R_NETMOD, origin_addr, origin_count, origin_datatype,
                          target_rank, target_disp, target_count, target_datatype, win, av);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_put(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count, target_datatype, win);
    else
        mpi_errno = MPIDI_NM_mpi_put(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, win,
                                     av);
#endif
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
#define FUNCNAME MPIDI_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_get(int transport,
                                       void *origin_addr,
                                       int origin_count,
                                       MPI_Datatype origin_datatype,
                                       int target_rank,
                                       MPI_Aint target_disp,
                                       int target_count, MPI_Datatype target_datatype,
                                       MPIR_Win * win, MPIDI_av_entry_t * av)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GET);

    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
            MPIR_Datatype_add_ref_if_not_builtin(target_datatype);
            /* Increase enq_cnts for RMA communication operations. */
            OPA_incr_int(&MPIDI_CH4U_WIN(win, local_enq_cnts));
            /* result addr/count/datatye is used for enqueuing Get because origin_addr in the
             * enqueue function is defined as const void *. */
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx),
                                    GET, NULL, 0, MPI_DATATYPE_NULL, NULL, origin_addr,
                                    origin_count, origin_datatype, target_rank, target_disp,
                                    target_count, target_datatype, MPI_OP_NULL, NULL, 0, 0, win, av,
                                    NULL);
            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_get(origin_addr, origin_count, origin_datatype,
                                         target_rank, target_disp, target_count, target_datatype,
                                         win, av);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GET);
    return mpi_errno;
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
    MPIDI_av_entry_t *av;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET);

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_get(MPIDI_CH4R_NETMOD, origin_addr, origin_count, origin_datatype,
                          target_rank, target_disp, target_count, target_datatype, win, av);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_get(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count, target_datatype, win);
    else
        mpi_errno = MPIDI_NM_mpi_get(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, win,
                                     av);
#endif
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
    MPIDI_av_entry_t *av;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ACCUMULATE);

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                        target_rank, target_disp, target_count,
                                        target_datatype, op, win, av);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win);
    else
        mpi_errno = MPIDI_NM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, op, win, av);
#endif
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
    MPIDI_av_entry_t *av;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMPARE_AND_SWAP);

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                              datatype, target_rank, target_disp, win, av);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                   datatype, target_rank, target_disp, win);
    else
        mpi_errno = MPIDI_NM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                  datatype, target_rank, target_disp, win, av);
#endif
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
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RACCUMULATE);

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        /* create a completed request for user. */
        MPIR_Request *sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Request_add_ref(sreq);
        MPID_Request_complete(sreq);
        *request = sreq;
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                         target_rank, target_disp, target_count,
                                         target_datatype, op, win, av, request);
#else
    int r;

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win, request);
    else
        mpi_errno = MPIDI_NM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win, av, request);
#endif
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
    MPIDI_av_entry_t *av;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RGET_ACCUMULATE);

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        /* create a completed request for user. */
        MPIR_Request *sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Request_add_ref(sreq);
        MPID_Request_complete(sreq);
        *request = sreq;
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                             result_addr, result_count, result_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win, av, request);
#else
    int r;

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)))
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
    MPIDI_av_entry_t *av;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_FETCH_AND_OP);

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_fetch_and_op(origin_addr, result_addr,
                                          datatype, target_rank, target_disp, op, win, av);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_fetch_and_op(origin_addr, result_addr,
                                               datatype, target_rank, target_disp, op, win);
    else
        mpi_errno = MPIDI_NM_mpi_fetch_and_op(origin_addr, result_addr,
                                              datatype, target_rank, target_disp, op, win, av);
#endif
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
    MPIDI_av_entry_t *av;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RGET);

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        /* create a completed request for user. */
        MPIR_Request *sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Request_add_ref(sreq);
        MPID_Request_complete(sreq);
        *request = sreq;
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count,
                                  target_datatype, win, av, request);
#else
    int r;

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count,
                                       target_datatype, win, request);
    else
        mpi_errno = MPIDI_NM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, win, av, request);
#endif
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
    MPIDI_av_entry_t *av;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RPUT);

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        /* create a completed request for user. */
        MPIR_Request *sreq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Request_add_ref(sreq);
        MPID_Request_complete(sreq);
        *request = sreq;
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count,
                                  target_datatype, win, av, request);
#else
    int r;

    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count,
                                       target_datatype, win, request);
    else
        mpi_errno = MPIDI_NM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, win, av, request);
#endif
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
    MPIDI_av_entry_t *av;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET_ACCUMULATE);

    if (unlikely(target_rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }
    av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                            result_addr, result_count, result_datatype,
                                            target_rank, target_disp, target_count, target_datatype,
                                            op, win, av);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                 result_addr, result_count, result_datatype,
                                                 target_rank, target_disp, target_count,
                                                 target_datatype, op, win);
    else
        mpi_errno = MPIDI_NM_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                result_addr, result_count, result_datatype,
                                                target_rank, target_disp, target_count,
                                                target_datatype, op, win, av);
#endif
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
