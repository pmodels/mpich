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
                                               MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int target_dt_contig;
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;
    MPIR_Datatype *dt_ptr = NULL;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

    MPIDI_Datatype_get_contig_dt_ptr(target_datatype, target_dt_contig, dt_ptr);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype) || target_dt_contig) {
        protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype, 0,
                                                MPIDIG_PUT_REQ);
        mpi_errno = MPIDIG_mpi_put_new(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_disp, target_count, target_datatype, av, win, 0,
                                       0, protocol);
    } else {
        int flattened_sz = 0;
        int dummy_host_buf;
        MPIR_Typerep_flatten_size(dt_ptr, &flattened_sz);
        int am_hdr_max_size = MPIDI_SHM_am_hdr_max_sz();
        if (sizeof(MPIDIG_put_msg_t) + flattened_sz <= am_hdr_max_size) {
            protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype,
                                                    flattened_sz, MPIDIG_PUT_REQ);
            mpi_errno = MPIDIG_mpi_put_new(origin_addr, origin_count, origin_datatype,
                                           target_rank, target_disp, target_count,
                                           target_datatype, av, win, flattened_sz, 0, protocol);
        } else {
            /* we should check the flattened_dt as the buffer. But we know it is always on host
             * memory. So use dummy_host_buf for checking here and defer the actual flattening
             * to later time. */
            protocol = MPIDI_SHM_am_choose_protocol(&dummy_host_buf, flattened_sz, MPI_BYTE, 0,
                                                    MPIDIG_PUT_DT_REQ);
            mpi_errno = MPIDIG_mpi_put_new(origin_addr, origin_count, origin_datatype,
                                           target_rank, target_disp, target_count,
                                           target_datatype, av, win, flattened_sz, 1, protocol);
        }
    }
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_get(void *origin_addr,
                                               int origin_count,
                                               MPI_Datatype origin_datatype,
                                               int target_rank,
                                               MPI_Aint target_disp,
                                               int target_count, MPI_Datatype target_datatype,
                                               MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int target_dt_contig;
    int dummy_host_buf;
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;
    int flattened_sz = 0;
    MPIR_Datatype *dt_ptr = NULL;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    MPIDI_Datatype_get_contig_dt_ptr(target_datatype, target_dt_contig, dt_ptr);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype) || target_dt_contig) {
        protocol = MPIDI_SHM_am_choose_protocol(NULL, 0, MPI_DATATYPE_NULL, 0, MPIDIG_GET_REQ);
        mpi_errno = MPIDIG_mpi_get_new(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_disp, target_count, target_datatype, av, win, 0,
                                       protocol);
    } else {
        MPIR_Typerep_flatten_size(dt_ptr, &flattened_sz);
        /* we should check the flattened_dt as the buffer. But we know it is always on host
         * memory. So use dummy_host_buf for checking here and defer the actual flattening
         * to later time. */
        protocol = MPIDI_SHM_am_choose_protocol(&dummy_host_buf, flattened_sz, MPI_BYTE, 0,
                                                MPIDIG_GET_REQ);
        mpi_errno = MPIDIG_mpi_get_new(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_disp, target_count, target_datatype, av, win,
                                       flattened_sz, protocol);
    }
    return mpi_errno;
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
    int mpi_errno = MPI_SUCCESS;
    int target_dt_contig;
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;
    MPIR_Datatype *dt_ptr = NULL;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

    MPIDI_Datatype_get_contig_dt_ptr(target_datatype, target_dt_contig, dt_ptr);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype) || target_dt_contig) {
        protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype, 0,
                                                MPIDIG_PUT_REQ);
        mpi_errno = MPIDIG_mpi_rput_new(origin_addr, origin_count, origin_datatype, target_rank,
                                        target_disp, target_count, target_datatype, av, win,
                                        request, 0, 0, protocol);
    } else {
        int flattened_sz = 0;
        int dummy_host_buf;
        MPIR_Typerep_flatten_size(dt_ptr, &flattened_sz);
        int am_hdr_max_size = MPIDI_SHM_am_hdr_max_sz();
        if (sizeof(MPIDIG_put_msg_t) + flattened_sz <= am_hdr_max_size) {
            protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype,
                                                    flattened_sz, MPIDIG_PUT_REQ);
            mpi_errno = MPIDIG_mpi_rput_new(origin_addr, origin_count, origin_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, av, win, request, flattened_sz, 0,
                                            protocol);
        } else {
            /* we should check the flattened_dt as the buffer. But we know it is always on host
             * memory. So use dummy_host_buf for checking here and defer the actual flattening
             * to later time. */
            protocol = MPIDI_SHM_am_choose_protocol(&dummy_host_buf, flattened_sz, MPI_BYTE, 0,
                                                    MPIDIG_PUT_DT_REQ);
            mpi_errno = MPIDIG_mpi_rput_new(origin_addr, origin_count, origin_datatype,
                                            target_rank, target_disp, target_count,
                                            target_datatype, av, win, request, flattened_sz, 1,
                                            protocol);
        }
    }
    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_compare_and_swap(const void *origin_addr,
                                                            const void *compare_addr,
                                                            void *result_addr,
                                                            MPI_Datatype datatype,
                                                            int target_rank, MPI_Aint target_disp,
                                                            MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

    protocol = MPIDI_SHM_am_choose_protocol(origin_addr, 2, datatype, 0, MPIDIG_CSWAP_REQ);
    return MPIDIG_mpi_compare_and_swap_new(origin_addr, compare_addr, result_addr, datatype,
                                           target_rank, target_disp, av, win, protocol);
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
    int mpi_errno = MPI_SUCCESS;
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;
    MPIR_Datatype *dt_ptr = NULL;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

    MPIDI_Datatype_get_dt_ptr(target_datatype, dt_ptr);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype, 0,
                                                MPIDIG_ACC_REQ);
        mpi_errno = MPIDIG_mpi_raccumulate_new(origin_addr, origin_count, origin_datatype,
                                               target_rank, target_disp, target_count,
                                               target_datatype, op, av, win, request, 0, 0,
                                               protocol);
    } else {
        int flattened_sz = 0;
        int dummy_host_buf;
        MPIR_Typerep_flatten_size(dt_ptr, &flattened_sz);
        int am_hdr_max_size = MPIDI_SHM_am_hdr_max_sz();
        if (sizeof(MPIDIG_acc_req_msg_t) + flattened_sz <= am_hdr_max_size) {
            protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype,
                                                    flattened_sz, MPIDIG_ACC_REQ);
            mpi_errno = MPIDIG_mpi_raccumulate_new(origin_addr, origin_count, origin_datatype,
                                                   target_rank, target_disp, target_count,
                                                   target_datatype, op, av, win, request,
                                                   flattened_sz, 0, protocol);
        } else {
            /* we should check the flattened_dt as the buffer. But we know it is always on host
             * memory. So use dummy_host_buf for checking here and defer the actual flattening
             * to later time. */
            protocol = MPIDI_SHM_am_choose_protocol(&dummy_host_buf, flattened_sz, MPI_BYTE, 0,
                                                    MPIDIG_ACC_DT_REQ);
            mpi_errno = MPIDIG_mpi_raccumulate_new(origin_addr, origin_count, origin_datatype,
                                                   target_rank, target_disp, target_count,
                                                   target_datatype, op, av, win, request,
                                                   flattened_sz, 1, protocol);
        }
    }
    return mpi_errno;
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
    int mpi_errno = MPI_SUCCESS;
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;
    MPIR_Datatype *dt_ptr = NULL;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

    MPIDI_Datatype_get_dt_ptr(target_datatype, dt_ptr);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype, 0,
                                                MPIDIG_GET_ACC_REQ);
        mpi_errno = MPIDIG_mpi_rget_accumulate_new(origin_addr, origin_count, origin_datatype,
                                                   result_addr, result_count, result_datatype,
                                                   target_rank, target_disp, target_count,
                                                   target_datatype, op, av, win, request, 0, 0,
                                                   protocol);
    } else {
        int flattened_sz = 0;
        int dummy_host_buf;
        MPIR_Typerep_flatten_size(dt_ptr, &flattened_sz);
        int am_hdr_max_size = MPIDI_SHM_am_hdr_max_sz();
        if (sizeof(MPIDIG_get_acc_req_msg_t) + flattened_sz <= am_hdr_max_size) {
            protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype,
                                                    flattened_sz, MPIDIG_GET_ACC_REQ);
            mpi_errno = MPIDIG_mpi_rget_accumulate_new(origin_addr, origin_count, origin_datatype,
                                                       result_addr, result_count, result_datatype,
                                                       target_rank, target_disp, target_count,
                                                       target_datatype, op, av, win, request,
                                                       flattened_sz, 0, protocol);
        } else {
            /* we should check the flattened_dt as the buffer. But we know it is always on host
             * memory. So use dummy_host_buf for checking here and defer the actual flattening
             * to later time. */
            protocol = MPIDI_SHM_am_choose_protocol(&dummy_host_buf, flattened_sz, MPI_BYTE, 0,
                                                    MPIDIG_GET_ACC_DT_REQ);
            mpi_errno = MPIDIG_mpi_rget_accumulate_new(origin_addr, origin_count, origin_datatype,
                                                       result_addr, result_count, result_datatype,
                                                       target_rank, target_disp, target_count,
                                                       target_datatype, op, av, win, request,
                                                       flattened_sz, 1, protocol);
        }
    }
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_fetch_and_op(const void *origin_addr,
                                                        void *result_addr,
                                                        MPI_Datatype datatype,
                                                        int target_rank,
                                                        MPI_Aint target_disp, MPI_Op op,
                                                        MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

    protocol = MPIDI_SHM_am_choose_protocol(origin_addr, 1, datatype, 0, MPIDIG_GET_ACC_REQ);
    mpi_errno = MPIDIG_mpi_fetch_and_op_new(origin_addr, result_addr, datatype, target_rank,
                                            target_disp, op, av, win, protocol);
    return mpi_errno;
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
    int mpi_errno = MPI_SUCCESS;
    int target_dt_contig;
    int dummy_host_buf;
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;
    int flattened_sz = 0;
    MPIR_Datatype *dt_ptr = NULL;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);
    MPIDI_Datatype_get_contig_dt_ptr(target_datatype, target_dt_contig, dt_ptr);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype) || target_dt_contig) {
        protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype, 0,
                                                MPIDIG_GET_REQ);
        mpi_errno = MPIDIG_mpi_rget_new(origin_addr, origin_count, origin_datatype, target_rank,
                                        target_disp, target_count, target_datatype, av, win,
                                        request, 0, protocol);
    } else {
        MPIR_Typerep_flatten_size(dt_ptr, &flattened_sz);
        /* we should check the flattened_dt as the buffer. But we know it is always on host
         * memory. So use dummy_host_buf for checking here and defer the actual flattening
         * to later time. */
        protocol = MPIDI_SHM_am_choose_protocol(&dummy_host_buf, flattened_sz, MPI_BYTE, 0,
                                                MPIDIG_GET_REQ);
        mpi_errno = MPIDIG_mpi_rget_new(origin_addr, origin_count, origin_datatype, target_rank,
                                        target_disp, target_count, target_datatype, av, win,
                                        request, flattened_sz, protocol);
    }
    return mpi_errno;
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
    int mpi_errno = MPI_SUCCESS;
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;
    MPIR_Datatype *dt_ptr = NULL;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

    MPIDI_Datatype_get_dt_ptr(target_datatype, dt_ptr);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype, 0,
                                                MPIDIG_GET_ACC_REQ);
        mpi_errno = MPIDIG_mpi_get_accumulate_new(origin_addr, origin_count, origin_datatype,
                                                  result_addr, result_count, result_datatype,
                                                  target_rank, target_disp, target_count,
                                                  target_datatype, op, av, win, 0, 0, protocol);
    } else {
        int flattened_sz = 0;
        int dummy_host_buf;
        MPIR_Typerep_flatten_size(dt_ptr, &flattened_sz);
        int am_hdr_max_size = MPIDI_SHM_am_hdr_max_sz();
        if (sizeof(MPIDIG_get_acc_req_msg_t) + flattened_sz <= am_hdr_max_size) {
            protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype,
                                                    flattened_sz, MPIDIG_GET_ACC_REQ);
            mpi_errno = MPIDIG_mpi_get_accumulate_new(origin_addr, origin_count, origin_datatype,
                                                      result_addr, result_count, result_datatype,
                                                      target_rank, target_disp, target_count,
                                                      target_datatype, op, av, win, flattened_sz, 0,
                                                      protocol);
        } else {
            /* we should check the flattened_dt as the buffer. But we know it is always on host
             * memory. So use dummy_host_buf for checking here and defer the actual flattening
             * to later time. */
            protocol = MPIDI_SHM_am_choose_protocol(&dummy_host_buf, flattened_sz, MPI_BYTE, 0,
                                                    MPIDIG_GET_ACC_DT_REQ);
            mpi_errno = MPIDIG_mpi_get_accumulate_new(origin_addr, origin_count, origin_datatype,
                                                      result_addr, result_count, result_datatype,
                                                      target_rank, target_disp, target_count,
                                                      target_datatype, op, av, win, flattened_sz, 1,
                                                      protocol);
        }
    }
    return mpi_errno;
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
    int mpi_errno = MPI_SUCCESS;
    int protocol = MPIDIG_AM_PROTOCOL__EAGER;
    MPIR_Datatype *dt_ptr = NULL;
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(win->comm_ptr, target_rank);

    MPIDI_Datatype_get_dt_ptr(target_datatype, dt_ptr);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype, 0,
                                                MPIDIG_ACC_REQ);
        mpi_errno = MPIDIG_mpi_accumulate_new(origin_addr, origin_count, origin_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, av, win, 0, 0, protocol);
    } else {
        int flattened_sz = 0;
        int dummy_host_buf;
        MPIR_Typerep_flatten_size(dt_ptr, &flattened_sz);
        int am_hdr_max_size = MPIDI_SHM_am_hdr_max_sz();
        if (sizeof(MPIDIG_acc_req_msg_t) + flattened_sz <= am_hdr_max_size) {
            protocol = MPIDI_SHM_am_choose_protocol(origin_addr, origin_count, origin_datatype,
                                                    flattened_sz, MPIDIG_ACC_REQ);
            mpi_errno = MPIDIG_mpi_accumulate_new(origin_addr, origin_count, origin_datatype,
                                                  target_rank, target_disp, target_count,
                                                  target_datatype, op, av, win, flattened_sz, 0,
                                                  protocol);
        } else {
            /* we should check the flattened_dt as the buffer. But we know it is always on host
             * memory. So use dummy_host_buf for checking here and defer the actual flattening
             * to later time. */
            protocol = MPIDI_SHM_am_choose_protocol(&dummy_host_buf, flattened_sz, MPI_BYTE, 0,
                                                    MPIDIG_ACC_DT_REQ);
            mpi_errno = MPIDIG_mpi_accumulate_new(origin_addr, origin_count, origin_datatype,
                                                  target_rank, target_disp, target_count,
                                                  target_datatype, op, av, win, flattened_sz, 1,
                                                  protocol);
        }
    }
    return mpi_errno;
}

#endif /* SHM_AM_FALLBACK_RMA_H_INCLUDED */
