/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_AM_FALLBACK_RMA_H_INCLUDED
#define SHM_AM_FALLBACK_RMA_H_INCLUDED

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
                                               MPI_Aint origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               MPI_Aint target_count, MPI_Datatype target_datatype,
                                               MPIR_Win * win, MPIDI_winattr_t winattr)
{
    return MPIDIG_mpi_put(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                          target_count, target_datatype, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_get(void *origin_addr,
                                               MPI_Aint origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               MPI_Aint target_count, MPI_Datatype target_datatype,
                                               MPIR_Win * win, MPIDI_winattr_t winattr)
{
    return MPIDIG_mpi_get(origin_addr, origin_count, origin_datatype, target_rank, target_disp,
                          target_count, target_datatype, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rput(const void *origin_addr,
                                                MPI_Aint origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                MPI_Aint target_count,
                                                MPI_Datatype target_datatype,
                                                MPIR_Win * win, MPIDI_winattr_t winattr,
                                                MPIR_Request ** request)
{
    return MPIDIG_mpi_rput(origin_addr, origin_count, origin_datatype, target_rank,
                           target_disp, target_count, target_datatype, win, request);
}


MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_compare_and_swap(const void *origin_addr,
                                                            const void *compare_addr,
                                                            void *result_addr,
                                                            MPI_Datatype datatype,
                                                            int target_rank, MPI_Aint target_disp,
                                                            MPIR_Win * win, MPIDI_winattr_t winattr)
{
    return MPIDIG_mpi_compare_and_swap(origin_addr, compare_addr, result_addr, datatype,
                                       target_rank, target_disp, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_raccumulate(const void *origin_addr,
                                                       MPI_Aint origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       int target_rank,
                                                       MPI_Aint target_disp,
                                                       MPI_Aint target_count,
                                                       MPI_Datatype target_datatype,
                                                       MPI_Op op, MPIR_Win * win,
                                                       MPIDI_winattr_t winattr,
                                                       MPIR_Request ** request)
{
    return MPIDIG_mpi_raccumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                  target_disp, target_count, target_datatype, op, win, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rget_accumulate(const void *origin_addr,
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
                                                           MPIDI_winattr_t winattr,
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
                                                        MPIR_Win * win, MPIDI_winattr_t winattr)
{
    return MPIDIG_mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank, target_disp, op,
                                   win);
}


MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rget(void *origin_addr,
                                                MPI_Aint origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                MPI_Aint target_count,
                                                MPI_Datatype target_datatype,
                                                MPIR_Win * win, MPIDI_winattr_t winattr,
                                                MPIR_Request ** request)
{
    return MPIDIG_mpi_rget(origin_addr, origin_count, origin_datatype, target_rank,
                           target_disp, target_count, target_datatype, win, request);
}


MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_get_accumulate(const void *origin_addr,
                                                          MPI_Aint origin_count,
                                                          MPI_Datatype origin_datatype,
                                                          void *result_addr,
                                                          MPI_Aint result_count,
                                                          MPI_Datatype result_datatype,
                                                          int target_rank,
                                                          MPI_Aint target_disp,
                                                          MPI_Aint target_count,
                                                          MPI_Datatype target_datatype, MPI_Op op,
                                                          MPIR_Win * win, MPIDI_winattr_t winattr)
{
    return MPIDIG_mpi_get_accumulate(origin_addr, origin_count, origin_datatype, result_addr,
                                     result_count, result_datatype, target_rank, target_disp,
                                     target_count, target_datatype, op, win);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_accumulate(const void *origin_addr,
                                                      MPI_Aint origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      int target_rank,
                                                      MPI_Aint target_disp,
                                                      MPI_Aint target_count,
                                                      MPI_Datatype target_datatype, MPI_Op op,
                                                      MPIR_Win * win, MPIDI_winattr_t winattr)
{
    return MPIDIG_mpi_accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                 target_disp, target_count, target_datatype, op, win);
}

#endif /* SHM_AM_FALLBACK_RMA_H_INCLUDED */
