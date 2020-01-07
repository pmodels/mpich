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

MPL_STATIC_INLINE_PREFIX int MPIDI_put_unsafe(const void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_put(origin_addr, origin_count, origin_datatype,
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
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_get_unsafe(void *origin_addr,
                                              int origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              int target_count, MPI_Datatype target_datatype,
                                              MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_get(origin_addr, origin_count, origin_datatype,
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
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                        target_rank, target_disp, target_count,
                                        target_datatype, op, win, av);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)) && !MPIDIG_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIDI_SHM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win);
    else
        mpi_errno = MPIDI_NM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, op, win, av);
#endif
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_compare_and_swap_unsafe(const void *origin_addr,
                                                           const void *compare_addr,
                                                           void *result_addr,
                                                           MPI_Datatype datatype,
                                                           int target_rank, MPI_Aint target_disp,
                                                           MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                              datatype, target_rank, target_disp, win, av);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)) && !MPIDIG_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIDI_SHM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                   datatype, target_rank, target_disp, win);
    else
        mpi_errno = MPIDI_NM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                  datatype, target_rank, target_disp, win, av);
#endif
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                         target_rank, target_disp, target_count,
                                         target_datatype, op, win, av, request);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)) && !MPIDIG_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIDI_SHM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win, request);
    else
        mpi_errno = MPIDI_NM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win, av, request);
#endif
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                             result_addr, result_count, result_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win, av, request);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)) && !MPIDIG_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIDI_SHM_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                  result_addr, result_count, result_datatype,
                                                  target_rank, target_disp, target_count,
                                                  target_datatype, op, win, request);
    else
        mpi_errno = MPIDI_NM_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                 result_addr, result_count, result_datatype,
                                                 target_rank, target_disp, target_count,
                                                 target_datatype, op, win, av, request);
#endif
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_fetch_and_op_unsafe(const void *origin_addr,
                                                       void *result_addr,
                                                       MPI_Datatype datatype,
                                                       int target_rank,
                                                       MPI_Aint target_disp, MPI_Op op,
                                                       MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_fetch_and_op(origin_addr, result_addr,
                                          datatype, target_rank, target_disp, op, win, av);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)) && !MPIDIG_WIN(win, info_args).disable_shm_accumulate)
        mpi_errno = MPIDI_SHM_mpi_fetch_and_op(origin_addr, result_addr,
                                               datatype, target_rank, target_disp, op, win);
    else
        mpi_errno = MPIDI_NM_mpi_fetch_and_op(origin_addr, result_addr,
                                              datatype, target_rank, target_disp, op, win, av);
#endif
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count,
                                  target_datatype, win, av, request);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count,
                                       target_datatype, win, request);
    else
        mpi_errno = MPIDI_NM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, win, av, request);
#endif
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count,
                                  target_datatype, win, av, request);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count,
                                       target_datatype, win, request);
    else
        mpi_errno = MPIDI_NM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, win, av, request);
#endif
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                            result_addr, result_count, result_datatype,
                                            target_rank, target_disp, target_count, target_datatype,
                                            op, win, av);
#else
    int r;

    if ((r = MPIDI_av_is_local(av)) && !MPIDIG_WIN(win, info_args).disable_shm_accumulate)
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
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_put_safe(const void *origin_addr,
                                            int origin_count,
                                            MPI_Datatype origin_datatype,
                                            int target_rank,
                                            MPI_Aint target_disp,
                                            int target_count, MPI_Datatype target_datatype,
                                            MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;

    MPID_THREAD_SAFE_BEGIN(VCI, MPIDI_global.vci_lock, cs_acq);

    if (!cs_acq) {
        MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
        MPIR_Datatype_add_ref_if_not_builtin(target_datatype);
        MPIDI_workq_rma_enqueue(PUT, origin_addr, origin_count, origin_datatype, NULL, NULL, 0,
                                MPI_DATATYPE_NULL, target_rank, target_disp, target_count,
                                target_datatype, MPI_OP_NULL, win, NULL, NULL);
    } else {
        MPIDI_workq_vci_progress_unsafe();
        mpi_errno = MPIDI_put_unsafe(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, win);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPID_THREAD_SAFE_END(VCI, MPIDI_global.vci_lock, cs_acq);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_get_safe(void *origin_addr,
                                            int origin_count,
                                            MPI_Datatype origin_datatype,
                                            int target_rank,
                                            MPI_Aint target_disp,
                                            int target_count, MPI_Datatype target_datatype,
                                            MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;

    MPID_THREAD_SAFE_BEGIN(VCI, MPIDI_global.vci_lock, cs_acq);

    if (!cs_acq) {
        MPIR_Datatype_add_ref_if_not_builtin(origin_datatype);
        MPIR_Datatype_add_ref_if_not_builtin(target_datatype);
        /* For MPI_Get, "origin" has to be passed to the "result" parameter
         * field, because `origin_addr` is declared as `const void *`. */
        MPIDI_workq_rma_enqueue(GET, NULL, origin_count, origin_datatype, NULL, origin_addr, 0,
                                MPI_DATATYPE_NULL, target_rank, target_disp, target_count,
                                target_datatype, MPI_OP_NULL, win, NULL, NULL);
    } else {
        MPIDI_workq_vci_progress_unsafe();
        mpi_errno = MPIDI_get_unsafe(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, win);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPID_THREAD_SAFE_END(VCI, MPIDI_global.vci_lock, cs_acq);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

    mpi_errno = MPIDI_accumulate_unsafe(origin_addr, origin_count, origin_datatype, target_rank,
                                        target_disp, target_count, target_datatype, op, win);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPID_THREAD_SAFE_END(VCI, MPIDI_global.vci_lock, cs_acq);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_compare_and_swap_safe(const void *origin_addr,
                                                         const void *compare_addr,
                                                         void *result_addr,
                                                         MPI_Datatype datatype,
                                                         int target_rank, MPI_Aint target_disp,
                                                         MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

    mpi_errno =
        MPIDI_compare_and_swap_unsafe(origin_addr, compare_addr, result_addr, datatype,
                                      target_rank, target_disp, win);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPID_THREAD_SAFE_END(VCI, MPIDI_global.vci_lock, cs_acq);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

    mpi_errno = MPIDI_raccumulate_unsafe(origin_addr, origin_count, origin_datatype, target_rank,
                                         target_disp, target_count, target_datatype, op, win,
                                         request);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

    mpi_errno =
        MPIDI_rget_accumulate_unsafe(origin_addr, origin_count, origin_datatype, result_addr,
                                     result_count, result_datatype, target_rank, target_disp,
                                     target_count, target_datatype, op, win, request);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_fetch_and_op_safe(const void *origin_addr,
                                                     void *result_addr,
                                                     MPI_Datatype datatype,
                                                     int target_rank,
                                                     MPI_Aint target_disp, MPI_Op op,
                                                     MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

    mpi_errno = MPIDI_fetch_and_op_unsafe(origin_addr, result_addr, datatype, target_rank,
                                          target_disp, op, win);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPID_THREAD_SAFE_END(VCI, MPIDI_global.vci_lock, cs_acq);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

    mpi_errno =
        MPIDI_rget_unsafe(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                          target_count, target_datatype, win, request);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

    mpi_errno =
        MPIDI_rput_unsafe(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                          target_count, target_datatype, win, request);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
    MPIDI_workq_vci_progress_unsafe();

    mpi_errno = MPIDI_get_accumulate_unsafe(origin_addr, origin_count, origin_datatype, result_addr,
                                            result_count, result_datatype, target_rank, target_disp,
                                            target_count, target_datatype, op, win);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Put(const void *origin_addr,
                                      int origin_count,
                                      MPI_Datatype origin_datatype,
                                      int target_rank,
                                      MPI_Aint target_disp,
                                      int target_count, MPI_Datatype target_datatype,
                                      MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_put_safe(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count, target_datatype, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Get(void *origin_addr,
                                      int origin_count,
                                      MPI_Datatype origin_datatype,
                                      int target_rank,
                                      MPI_Aint target_disp,
                                      int target_count, MPI_Datatype target_datatype,
                                      MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_get_safe(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count, target_datatype, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    mpi_errno = MPIDI_accumulate_safe(origin_addr, origin_count, origin_datatype, target_rank,
                                      target_disp, target_count, target_datatype, op, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Compare_and_swap(const void *origin_addr,
                                                   const void *compare_addr,
                                                   void *result_addr,
                                                   MPI_Datatype datatype,
                                                   int target_rank, MPI_Aint target_disp,
                                                   MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_compare_and_swap_safe(origin_addr, compare_addr, result_addr,
                                            datatype, target_rank, target_disp, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    mpi_errno = MPIDI_raccumulate_safe(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_disp, target_count, target_datatype, op, win,
                                       request);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    mpi_errno = MPIDI_rget_accumulate_safe(origin_addr, origin_count, origin_datatype, result_addr,
                                           result_count, result_datatype, target_rank, target_disp,
                                           target_count, target_datatype, op, win, request);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Fetch_and_op(const void *origin_addr,
                                               void *result_addr,
                                               MPI_Datatype datatype,
                                               int target_rank,
                                               MPI_Aint target_disp, MPI_Op op, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_fetch_and_op_safe(origin_addr, result_addr,
                                        datatype, target_rank, target_disp, op, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    mpi_errno = MPIDI_rget_safe(origin_addr, origin_count, origin_datatype, target_rank,
                                target_disp, target_count, target_datatype, win, request);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    mpi_errno = MPIDI_rput_safe(origin_addr, origin_count, origin_datatype, target_rank,
                                target_disp, target_count, target_datatype, win, request);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    mpi_errno = MPIDI_get_accumulate_safe(origin_addr, origin_count, origin_datatype,
                                          result_addr, result_count, result_datatype, target_rank,
                                          target_disp, target_count, target_datatype, op, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_RMA_H_INCLUDED */
