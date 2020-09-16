/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_RMA_H_INCLUDED
#define POSIX_RMA_H_INCLUDED

#include "ch4_impl.h"
#include "posix_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_compute_accumulate(void *origin_addr,
                                                            int origin_count,
                                                            MPI_Datatype origin_datatype,
                                                            void *target_addr,
                                                            int target_count,
                                                            MPI_Datatype target_datatype, MPI_Op op)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype basic_type = MPI_DATATYPE_NULL;
    MPI_Aint predefined_dtp_size = 0, predefined_dtp_count = 0;
    MPI_Aint total_len = 0;
    MPI_Aint origin_dtp_size = 0;
    MPIR_Datatype *origin_dtp_ptr = NULL;
    void *packed_buf = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_COMPUTE_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_COMPUTE_ACCUMULATE);

    /* Handle contig origin datatype */
    if (MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
        mpi_errno = MPIDIG_compute_acc_op(origin_addr, origin_count, origin_datatype, target_addr,
                                          target_count, target_datatype, op,
                                          MPIDIG_ACC_SRCBUF_DEFAULT);
        MPIR_ERR_CHECK(mpi_errno);

        goto fn_exit;
    }

    /* Handle derived origin datatype */
    /* Get total length of origin data */
    MPIR_Datatype_get_size_macro(origin_datatype, origin_dtp_size);
    total_len = origin_dtp_size * origin_count;

    /* Standard (page 425 in 3.1 report) requires predefined datatype or
     * a derived datatype where all basic components are of the same predefined datatype.
     * Thus, basic_type should be correctly set. */
    MPIR_Datatype_get_ptr(origin_datatype, origin_dtp_ptr);
    MPIR_Assert(origin_dtp_ptr != NULL && origin_dtp_ptr->basic_type != MPI_DATATYPE_NULL);

    basic_type = origin_dtp_ptr->basic_type;
    MPIR_Datatype_get_size_macro(basic_type, predefined_dtp_size);
    MPIR_Assert(predefined_dtp_size > 0);
    predefined_dtp_count = total_len / predefined_dtp_size;

#if defined(HAVE_ERROR_CHECKING)
    MPI_Aint predefined_dtp_extent ATTRIBUTE((unused)) = 0;
    MPIR_Datatype_get_extent_macro(basic_type, predefined_dtp_extent);
    MPIR_Assert(predefined_dtp_count > 0 && predefined_dtp_size > 0 && predefined_dtp_extent > 0);
#endif

    /* Pack origin data into a contig buffer */
    packed_buf = MPL_malloc(total_len, MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP(packed_buf == NULL, mpi_errno, MPI_ERR_NO_MEM, "**nomem");

    MPI_Aint actual_pack_bytes;
    mpi_errno = MPIR_Typerep_pack(origin_addr, origin_count, origin_datatype, 0,
                                  packed_buf, total_len, &actual_pack_bytes);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(actual_pack_bytes == total_len);

    mpi_errno = MPIDIG_compute_acc_op((void *) packed_buf, (int) predefined_dtp_count, basic_type,
                                      target_addr, target_count, target_datatype, op,
                                      MPIDIG_ACC_SRCBUF_PACKED);

  fn_exit:
    MPL_free(packed_buf);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_COMPUTE_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_put(const void *origin_addr,
                                                int origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                int target_count, MPI_Datatype target_datatype,
                                                MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    size_t origin_data_sz = 0, target_data_sz = 0;
    int disp_unit = 0;
    void *base = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DO_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DO_PUT);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_origin_target_size(origin_datatype, target_datatype,
                                            origin_count, target_count,
                                            origin_data_sz, target_data_sz);
    if (origin_data_sz == 0 || target_data_sz == 0)
        goto fn_exit;

    if (target_rank == MPIDIU_win_comm_rank(win, winattr)) {
        base = win->base;
        disp_unit = win->disp_unit;
    } else {
        MPIDIG_win_shared_info_t *shared_table = MPIDIG_WIN(win, shared_table);
        int local_target_rank = MPIDIU_win_rank_to_intra_rank(win, target_rank, winattr);
        disp_unit = shared_table[local_target_rank].disp_unit;
        base = shared_table[local_target_rank].shm_base_addr;
    }

    mpi_errno = MPIR_Localcopy(origin_addr, origin_count, origin_datatype,
                               (char *) base + disp_unit * target_disp, target_count,
                               target_datatype);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DO_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_get(void *origin_addr,
                                                int origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                int target_count, MPI_Datatype target_datatype,
                                                MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    size_t origin_data_sz = 0, target_data_sz = 0;
    int disp_unit = 0;
    void *base = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DO_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DO_GET);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_origin_target_size(origin_datatype, target_datatype,
                                            origin_count, target_count,
                                            origin_data_sz, target_data_sz);
    if (origin_data_sz == 0 || target_data_sz == 0)
        goto fn_exit;

    if (target_rank == MPIDIU_win_comm_rank(win, winattr)) {
        base = win->base;
        disp_unit = win->disp_unit;
    } else {
        MPIDIG_win_shared_info_t *shared_table = MPIDIG_WIN(win, shared_table);
        int local_target_rank = MPIDIU_win_rank_to_intra_rank(win, target_rank, winattr);
        disp_unit = shared_table[local_target_rank].disp_unit;
        base = shared_table[local_target_rank].shm_base_addr;
    }

    mpi_errno = MPIR_Localcopy((char *) base + disp_unit * target_disp, target_count,
                               target_datatype, origin_addr, origin_count, origin_datatype);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DO_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_get_accumulate(const void *origin_addr,
                                                           int origin_count,
                                                           MPI_Datatype origin_datatype,
                                                           void *result_addr,
                                                           int result_count,
                                                           MPI_Datatype result_datatype,
                                                           int target_rank,
                                                           MPI_Aint target_disp,
                                                           int target_count,
                                                           MPI_Datatype target_datatype, MPI_Op op,
                                                           MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
    size_t origin_data_sz = 0, target_data_sz = 0, result_data_sz = 0;
    int shm_locked = 0, disp_unit = 0;
    void *base = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DO_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DO_GET_ACCUMULATE);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(origin_datatype, origin_count, origin_data_sz);
    MPIDI_Datatype_check_size(target_datatype, target_count, target_data_sz);
    MPIDI_Datatype_check_size(result_datatype, result_count, result_data_sz);
    if (target_data_sz == 0 || (origin_data_sz == 0 && result_data_sz == 0))
        goto fn_exit;

    if (target_rank == MPIDIU_win_comm_rank(win, winattr)) {
        base = win->base;
        disp_unit = win->disp_unit;
    } else {
        MPIDIG_win_shared_info_t *shared_table = MPIDIG_WIN(win, shared_table);
        int local_target_rank = MPIDIU_win_rank_to_intra_rank(win, target_rank, winattr);
        disp_unit = shared_table[local_target_rank].disp_unit;
        base = shared_table[local_target_rank].shm_base_addr;
    }

    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED) {
        MPIDI_POSIX_RMA_MUTEX_LOCK(posix_win->shm_mutex_ptr);
        shm_locked = 1;
    }

    mpi_errno = MPIR_Localcopy((char *) base + disp_unit * target_disp, target_count,
                               target_datatype, result_addr, result_count, result_datatype);
    MPIR_ERR_CHECK(mpi_errno);

    if (op != MPI_NO_OP) {
        mpi_errno = MPIDI_POSIX_compute_accumulate((void *) origin_addr, origin_count,
                                                   origin_datatype,
                                                   (char *) base + disp_unit * target_disp,
                                                   target_count, target_datatype, op);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    if (shm_locked)
        MPIDI_POSIX_RMA_MUTEX_UNLOCK(posix_win->shm_mutex_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DO_GET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_accumulate(const void *origin_addr,
                                                       int origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       int target_rank,
                                                       MPI_Aint target_disp,
                                                       int target_count,
                                                       MPI_Datatype target_datatype, MPI_Op op,
                                                       MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
    size_t origin_data_sz = 0, target_data_sz = 0;
    int disp_unit = 0;
    void *base = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DO_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DO_ACCUMULATE);

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(origin_datatype, origin_count, origin_data_sz);
    MPIDI_Datatype_check_size(target_datatype, target_count, target_data_sz);
    if (origin_data_sz == 0 || target_data_sz == 0)
        goto fn_exit;

    if (target_rank == MPIDIU_win_comm_rank(win, winattr)) {
        base = win->base;
        disp_unit = win->disp_unit;
    } else {
        MPIDIG_win_shared_info_t *shared_table = MPIDIG_WIN(win, shared_table);
        int local_target_rank = MPIDIU_win_rank_to_intra_rank(win, target_rank, winattr);
        disp_unit = shared_table[local_target_rank].disp_unit;
        base = shared_table[local_target_rank].shm_base_addr;
    }

    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED)
        MPIDI_POSIX_RMA_MUTEX_LOCK(posix_win->shm_mutex_ptr);

    mpi_errno = MPIDI_POSIX_compute_accumulate((void *) origin_addr, origin_count, origin_datatype,
                                               (char *) base + disp_unit * target_disp,
                                               target_count, target_datatype, op);
    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED)
        MPIDI_POSIX_RMA_MUTEX_UNLOCK(posix_win->shm_mutex_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DO_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_put(const void *origin_addr,
                                                 int origin_count,
                                                 MPI_Datatype origin_datatype,
                                                 int target_rank,
                                                 MPI_Aint target_disp,
                                                 int target_count, MPI_Datatype target_datatype,
                                                 MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_PUT);

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_put(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win);
    } else {
        mpi_errno = MPIDI_POSIX_do_put(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_disp, target_count, target_datatype, win, winattr);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_PUT);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_get(void *origin_addr,
                                                 int origin_count,
                                                 MPI_Datatype origin_datatype,
                                                 int target_rank,
                                                 MPI_Aint target_disp,
                                                 int target_count, MPI_Datatype target_datatype,
                                                 MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_GET);

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_get(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win);
    } else {
        mpi_errno = MPIDI_POSIX_do_get(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_disp, target_count, target_datatype, win, winattr);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_GET);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_rput(const void *origin_addr,
                                                  int origin_count,
                                                  MPI_Datatype origin_datatype,
                                                  int target_rank,
                                                  MPI_Aint target_disp,
                                                  int target_count,
                                                  MPI_Datatype target_datatype,
                                                  MPIR_Win * win, MPIDI_winattr_t winattr,
                                                  MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_RPUT);

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_rput(origin_addr, origin_count, origin_datatype,
                                    target_rank, target_disp, target_count,
                                    target_datatype, win, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_POSIX_do_put(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win,
                                   winattr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a completed request for user. */
    sreq = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__RMA, 0);
    MPIR_Assert(sreq);

    MPIR_Request_add_ref(sreq);
    MPID_Request_complete(sreq);
    *request = sreq;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_RPUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_compare_and_swap(const void *origin_addr,
                                                              const void *compare_addr,
                                                              void *result_addr,
                                                              MPI_Datatype datatype,
                                                              int target_rank, MPI_Aint target_disp,
                                                              MPIR_Win * win,
                                                              MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
    size_t data_sz = 0;
    int disp_unit = 0;
    void *base = NULL, *target_addr = NULL;
    MPI_Aint dtype_sz = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMPARE_AND_SWAP);

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                datatype, target_rank, target_disp, win);
        goto fn_exit;
    }

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(datatype, 1, data_sz);
    if (data_sz == 0)
        goto fn_exit;

    if (target_rank == MPIDIU_win_comm_rank(win, winattr)) {
        base = win->base;
        disp_unit = win->disp_unit;
    } else {
        MPIDIG_win_shared_info_t *shared_table = MPIDIG_WIN(win, shared_table);
        int local_target_rank = MPIDIU_win_rank_to_intra_rank(win, target_rank, winattr);
        disp_unit = shared_table[local_target_rank].disp_unit;
        base = shared_table[local_target_rank].shm_base_addr;
    }

    target_addr = (char *) base + disp_unit * target_disp;
    MPIR_Datatype_get_size_macro(datatype, dtype_sz);

    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED)
        MPIDI_POSIX_RMA_MUTEX_LOCK(posix_win->shm_mutex_ptr);

    MPIR_Typerep_copy(result_addr, target_addr, dtype_sz);
    if (MPIR_Compare_equal(compare_addr, target_addr, datatype)) {
        MPIR_Typerep_copy(target_addr, origin_addr, dtype_sz);
    }

    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED)
        MPIDI_POSIX_RMA_MUTEX_UNLOCK(posix_win->shm_mutex_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMPARE_AND_SWAP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_raccumulate(const void *origin_addr,
                                                         int origin_count,
                                                         MPI_Datatype origin_datatype,
                                                         int target_rank,
                                                         MPI_Aint target_disp,
                                                         int target_count,
                                                         MPI_Datatype target_datatype,
                                                         MPI_Op op, MPIR_Win * win,
                                                         MPIDI_winattr_t winattr,
                                                         MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_RACCUMULATE);

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                           target_rank, target_disp, target_count,
                                           target_datatype, op, win, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_POSIX_do_accumulate(origin_addr, origin_count, origin_datatype,
                                          target_rank, target_disp, target_count,
                                          target_datatype, op, win, winattr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a completed request for user. */
    sreq = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__RMA, 0);
    MPIR_Assert(sreq);

    MPIR_Request_add_ref(sreq);
    MPID_Request_complete(sreq);
    *request = sreq;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_RACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_rget_accumulate(const void *origin_addr,
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
                                                             MPIDI_winattr_t winattr,
                                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_RGET_ACCUMULATE);

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                               result_addr, result_count, result_datatype,
                                               target_rank, target_disp, target_count,
                                               target_datatype, op, win, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_POSIX_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                              result_addr, result_count, result_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win, winattr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a completed request for user. */
    sreq = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__RMA, 0);
    MPIR_Assert(sreq);

    MPIR_Request_add_ref(sreq);
    MPID_Request_complete(sreq);
    *request = sreq;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_RGET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_fetch_and_op(const void *origin_addr,
                                                          void *result_addr,
                                                          MPI_Datatype datatype,
                                                          int target_rank,
                                                          MPI_Aint target_disp, MPI_Op op,
                                                          MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
    size_t data_sz = 0;
    int disp_unit = 0;
    void *base = NULL, *target_addr = NULL;
    MPI_Aint dtype_sz = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_FETCH_AND_OP);

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_fetch_and_op(origin_addr, result_addr, datatype,
                                            target_rank, target_disp, op, win);
        goto fn_exit;
    }

    MPIDIG_RMA_OP_CHECK_SYNC(target_rank, win);

    MPIDI_Datatype_check_size(datatype, 1, data_sz);
    if (data_sz == 0)
        goto fn_exit;

    if (target_rank == MPIDIU_win_comm_rank(win, winattr)) {
        base = win->base;
        disp_unit = win->disp_unit;
    } else {
        MPIDIG_win_shared_info_t *shared_table = MPIDIG_WIN(win, shared_table);
        int local_target_rank = MPIDIU_win_rank_to_intra_rank(win, target_rank, winattr);
        disp_unit = shared_table[local_target_rank].disp_unit;
        base = shared_table[local_target_rank].shm_base_addr;
    }

    target_addr = (char *) base + disp_unit * target_disp;
    MPIR_Datatype_get_size_macro(datatype, dtype_sz);

    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED)
        MPIDI_POSIX_RMA_MUTEX_LOCK(posix_win->shm_mutex_ptr);

    MPIR_Typerep_copy(result_addr, target_addr, dtype_sz);

    if (op != MPI_NO_OP) {
        /* We need to make sure op is valid here.
         * 0xf is the mask for op index in MPIR_Op_table,
         * and op should start from 1. */
        MPIR_Assert(((op) & 0xf) > 0);
        MPI_User_function *uop = MPIR_OP_HDL_TO_FN(op);
        int one = 1;

        (*uop) ((void *) origin_addr, target_addr, &one, &datatype);
    }

    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED)
        MPIDI_POSIX_RMA_MUTEX_UNLOCK(posix_win->shm_mutex_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_FETCH_AND_OP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_rget(void *origin_addr,
                                                  int origin_count,
                                                  MPI_Datatype origin_datatype,
                                                  int target_rank,
                                                  MPI_Aint target_disp,
                                                  int target_count,
                                                  MPI_Datatype target_datatype,
                                                  MPIR_Win * win, MPIDI_winattr_t winattr,
                                                  MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_RGET);

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_rget(origin_addr, origin_count, origin_datatype,
                                    target_rank, target_disp, target_count,
                                    target_datatype, win, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_POSIX_do_get(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win,
                                   winattr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a completed request for user. */
    sreq = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__RMA, 0);
    MPIR_Assert(sreq);

    MPIR_Request_add_ref(sreq);
    MPID_Request_complete(sreq);
    *request = sreq;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_RGET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_get_accumulate(const void *origin_addr,
                                                            int origin_count,
                                                            MPI_Datatype origin_datatype,
                                                            void *result_addr,
                                                            int result_count,
                                                            MPI_Datatype result_datatype,
                                                            int target_rank,
                                                            MPI_Aint target_disp,
                                                            int target_count,
                                                            MPI_Datatype target_datatype, MPI_Op op,
                                                            MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_GET_ACCUMULATE);

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                              result_addr, result_count, result_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win);
    } else {
        mpi_errno = MPIDI_POSIX_do_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                  result_addr, result_count, result_datatype,
                                                  target_rank, target_disp, target_count,
                                                  target_datatype, op, win, winattr);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_GET_ACCUMULATE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_accumulate(const void *origin_addr,
                                                        int origin_count,
                                                        MPI_Datatype origin_datatype,
                                                        int target_rank,
                                                        MPI_Aint target_disp,
                                                        int target_count,
                                                        MPI_Datatype target_datatype, MPI_Op op,
                                                        MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ACCUMULATE);

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                          target_rank, target_disp, target_count,
                                          target_datatype, op, win);
    } else {
        mpi_errno = MPIDI_POSIX_do_accumulate(origin_addr, origin_count, origin_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win, winattr);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ACCUMULATE);
    return mpi_errno;
}

#endif /* POSIX_RMA_H_INCLUDED */
