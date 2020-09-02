/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_AM_FALLBACK_RMA_H_INCLUDED
#define SHM_AM_FALLBACK_RMA_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDIG_mpi_win_start(group, assert, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_complete(MPIR_Win * win)
{
    return MPIDIG_mpi_win_complete(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDIG_mpi_win_post(group, assert, win);
}


MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_wait(MPIR_Win * win)
{
    return MPIDIG_mpi_win_wait(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_test(MPIR_Win * win, int *flag)
{
    return MPIDIG_mpi_win_test(win, flag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_lock(int lock_type, int rank, int assert,
                                                    MPIR_Win * win)
{
    return MPIDIG_mpi_win_lock(lock_type, rank, assert, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_unlock(int rank, MPIR_Win * win)
{
    return MPIDIG_mpi_win_unlock(rank, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_fence(int assert, MPIR_Win * win)
{
    return MPIDIG_mpi_win_fence(assert, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_shared_query(MPIR_Win * win,
                                                            int rank,
                                                            MPI_Aint * size, int *disp_unit,
                                                            void *baseptr)
{
    return MPIDIG_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush(int rank, MPIR_Win * win)
{
    return MPIDIG_mpi_win_flush(rank, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_local_all(MPIR_Win * win)
{
    return MPIDIG_mpi_win_flush_local_all(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_unlock_all(MPIR_Win * win)
{
    return MPIDIG_mpi_win_unlock_all(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_local(int rank, MPIR_Win * win)
{
    return MPIDIG_mpi_win_flush_local(rank, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_sync(MPIR_Win * win)
{
    return MPIDIG_mpi_win_sync(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_all(MPIR_Win * win)
{
    return MPIDIG_mpi_win_flush_all(win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    return MPIDIG_mpi_win_lock_all(assert, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_win_cmpl_hook(MPIR_Win * win)
{
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_target_cmpl_hook(int rank, MPIR_Win * win)
{
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_target_local_cmpl_hook(int rank, MPIR_Win * win)
{
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_put(const void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count, MPI_Datatype target_datatype,
                                               MPIR_Win * win, bool target_abs_flag)
{
    return MPIDIG_mpi_put(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                          target_count, target_datatype, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_get(void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count, MPI_Datatype target_datatype,
                                               MPIR_Win * win)
{
    return MPIDIG_mpi_get(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                          target_count, target_datatype, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rput(const void *origin_addr,
                                                int origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                int target_count,
                                                MPI_Datatype target_datatype,
                                                MPIR_Win * win, MPIR_Request ** request)
{
    return MPIDIG_mpi_rput(origin_addr, origin_count, origin_datatype, target_rank,
                           target_disp, target_count, target_datatype, win, request);
}


MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_compare_and_swap(const void *origin_addr,
                                                            const void *compare_addr,
                                                            void *result_addr,
                                                            MPI_Datatype datatype,
                                                            int target_rank, MPI_Aint target_disp,
                                                            MPIR_Win * win)
{
    return MPIDIG_mpi_compare_and_swap(origin_addr, compare_addr, result_addr, datatype,
                                       target_rank, target_disp, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_raccumulate(const void *origin_addr,
                                                       int origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       int target_rank,
                                                       MPI_Aint target_disp,
                                                       int target_count,
                                                       MPI_Datatype target_datatype,
                                                       MPI_Op op, MPIR_Win * win,
                                                       MPIR_Request ** request)
{
    return MPIDIG_mpi_raccumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                  target_disp, target_count, target_datatype, op, win, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rget_accumulate(const void *origin_addr,
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
    return MPIDIG_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                      result_count, result_datatype, target_rank, target_disp,
                                      target_count, target_datatype, op, win, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_fetch_and_op(const void *origin_addr,
                                                        void *result_addr,
                                                        MPI_Datatype datatype,
                                                        int target_rank,
                                                        MPI_Aint target_disp, MPI_Op op,
                                                        MPIR_Win * win)
{
    return MPIDIG_mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank, target_disp, op,
                                   win);
}


MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rget(void *origin_addr,
                                                int origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                int target_count,
                                                MPI_Datatype target_datatype,
                                                MPIR_Win * win, MPIR_Request ** request)
{
    return MPIDIG_mpi_rget(origin_addr, origin_count, origin_datatype, target_rank,
                           target_disp, target_count, target_datatype, win, request);
}


MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_get_accumulate(const void *origin_addr,
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
    return MPIDIG_mpi_get_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                     result_count, result_datatype, target_rank, target_disp,
                                     target_count, target_datatype, op, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_accumulate(const void *origin_addr,
                                                      int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      int target_rank,
                                                      MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype, MPI_Op op,
                                                      MPIR_Win * win)
{
    return MPIDIG_mpi_accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                 target_disp, target_count, target_datatype, op, win);
}

#endif /* SHM_AM_FALLBACK_RMA_H_INCLUDED */
