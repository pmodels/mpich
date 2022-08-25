/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_RMA_H_INCLUDED
#define CH4_RMA_H_INCLUDED

#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_dbg_dump_winattr(MPIDI_winattr_t winattr)
{
#ifndef CHECK_WINATTR
#define CHECK_WINATTR(attr, flag) ((attr) & flag ? 1 : 0)

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "winattr=0x%x (DIRECT_INTRA_COMM %d, SHM_ALLOCATED %d,"
                     "ACCU_NO_SHM %d, ACCU_SAME_OP_NO_OP %d, NM_REACHABLE %d, NM_DYNAMIC_MR %d)",
                     winattr, CHECK_WINATTR(winattr, MPIDI_WINATTR_DIRECT_INTRA_COMM),
                     CHECK_WINATTR(winattr, MPIDI_WINATTR_SHM_ALLOCATED),
                     CHECK_WINATTR(winattr, MPIDI_WINATTR_ACCU_NO_SHM),
                     CHECK_WINATTR(winattr, MPIDI_WINATTR_ACCU_SAME_OP_NO_OP),
                     CHECK_WINATTR(winattr, MPIDI_WINATTR_NM_REACHABLE),
                     CHECK_WINATTR(winattr, MPIDI_WINATTR_NM_DYNAMIC_MR)));
#undef CHECK_WINATTR
#endif
}

MPL_STATIC_INLINE_PREFIX int MPIDI_put(const void *origin_addr,
                                       MPI_Aint origin_count,
                                       MPI_Datatype origin_datatype,
                                       int target_rank,
                                       MPI_Aint target_disp,
                                       MPI_Aint target_count, MPI_Datatype target_datatype,
                                       MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_winattr_t winattr = MPIDI_WIN(win, winattr);
    MPIDI_av_entry_t *av = MPIDIU_win_rank_to_av(win, target_rank, winattr);
    MPIDI_dbg_dump_winattr(winattr);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_put(origin_addr, origin_count, origin_datatype,
                                 target_rank, target_disp, target_count, target_datatype, win, av,
                                 winattr);
#else
    if (MPIDI_av_is_local(av))
        mpi_errno = MPIDI_SHM_mpi_put(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count, target_datatype, win,
                                      winattr);
    else
        mpi_errno = MPIDI_NM_mpi_put(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, win,
                                     av, winattr);
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_get(void *origin_addr,
                                       MPI_Aint origin_count,
                                       MPI_Datatype origin_datatype,
                                       int target_rank,
                                       MPI_Aint target_disp,
                                       MPI_Aint target_count, MPI_Datatype target_datatype,
                                       MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_winattr_t winattr = MPIDI_WIN(win, winattr);
    MPIDI_av_entry_t *av = MPIDIU_win_rank_to_av(win, target_rank, winattr);
    MPIDI_dbg_dump_winattr(winattr);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_get(origin_addr, origin_count, origin_datatype,
                                 target_rank, target_disp, target_count, target_datatype, win, av,
                                 winattr);
#else
    if (MPIDI_av_is_local(av))
        mpi_errno = MPIDI_SHM_mpi_get(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count, target_datatype, win,
                                      winattr);
    else
        mpi_errno = MPIDI_NM_mpi_get(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, win,
                                     av, winattr);
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_accumulate(const void *origin_addr,
                                              MPI_Aint origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              MPI_Aint target_count,
                                              MPI_Datatype target_datatype, MPI_Op op,
                                              MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_winattr_t winattr = MPIDI_WIN(win, winattr);
    MPIDI_av_entry_t *av = MPIDIU_win_rank_to_av(win, target_rank, winattr);
    MPIDI_dbg_dump_winattr(winattr);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                        target_rank, target_disp, target_count,
                                        target_datatype, op, win, av, winattr);
#else
    if (MPIDI_av_is_local(av) && !(winattr & MPIDI_WINATTR_ACCU_NO_SHM))
        mpi_errno = MPIDI_SHM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win, winattr);
    else
        mpi_errno = MPIDI_NM_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, op, win, av, winattr);
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_compare_and_swap(const void *origin_addr,
                                                    const void *compare_addr,
                                                    void *result_addr,
                                                    MPI_Datatype datatype,
                                                    int target_rank, MPI_Aint target_disp,
                                                    MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_winattr_t winattr = MPIDI_WIN(win, winattr);
    MPIDI_av_entry_t *av = MPIDIU_win_rank_to_av(win, target_rank, winattr);
    MPIDI_dbg_dump_winattr(winattr);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                              datatype, target_rank, target_disp, win, av, winattr);
#else
    if (MPIDI_av_is_local(av) && !(winattr & MPIDI_WINATTR_ACCU_NO_SHM))
        mpi_errno = MPIDI_SHM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                   datatype, target_rank, target_disp, win,
                                                   winattr);
    else
        mpi_errno = MPIDI_NM_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                  datatype, target_rank, target_disp, win, av,
                                                  winattr);
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_raccumulate(const void *origin_addr,
                                               MPI_Aint origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               MPI_Aint target_count,
                                               MPI_Datatype target_datatype,
                                               MPI_Op op, MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_winattr_t winattr = MPIDI_WIN(win, winattr);
    MPIDI_av_entry_t *av = MPIDIU_win_rank_to_av(win, target_rank, winattr);
    MPIDI_dbg_dump_winattr(winattr);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                         target_rank, target_disp, target_count,
                                         target_datatype, op, win, av, winattr, request);
#else
    if (MPIDI_av_is_local(av) && !(winattr & MPIDI_WINATTR_ACCU_NO_SHM))
        mpi_errno = MPIDI_SHM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win, winattr, request);
    else
        mpi_errno = MPIDI_NM_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win, av, winattr, request);
#endif
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_rget_accumulate(const void *origin_addr,
                                                   MPI_Aint origin_count,
                                                   MPI_Datatype origin_datatype,
                                                   void *result_addr,
                                                   MPI_Aint result_count,
                                                   MPI_Datatype result_datatype,
                                                   int target_rank,
                                                   MPI_Aint target_disp,
                                                   MPI_Aint target_count,
                                                   MPI_Datatype target_datatype,
                                                   MPI_Op op, MPIR_Win * win,
                                                   MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_winattr_t winattr = MPIDI_WIN(win, winattr);
    MPIDI_av_entry_t *av = MPIDIU_win_rank_to_av(win, target_rank, winattr);
    MPIDI_dbg_dump_winattr(winattr);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                             result_addr, result_count, result_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win, av, winattr, request);
#else
    if (MPIDI_av_is_local(av) && !(winattr & MPIDI_WINATTR_ACCU_NO_SHM))
        mpi_errno = MPIDI_SHM_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                  result_addr, result_count, result_datatype,
                                                  target_rank, target_disp, target_count,
                                                  target_datatype, op, win, winattr, request);
    else
        mpi_errno = MPIDI_NM_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                 result_addr, result_count, result_datatype,
                                                 target_rank, target_disp, target_count,
                                                 target_datatype, op, win, av, winattr, request);
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_fetch_and_op(const void *origin_addr,
                                                void *result_addr,
                                                MPI_Datatype datatype,
                                                int target_rank,
                                                MPI_Aint target_disp, MPI_Op op, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_winattr_t winattr = MPIDI_WIN(win, winattr);
    MPIDI_av_entry_t *av = MPIDIU_win_rank_to_av(win, target_rank, winattr);
    MPIDI_dbg_dump_winattr(winattr);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_fetch_and_op(origin_addr, result_addr,
                                          datatype, target_rank, target_disp, op, win, av, winattr);
#else
    if (MPIDI_av_is_local(av) && !(winattr & MPIDI_WINATTR_ACCU_NO_SHM))
        mpi_errno = MPIDI_SHM_mpi_fetch_and_op(origin_addr, result_addr,
                                               datatype, target_rank, target_disp, op, win,
                                               winattr);
    else
        mpi_errno = MPIDI_NM_mpi_fetch_and_op(origin_addr, result_addr,
                                              datatype, target_rank, target_disp, op, win, av,
                                              winattr);
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_rget(void *origin_addr,
                                        MPI_Aint origin_count,
                                        MPI_Datatype origin_datatype,
                                        int target_rank,
                                        MPI_Aint target_disp,
                                        MPI_Aint target_count,
                                        MPI_Datatype target_datatype, MPIR_Win * win,
                                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_winattr_t winattr = MPIDI_WIN(win, winattr);
    MPIDI_av_entry_t *av = MPIDIU_win_rank_to_av(win, target_rank, winattr);
    MPIDI_dbg_dump_winattr(winattr);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count,
                                  target_datatype, win, av, winattr, request);
#else
    if (MPIDI_av_is_local(av))
        mpi_errno = MPIDI_SHM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count,
                                       target_datatype, win, winattr, request);
    else
        mpi_errno = MPIDI_NM_mpi_rget(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, win, av, winattr, request);
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_rput(const void *origin_addr,
                                        MPI_Aint origin_count,
                                        MPI_Datatype origin_datatype,
                                        int target_rank,
                                        MPI_Aint target_disp,
                                        MPI_Aint target_count,
                                        MPI_Datatype target_datatype, MPIR_Win * win,
                                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_winattr_t winattr = MPIDI_WIN(win, winattr);
    MPIDI_av_entry_t *av = MPIDIU_win_rank_to_av(win, target_rank, winattr);
    MPIDI_dbg_dump_winattr(winattr);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count,
                                  target_datatype, win, av, winattr, request);
#else
    if (MPIDI_av_is_local(av))
        mpi_errno = MPIDI_SHM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                       target_rank, target_disp, target_count,
                                       target_datatype, win, winattr, request);
    else
        mpi_errno = MPIDI_NM_mpi_rput(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, win, av, winattr, request);
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_get_accumulate(const void *origin_addr,
                                                  MPI_Aint origin_count,
                                                  MPI_Datatype origin_datatype,
                                                  void *result_addr,
                                                  MPI_Aint result_count,
                                                  MPI_Datatype result_datatype,
                                                  int target_rank,
                                                  MPI_Aint target_disp,
                                                  MPI_Aint target_count,
                                                  MPI_Datatype target_datatype, MPI_Op op,
                                                  MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_winattr_t winattr = MPIDI_WIN(win, winattr);
    MPIDI_av_entry_t *av = MPIDIU_win_rank_to_av(win, target_rank, winattr);
    MPIDI_dbg_dump_winattr(winattr);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                            result_addr, result_count, result_datatype,
                                            target_rank, target_disp, target_count, target_datatype,
                                            op, win, av, winattr);
#else
    if (MPIDI_av_is_local(av) && !(winattr & MPIDI_WINATTR_ACCU_NO_SHM))
        mpi_errno = MPIDI_SHM_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                 result_addr, result_count, result_datatype,
                                                 target_rank, target_disp, target_count,
                                                 target_datatype, op, win, winattr);
    else
        mpi_errno = MPIDI_NM_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                result_addr, result_count, result_datatype,
                                                target_rank, target_disp, target_count,
                                                target_datatype, op, win, av, winattr);
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPID_Put(const void *origin_addr,
                                      MPI_Aint origin_count,
                                      MPI_Datatype origin_datatype,
                                      int target_rank,
                                      MPI_Aint target_disp,
                                      MPI_Aint target_count, MPI_Datatype target_datatype,
                                      MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_put(origin_addr, origin_count, origin_datatype,
                          target_rank, target_disp, target_count, target_datatype, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Get(void *origin_addr,
                                      MPI_Aint origin_count,
                                      MPI_Datatype origin_datatype,
                                      int target_rank,
                                      MPI_Aint target_disp,
                                      MPI_Aint target_count, MPI_Datatype target_datatype,
                                      MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_get(origin_addr, origin_count, origin_datatype,
                          target_rank, target_disp, target_count, target_datatype, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Accumulate(const void *origin_addr,
                                             MPI_Aint origin_count,
                                             MPI_Datatype origin_datatype,
                                             int target_rank,
                                             MPI_Aint target_disp,
                                             MPI_Aint target_count,
                                             MPI_Datatype target_datatype, MPI_Op op,
                                             MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                 target_disp, target_count, target_datatype, op, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
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
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_compare_and_swap(origin_addr, compare_addr, result_addr,
                                       datatype, target_rank, target_disp, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Raccumulate(const void *origin_addr,
                                              MPI_Aint origin_count,
                                              MPI_Datatype origin_datatype,
                                              int target_rank,
                                              MPI_Aint target_disp,
                                              MPI_Aint target_count,
                                              MPI_Datatype target_datatype,
                                              MPI_Op op, MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_raccumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                  target_disp, target_count, target_datatype, op, win, request);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Rget_accumulate(const void *origin_addr,
                                                  MPI_Aint origin_count,
                                                  MPI_Datatype origin_datatype,
                                                  void *result_addr,
                                                  MPI_Aint result_count,
                                                  MPI_Datatype result_datatype,
                                                  int target_rank,
                                                  MPI_Aint target_disp,
                                                  MPI_Aint target_count,
                                                  MPI_Datatype target_datatype,
                                                  MPI_Op op, MPIR_Win * win,
                                                  MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_rget_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                      result_count, result_datatype, target_rank, target_disp,
                                      target_count, target_datatype, op, win, request);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
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
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_fetch_and_op(origin_addr, result_addr,
                                   datatype, target_rank, target_disp, op, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPID_Rget(void *origin_addr,
                                       MPI_Aint origin_count,
                                       MPI_Datatype origin_datatype,
                                       int target_rank,
                                       MPI_Aint target_disp,
                                       MPI_Aint target_count,
                                       MPI_Datatype target_datatype, MPIR_Win * win,
                                       MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_rget(origin_addr, origin_count, origin_datatype, target_rank,
                           target_disp, target_count, target_datatype, win, request);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Rput(const void *origin_addr,
                                       MPI_Aint origin_count,
                                       MPI_Datatype origin_datatype,
                                       int target_rank,
                                       MPI_Aint target_disp,
                                       MPI_Aint target_count,
                                       MPI_Datatype target_datatype, MPIR_Win * win,
                                       MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;


    mpi_errno = MPIDI_rput(origin_addr, origin_count, origin_datatype, target_rank,
                           target_disp, target_count, target_datatype, win, request);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Get_accumulate(const void *origin_addr,
                                                 MPI_Aint origin_count,
                                                 MPI_Datatype origin_datatype,
                                                 void *result_addr,
                                                 MPI_Aint result_count,
                                                 MPI_Datatype result_datatype,
                                                 int target_rank,
                                                 MPI_Aint target_disp,
                                                 MPI_Aint target_count,
                                                 MPI_Datatype target_datatype, MPI_Op op,
                                                 MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_get_accumulate(origin_addr, origin_count, origin_datatype,
                                     result_addr, result_count, result_datatype, target_rank,
                                     target_disp, target_count, target_datatype, op, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_RMA_H_INCLUDED */
