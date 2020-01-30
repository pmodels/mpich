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

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_put(const void *origin_addr, int origin_count,
                                            MPI_Datatype origin_datatype, int target_rank,
                                            MPI_Aint target_disp, int target_count,
                                            MPI_Datatype target_datatype, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_PUT);

    mpi_errno = MPIDIG_do_put(origin_addr, origin_count, origin_datatype,
                              target_rank, target_disp, target_count, target_datatype, win, NULL);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_rput(const void *origin_addr, int origin_count,
                                             MPI_Datatype origin_datatype, int target_rank,
                                             MPI_Aint target_disp, int target_count,
                                             MPI_Datatype target_datatype, MPIR_Win * win,
                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_RPUT);

    mpi_errno = MPIDIG_do_put(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                              target_count, target_datatype, win, &sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    *request = sreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RPUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_get(void *origin_addr, int origin_count,
                                            MPI_Datatype origin_datatype, int target_rank,
                                            MPI_Aint target_disp, int target_count,
                                            MPI_Datatype target_datatype, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_GET);
    mpi_errno = MPIDIG_do_get(origin_addr, origin_count, origin_datatype,
                              target_rank, target_disp, target_count, target_datatype, win, NULL);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_rget(void *origin_addr, int origin_count,
                                             MPI_Datatype origin_datatype, int target_rank,
                                             MPI_Aint target_disp, int target_count,
                                             MPI_Datatype target_datatype, MPIR_Win * win,
                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_RGET);

    mpi_errno = MPIDIG_do_get(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                              target_count, target_datatype, win, &sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    *request = sreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RGET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_raccumulate(const void *origin_addr, int origin_count,
                                                    MPI_Datatype origin_datatype, int target_rank,
                                                    MPI_Aint target_disp, int target_count,
                                                    MPI_Datatype target_datatype, MPI_Op op,
                                                    MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_RACCUMULATE);

    mpi_errno = MPIDIG_do_accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                     target_disp, target_count, target_datatype, op, win, &sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    *request = sreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_accumulate(const void *origin_addr, int origin_count,
                                                   MPI_Datatype origin_datatype, int target_rank,
                                                   MPI_Aint target_disp, int target_count,
                                                   MPI_Datatype target_datatype, MPI_Op op,
                                                   MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_ACCUMULATE);

    mpi_errno = MPIDIG_do_accumulate(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count, target_datatype, op,
                                     win, NULL);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_rget_accumulate(const void *origin_addr,
                                                        int origin_count,
                                                        MPI_Datatype origin_datatype,
                                                        void *result_addr, int result_count,
                                                        MPI_Datatype result_datatype,
                                                        int target_rank, MPI_Aint target_disp,
                                                        int target_count,
                                                        MPI_Datatype target_datatype, MPI_Op op,
                                                        MPIR_Win * win, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_RGET_ACCUMULATE);

    mpi_errno = MPIDIG_do_get_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                         result_count, result_datatype, target_rank, target_disp,
                                         target_count, target_datatype, op, win, &sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    *request = sreq;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RGET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_get_accumulate(const void *origin_addr,
                                                       int origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       void *result_addr, int result_count,
                                                       MPI_Datatype result_datatype,
                                                       int target_rank, MPI_Aint target_disp,
                                                       int target_count,
                                                       MPI_Datatype target_datatype,
                                                       MPI_Op op, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_GET_ACCUMULATE);

    mpi_errno = MPIDIG_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                         result_addr, result_count, result_datatype,
                                         target_rank, target_disp, target_count, target_datatype,
                                         op, win, NULL);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_GET_ACCUMULATE);
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
    return MPIDIG_do_cswap(origin_addr, compare_addr, result_addr, datatype,
                           target_rank, target_disp, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_fetch_and_op(const void *origin_addr, void *result_addr,
                                                     MPI_Datatype datatype, int target_rank,
                                                     MPI_Aint target_disp, MPI_Op op,
                                                     MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_FETCH_AND_OP);

    mpi_errno = MPIDIG_mpi_get_accumulate(origin_addr, 1, datatype, result_addr, 1, datatype,
                                          target_rank, target_disp, 1, datatype, op, win);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_FETCH_AND_OP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4R_RMA_H_INCLUDED */
