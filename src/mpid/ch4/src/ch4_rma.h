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
#define FUNCNAME MPIDI_Put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
__CH4_INLINE__ int MPIDI_Put(const void *origin_addr,
                             int origin_count,
                             MPI_Datatype origin_datatype,
                             int target_rank,
                             MPI_Aint target_disp,
                             int target_count, MPI_Datatype target_datatype, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_PUT);
    mpi_errno = MPIDI_NM_put(origin_addr, origin_count, origin_datatype,
                             target_rank, target_disp, target_count, target_datatype, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
__CH4_INLINE__ int MPIDI_Get(void *origin_addr,
                             int origin_count,
                             MPI_Datatype origin_datatype,
                             int target_rank,
                             MPI_Aint target_disp,
                             int target_count, MPI_Datatype target_datatype, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_GET);
    mpi_errno = MPIDI_NM_get(origin_addr, origin_count, origin_datatype,
                             target_rank, target_disp, target_count, target_datatype, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
__CH4_INLINE__ int MPIDI_Accumulate(const void *origin_addr,
                                    int origin_count,
                                    MPI_Datatype origin_datatype,
                                    int target_rank,
                                    MPI_Aint target_disp,
                                    int target_count,
                                    MPI_Datatype target_datatype, MPI_Op op, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_ACCUMULATE);
    mpi_errno = MPIDI_NM_accumulate(origin_addr, origin_count, origin_datatype,
                                    target_rank, target_disp, target_count,
                                    target_datatype, op, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Compare_and_swap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
__CH4_INLINE__ int MPIDI_Compare_and_swap(const void *origin_addr,
                                          const void *compare_addr,
                                          void *result_addr,
                                          MPI_Datatype datatype,
                                          int target_rank, MPI_Aint target_disp, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_COMPARE_AND_SWAP);
    mpi_errno = MPIDI_NM_compare_and_swap(origin_addr, compare_addr, result_addr,
                                          datatype, target_rank, target_disp, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_COMPARE_AND_SWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Raccumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
__CH4_INLINE__ int MPIDI_Raccumulate(const void *origin_addr,
                                     int origin_count,
                                     MPI_Datatype origin_datatype,
                                     int target_rank,
                                     MPI_Aint target_disp,
                                     int target_count,
                                     MPI_Datatype target_datatype,
                                     MPI_Op op, MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_RACCUMULATE);
    mpi_errno = MPIDI_NM_raccumulate(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count,
                                     target_datatype, op, win, request);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_RACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Rget_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
__CH4_INLINE__ int MPIDI_Rget_accumulate(const void *origin_addr,
                                         int origin_count,
                                         MPI_Datatype origin_datatype,
                                         void *result_addr,
                                         int result_count,
                                         MPI_Datatype result_datatype,
                                         int target_rank,
                                         MPI_Aint target_disp,
                                         int target_count,
                                         MPI_Datatype target_datatype,
                                         MPI_Op op, MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_RGET_ACCUMULATE);
    mpi_errno = MPIDI_NM_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                         result_addr, result_count, result_datatype,
                                         target_rank, target_disp, target_count,
                                         target_datatype, op, win, request);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_RGET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Fetch_and_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
__CH4_INLINE__ int MPIDI_Fetch_and_op(const void *origin_addr,
                                      void *result_addr,
                                      MPI_Datatype datatype,
                                      int target_rank,
                                      MPI_Aint target_disp, MPI_Op op, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_FETCH_AND_OP);
    mpi_errno = MPIDI_NM_fetch_and_op(origin_addr, result_addr,
                                      datatype, target_rank, target_disp, op, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_FETCH_AND_OP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Rget
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
__CH4_INLINE__ int MPIDI_Rget(void *origin_addr,
                              int origin_count,
                              MPI_Datatype origin_datatype,
                              int target_rank,
                              MPI_Aint target_disp,
                              int target_count,
                              MPI_Datatype target_datatype, MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_RGET);
    mpi_errno = MPIDI_NM_rget(origin_addr, origin_count, origin_datatype,
                              target_rank, target_disp, target_count,
                              target_datatype, win, request);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_RGET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Rput
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
__CH4_INLINE__ int MPIDI_Rput(const void *origin_addr,
                              int origin_count,
                              MPI_Datatype origin_datatype,
                              int target_rank,
                              MPI_Aint target_disp,
                              int target_count,
                              MPI_Datatype target_datatype, MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_RPUT);
    mpi_errno = MPIDI_NM_rput(origin_addr, origin_count, origin_datatype,
                              target_rank, target_disp, target_count,
                              target_datatype, win, request);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_RPUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Get_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
__CH4_INLINE__ int MPIDI_Get_accumulate(const void *origin_addr,
                                        int origin_count,
                                        MPI_Datatype origin_datatype,
                                        void *result_addr,
                                        int result_count,
                                        MPI_Datatype result_datatype,
                                        int target_rank,
                                        MPI_Aint target_disp,
                                        int target_count,
                                        MPI_Datatype target_datatype, MPI_Op op, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4_GET_ACCUMULATE);
    mpi_errno = MPIDI_NM_get_accumulate(origin_addr, origin_count, origin_datatype,
                                        result_addr, result_count, result_datatype,
                                        target_rank, target_disp, target_count, target_datatype,
                                        op, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4_GET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_RMA_H_INCLUDED */
