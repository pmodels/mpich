/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef SHM_RMA_H_INCLUDED
#define SHM_RMA_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_shared_query(MPIR_Win * win, int rank,
                                                            MPI_Aint * size, int *disp_unit,
                                                            void *baseptr)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_put(const void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_put(origin_addr, origin_count, origin_datatype, target_rank,
                              target_disp, target_count, target_datatype, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_start(group, assert, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_complete(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_complete(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_post(group, assert, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_wait(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_wait(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_test(MPIR_Win * win, int *flag)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_test(win, flag);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_lock(int lock_type, int rank, int assert,
                                                    MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_lock(lock_type, rank, assert, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_unlock(int rank, MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_unlock(rank, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_get(void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_get(origin_addr, origin_count, origin_datatype, target_rank,
                              target_disp, target_count, target_datatype, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_fence(int assert, MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_fence(assert, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_accumulate(const void *origin_addr, int origin_count,
                                                      MPI_Datatype origin_datatype, int target_rank,
                                                      MPI_Aint target_disp, int target_count,
                                                      MPI_Datatype target_datatype, MPI_Op op,
                                                      MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                     target_rank, target_disp, target_count,
                                     target_datatype, op, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rput(const void *origin_addr, int origin_count,
                                                MPI_Datatype origin_datatype, int target_rank,
                                                MPI_Aint target_disp, int target_count,
                                                MPI_Datatype target_datatype, MPIR_Win * win,
                                                MPIR_Request ** request)
{
    int ret;


    ret = MPIDI_POSIX_mpi_rput(origin_addr, origin_count, origin_datatype, target_rank,
                               target_disp, target_count, target_datatype, win, request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_local(int rank, MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_flush_local(rank, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_compare_and_swap(const void *origin_addr,
                                                            const void *compare_addr,
                                                            void *result_addr,
                                                            MPI_Datatype datatype,
                                                            int target_rank, MPI_Aint target_disp,
                                                            MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                           datatype, target_rank, target_disp, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_raccumulate(const void *origin_addr, int origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       int target_rank, MPI_Aint target_disp,
                                                       int target_count,
                                                       MPI_Datatype target_datatype, MPI_Op op,
                                                       MPIR_Win * win, MPIR_Request ** request)
{
    int ret;


    ret = MPIDI_POSIX_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, op, win, request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rget_accumulate(const void *origin_addr,
                                                           int origin_count,
                                                           MPI_Datatype origin_datatype,
                                                           void *result_addr, int result_count,
                                                           MPI_Datatype result_datatype,
                                                           int target_rank, MPI_Aint target_disp,
                                                           int target_count,
                                                           MPI_Datatype target_datatype, MPI_Op op,
                                                           MPIR_Win * win, MPIR_Request ** request)
{
    int ret;


    ret = MPIDI_POSIX_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                          result_addr, result_count, result_datatype,
                                          target_rank, target_disp, target_count,
                                          target_datatype, op, win, request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_fetch_and_op(const void *origin_addr,
                                                        void *result_addr, MPI_Datatype datatype,
                                                        int target_rank, MPI_Aint target_disp,
                                                        MPI_Op op, MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank,
                                       target_disp, op, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush(int rank, MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_flush(rank, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_local_all(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_flush_local_all(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_unlock_all(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_unlock_all(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rget(void *origin_addr, int origin_count,
                                                MPI_Datatype origin_datatype, int target_rank,
                                                MPI_Aint target_disp, int target_count,
                                                MPI_Datatype target_datatype, MPIR_Win * win,
                                                MPIR_Request ** request)
{
    int ret;


    ret = MPIDI_POSIX_mpi_rget(origin_addr, origin_count, origin_datatype, target_rank,
                               target_disp, target_count, target_datatype, win, request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_sync(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_sync(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_all(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_flush_all(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_get_accumulate(const void *origin_addr,
                                                          int origin_count,
                                                          MPI_Datatype origin_datatype,
                                                          void *result_addr, int result_count,
                                                          MPI_Datatype result_datatype,
                                                          int target_rank, MPI_Aint target_disp,
                                                          int target_count,
                                                          MPI_Datatype target_datatype, MPI_Op op,
                                                          MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                         result_addr, result_count, result_datatype,
                                         target_rank, target_disp, target_count,
                                         target_datatype, op, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_lock_all(assert, win);

    return ret;
}

#endif /* SHM_RMA_H_INCLUDED */
