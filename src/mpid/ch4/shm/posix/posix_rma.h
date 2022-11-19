/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_RMA_H_INCLUDED
#define POSIX_RMA_H_INCLUDED

#include "ch4_impl.h"
#include "posix_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_compute_accumulate(void *origin_addr,
                                                            MPI_Aint origin_count,
                                                            MPI_Datatype origin_datatype,
                                                            void *target_addr,
                                                            MPI_Aint target_count,
                                                            MPI_Datatype target_datatype, MPI_Op op,
                                                            int mapped_device)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype basic_type = MPI_DATATYPE_NULL;
    MPI_Aint predefined_dtp_size = 0, predefined_dtp_count = 0;
    MPI_Aint total_len = 0;
    MPI_Aint origin_dtp_size = 0;
    MPIR_Datatype *origin_dtp_ptr = NULL;
    bool is_packed;
    void *packed_buf = NULL;
    MPIR_FUNC_ENTER;

    /* Handle contig origin datatype */
    if (MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
        is_packed = false;
        mpi_errno = MPIR_Typerep_op(origin_addr, origin_count, origin_datatype,
                                    target_addr, target_count, target_datatype,
                                    op, is_packed, mapped_device);
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
    mpi_errno = MPIR_Typerep_pack(origin_addr, origin_count, origin_datatype, 0, packed_buf,
                                  total_len, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(actual_pack_bytes == total_len);

    is_packed = true;
    mpi_errno = MPIR_Typerep_op((void *) packed_buf, (int) predefined_dtp_count, basic_type,
                                target_addr, target_count, target_datatype,
                                op, is_packed, mapped_device);

  fn_exit:
    MPL_free(packed_buf);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_put(const void *origin_addr,
                                                MPI_Aint origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                MPI_Aint target_count, MPI_Datatype target_datatype,
                                                MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    size_t origin_data_sz = 0, target_data_sz = 0;
    int disp_unit = 0;
    void *base = NULL;
    MPIR_FUNC_ENTER;

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

    if (winattr & MPIDI_WINATTR_MR_PREFERRED) {
        /* If MR-preferred is set, switch to nonblocking version which may slightly
         * increase per-op+flush overhead. */
        MPIR_Typerep_req typerep_req = MPIR_TYPEREP_REQ_NULL;
        mpi_errno = MPIR_Ilocalcopy(origin_addr, origin_count, origin_datatype,
                                    (char *) base + disp_unit * target_disp, target_count,
                                    target_datatype, &typerep_req);
        MPIDI_POSIX_rma_outstanding_req_enqueu(typerep_req, &win->dev.shm.posix);
    } else {
        /* By default issuing blocking copy for lower per-op latency. */
        mpi_errno = MPIR_Localcopy(origin_addr, origin_count, origin_datatype,
                                   (char *) base + disp_unit * target_disp, target_count,
                                   target_datatype);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_get(void *origin_addr,
                                                MPI_Aint origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                MPI_Aint target_count, MPI_Datatype target_datatype,
                                                MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    size_t origin_data_sz = 0, target_data_sz = 0;
    int disp_unit = 0;
    void *base = NULL;
    MPIR_FUNC_ENTER;

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

    if (winattr & MPIDI_WINATTR_MR_PREFERRED) {
        /* If MR-preferred is set, switch to nonblocking version which may slightly
         * increase per-op+flush overhead. */
        MPIR_Typerep_req typerep_req = MPIR_TYPEREP_REQ_NULL;
        mpi_errno = MPIR_Ilocalcopy((char *) base + disp_unit * target_disp, target_count,
                                    target_datatype, origin_addr, origin_count, origin_datatype,
                                    &typerep_req);
        MPIDI_POSIX_rma_outstanding_req_enqueu(typerep_req, &win->dev.shm.posix);
    } else {
        /* By default issuing blocking copy for lower per-op latency. */
        mpi_errno = MPIR_Localcopy((char *) base + disp_unit * target_disp, target_count,
                                   target_datatype, origin_addr, origin_count, origin_datatype);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_get_accumulate(const void *origin_addr,
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
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
    size_t origin_data_sz = 0, target_data_sz = 0, result_data_sz = 0;
    int disp_unit = 0;
    void *base = NULL;
    int mapped_device = -1;
    MPIR_FUNC_ENTER;

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
        mapped_device = shared_table[local_target_rank].ipc_mapped_device;
    }

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    int vci = MPIDI_WIN(win, am_vci);
#endif
    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED) {
        MPIDI_POSIX_RMA_MUTEX_LOCK(posix_win->shm_mutex_ptr);
    } else {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    }

    mpi_errno = MPIR_Localcopy((char *) base + disp_unit * target_disp, target_count,
                               target_datatype, result_addr, result_count, result_datatype);

    if (mpi_errno == MPI_SUCCESS && op != MPI_NO_OP) {
        mpi_errno = MPIDI_POSIX_compute_accumulate((void *) origin_addr, origin_count,
                                                   origin_datatype,
                                                   (char *) base + disp_unit * target_disp,
                                                   target_count, target_datatype, op,
                                                   mapped_device);
    }

    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED) {
        MPIDI_POSIX_RMA_MUTEX_UNLOCK(posix_win->shm_mutex_ptr);
    } else {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_accumulate(const void *origin_addr,
                                                       MPI_Aint origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       int target_rank,
                                                       MPI_Aint target_disp,
                                                       MPI_Aint target_count,
                                                       MPI_Datatype target_datatype, MPI_Op op,
                                                       MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_win_t *posix_win = &win->dev.shm.posix;
    size_t origin_data_sz = 0, target_data_sz = 0;
    int disp_unit = 0;
    void *base = NULL;
    int mapped_device = -1;
    MPIR_FUNC_ENTER;

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
        mapped_device = shared_table[local_target_rank].ipc_mapped_device;
    }

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    int vci = MPIDI_WIN(win, am_vci);
#endif
    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED) {
        MPIDI_POSIX_RMA_MUTEX_LOCK(posix_win->shm_mutex_ptr);
    } else {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    }

    mpi_errno = MPIDI_POSIX_compute_accumulate((void *) origin_addr, origin_count, origin_datatype,
                                               MPIR_get_contig_ptr(base, disp_unit * target_disp),
                                               target_count, target_datatype, op, mapped_device);
    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED) {
        MPIDI_POSIX_RMA_MUTEX_UNLOCK(posix_win->shm_mutex_ptr);
    } else {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_put(const void *origin_addr,
                                                 MPI_Aint origin_count,
                                                 MPI_Datatype origin_datatype,
                                                 int target_rank,
                                                 MPI_Aint target_disp,
                                                 MPI_Aint target_count,
                                                 MPI_Datatype target_datatype, MPIR_Win * win,
                                                 MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_put(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win);
    } else {
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
        int vci = MPIDI_WIN(win, am_vci);
#endif
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
        mpi_errno = MPIDI_POSIX_do_put(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_disp, target_count, target_datatype, win, winattr);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_get(void *origin_addr,
                                                 MPI_Aint origin_count,
                                                 MPI_Datatype origin_datatype,
                                                 int target_rank,
                                                 MPI_Aint target_disp,
                                                 MPI_Aint target_count,
                                                 MPI_Datatype target_datatype, MPIR_Win * win,
                                                 MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_get(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win);
    } else {
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
        int vci = MPIDI_WIN(win, am_vci);
#endif
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
        mpi_errno = MPIDI_POSIX_do_get(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_disp, target_count, target_datatype, win, winattr);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_rput(const void *origin_addr,
                                                  MPI_Aint origin_count,
                                                  MPI_Datatype origin_datatype,
                                                  int target_rank,
                                                  MPI_Aint target_disp,
                                                  MPI_Aint target_count,
                                                  MPI_Datatype target_datatype,
                                                  MPIR_Win * win, MPIDI_winattr_t winattr,
                                                  MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_rput(origin_addr, origin_count, origin_datatype,
                                    target_rank, target_disp, target_count,
                                    target_datatype, win, request);
        goto fn_exit;
    }

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDI_POSIX_do_put(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win,
                                   winattr);
    if (mpi_errno == MPI_SUCCESS) {
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
    }
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
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
    MPIR_FUNC_ENTER;

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

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    int vci = MPIDI_WIN(win, am_vci);
#endif
    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED) {
        MPIDI_POSIX_RMA_MUTEX_LOCK(posix_win->shm_mutex_ptr);
    } else {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    }

    MPIR_Typerep_copy(result_addr, target_addr, dtype_sz, MPIR_TYPEREP_FLAG_NONE);
    if (MPIR_Compare_equal(compare_addr, target_addr, datatype)) {
        MPIR_Typerep_copy(target_addr, origin_addr, dtype_sz, MPIR_TYPEREP_FLAG_NONE);
    }

    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED) {
        MPIDI_POSIX_RMA_MUTEX_UNLOCK(posix_win->shm_mutex_ptr);
    } else {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_raccumulate(const void *origin_addr,
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
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

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

    *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_rget_accumulate(const void *origin_addr,
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
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

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

    *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);

  fn_exit:
    MPIR_FUNC_EXIT;
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
    MPIR_FUNC_ENTER;

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

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    int vci = MPIDI_WIN(win, am_vci);
#endif
    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED) {
        MPIDI_POSIX_RMA_MUTEX_LOCK(posix_win->shm_mutex_ptr);
    } else {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    }

    MPIR_Typerep_copy(result_addr, target_addr, dtype_sz, MPIR_TYPEREP_FLAG_NONE);

    if (op != MPI_NO_OP) {
        /* We need to make sure op is valid here.
         * 0xf is the mask for op index in MPIR_Op_table,
         * and op should start from 1. */
        MPIR_Assert(((op) & 0xf) > 0);
        MPIR_op_function *uop = MPIR_OP_HDL_TO_FN(op);
        MPI_Aint one = 1;

        (*uop) ((void *) origin_addr, target_addr, &one, &datatype);
    }

    if (winattr & MPIDI_WINATTR_SHM_ALLOCATED) {
        MPIDI_POSIX_RMA_MUTEX_UNLOCK(posix_win->shm_mutex_ptr);
    } else {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_rget(void *origin_addr,
                                                  MPI_Aint origin_count,
                                                  MPI_Datatype origin_datatype,
                                                  int target_rank,
                                                  MPI_Aint target_disp,
                                                  MPI_Aint target_count,
                                                  MPI_Datatype target_datatype,
                                                  MPIR_Win * win, MPIDI_winattr_t winattr,
                                                  MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* CH4 schedules operation only based on process locality.
     * Thus the target might not be in shared memory of the window.*/
    if (!(winattr & MPIDI_WINATTR_SHM_ALLOCATED) &&
        target_rank != MPIDIU_win_comm_rank(win, winattr)) {
        mpi_errno = MPIDIG_mpi_rget(origin_addr, origin_count, origin_datatype,
                                    target_rank, target_disp, target_count,
                                    target_datatype, win, request);
        goto fn_exit;
    }

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    mpi_errno = MPIDI_POSIX_do_get(origin_addr, origin_count, origin_datatype,
                                   target_rank, target_disp, target_count, target_datatype, win,
                                   winattr);
    if (mpi_errno == MPI_SUCCESS) {
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RMA);
    }
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_get_accumulate(const void *origin_addr,
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
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

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

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_accumulate(const void *origin_addr,
                                                        MPI_Aint origin_count,
                                                        MPI_Datatype origin_datatype,
                                                        int target_rank,
                                                        MPI_Aint target_disp,
                                                        MPI_Aint target_count,
                                                        MPI_Datatype target_datatype, MPI_Op op,
                                                        MPIR_Win * win, MPIDI_winattr_t winattr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

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

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#endif /* POSIX_RMA_H_INCLUDED */
