/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_WIN_H_INCLUDED
#define MPIDIG_WIN_H_INCLUDED

#include "ch4_types.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_RMA_MEM_EFFICIENT
      category    : CH4
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_GROUP_EQ
      description : >-
        If true, memory-saving mode is on, per-target object is released
        at the epoch end call.
        If false, performance-efficient mode is on, all allocated target
        objects are cached and freed at win_finalize.

    - name        : MPIR_CVAR_CH4_RMA_ENABLE_DYNAMIC_AM_PROGRESS
      category    : CH4
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If true, allows RMA synchronization calls to dynamically reduce the frequency
        of internal progress polling for incoming RMA active messages received on
        the target process. The RMA synchronization call initially polls progress
        with a low frequency (defined by MPIR_CVAR_CH4_RMA_AM_PROGRESS_LOW_FREQ_INTERVAL)
        to reduce synchronization overhead. Once any RMA active message has been
        received, it will always poll progress once at every synchronization call
        to ensure prompt target-side progress.
        Effective only for passive target synchronization MPI_Win_flush{_all}
        and MPI_Win_flush_local{_all}.

    - name        : MPIR_CVAR_CH4_RMA_AM_PROGRESS_INTERVAL
      category    : CH4
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies a static interval of progress polling for incoming RMA active messages
        received on the target process.
        Effective only for passive-target synchronization MPI_Win_flush{_all} and
        MPI_Win_flush_local{_all}. Interval indicates the number of performed
        flush calls before polling. It is counted globally across all windows.
        Invalid when MPIR_CVAR_CH4_RMA_ENABLE_DYNAMIC_AM_PROGRESS is true.

    - name        : MPIR_CVAR_CH4_RMA_AM_PROGRESS_LOW_FREQ_INTERVAL
      category    : CH4
      type        : int
      default     : 100
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the interval of progress polling with low frequency for
        incoming RMA active message received on the target process.
        Effective only for passive-target synchronization MPI_Win_flush{_all} and
        MPI_Win_flush_local{_all}. Interval indicates the number of performed
        flush calls before polling. It is counted globally across all windows.
        Used when MPIR_CVAR_CH4_RMA_ENABLE_DYNAMIC_AM_PROGRESS is true.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_winlock_getlocallock ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_wincreate_allgather ATTRIBUTE((unused));
extern MPIR_T_pvar_timer_t PVAR_TIMER_rma_amhdr_set ATTRIBUTE((unused));

int MPIDIG_RMA_Init_sync_pvars(void);
int MPIDIG_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info);
int MPIDIG_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p);
int MPIDIG_mpi_win_free(MPIR_Win ** win_ptr);
int MPIDIG_mpi_win_create(void *base, MPI_Aint length, int disp_unit, MPIR_Info * info,
                          MPIR_Comm * comm_ptr, MPIR_Win ** win_ptr);
int MPIDIG_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size);
int MPIDIG_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPIR_Info * info_ptr,
                                   MPIR_Comm * comm_ptr, void **base_ptr, MPIR_Win ** win_ptr);
int MPIDIG_mpi_win_detach(MPIR_Win * win, const void *base);
int MPIDIG_mpi_win_allocate(MPI_Aint size, int disp_unit, MPIR_Info * info,
                            MPIR_Comm * comm, void *baseptr, MPIR_Win ** win_ptr);
int MPIDIG_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm, MPIR_Win ** win_ptr);

MPL_STATIC_INLINE_PREFIX int MPIDIG_fill_ranks_in_win_grp(MPIR_Win * win_ptr,
                                                          MPIR_Group * group_ptr,
                                                          int *ranks_in_win_grp)
{
    int mpi_errno = MPI_SUCCESS;
    int i, *ranks_in_grp = NULL;
    MPIR_Group *win_grp_ptr;

    MPIR_FUNC_ENTER;

    ranks_in_grp = (int *) MPL_malloc(group_ptr->size * sizeof(int), MPL_MEM_RMA);
    MPIR_Assert(ranks_in_grp);
    for (i = 0; i < group_ptr->size; i++)
        ranks_in_grp[i] = i;

    mpi_errno = MPIR_Comm_group_impl(win_ptr->comm_ptr, &win_grp_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Group_translate_ranks_impl(group_ptr, group_ptr->size,
                                                ranks_in_grp, win_grp_ptr, ranks_in_win_grp);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Group_free_impl(win_grp_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPL_free(ranks_in_grp);

    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    MPIDIG_ACCESS_EPOCH_CHECK_NONE(win, mpi_errno, goto fn_fail);

    MPIR_Group_add_ref(group);
    if (assert & MPI_MODE_NOCHECK) {
        goto no_check;
    }

    MPIDIU_PROGRESS_WHILE(group->size != (int) MPIDIG_WIN(win, sync).pw.count, vci);
  no_check:
    MPIDIG_WIN(win, sync).pw.count = 0;

    MPIR_ERR_CHKANDJUMP((MPIDIG_WIN(win, sync).sc.group != NULL),
                        mpi_errno, MPI_ERR_GROUP, "**group");
    MPIDIG_WIN(win, sync).sc.group = group;
    MPIDIG_WIN(win, sync).access_epoch_type = MPIDIG_EPOTYPE_START;

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_complete(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_win_cntrl_msg_t msg;
    int win_grp_idx, peer;
    MPIR_Group *group;
    int *ranks_in_win_grp = NULL;
    int need_unlock = 0;

    MPIR_FUNC_ENTER;

    MPIDIG_ACCESS_EPOCH_CHECK(win, MPIDIG_EPOTYPE_START, mpi_errno, return mpi_errno);

    group = MPIDIG_WIN(win, sync).sc.group;
    MPIR_Assert(group != NULL);

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_win_cmpl_hook(win);
    MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_win_cmpl_hook(win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    need_unlock = 1;

    msg.win_id = MPIDIG_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;

    /* Ensure completion of AM operations */
    ranks_in_win_grp = (int *) MPL_malloc(sizeof(int) * group->size, MPL_MEM_RMA);
    MPIR_Assert(ranks_in_win_grp);

    mpi_errno = MPIDIG_fill_ranks_in_win_grp(win, group, ranks_in_win_grp);
    MPIR_ERR_CHECK(mpi_errno);

    /* FIXME: now we simply set per-target counters for PSCW, can it be optimized ? */
    MPIDIU_PROGRESS_DO_WHILE(!MPIDIG_win_check_group_local_completed
                             (win, ranks_in_win_grp, group->size), vci);

    for (win_grp_idx = 0; win_grp_idx < group->size; ++win_grp_idx) {
        peer = ranks_in_win_grp[win_grp_idx];

        int vci_target = MPIDI_WIN_TARGET_VCI(win, peer);
        CH4_CALL(am_send_hdr
                 (peer, win->comm_ptr, MPIDIG_WIN_COMPLETE, &msg, sizeof(msg), vci, vci_target),
                 MPIDI_rank_is_local(peer, win->comm_ptr), mpi_errno);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

    /* In performance-efficient mode, all allocated targets are freed at win_finalize. */
    if (MPIR_CVAR_CH4_RMA_MEM_EFFICIENT)
        MPIDIG_win_target_cleanall(win);
    MPIDIG_WIN(win, sync).access_epoch_type = MPIDIG_EPOTYPE_NONE;
    MPIR_Group_release(MPIDIG_WIN(win, sync).sc.group);
    MPIDIG_WIN(win, sync).sc.group = NULL;

  fn_exit:
    MPL_free(ranks_in_win_grp);

    if (need_unlock) {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_win_cntrl_msg_t msg;
    int win_grp_idx, peer;
    int *ranks_in_win_grp = NULL;

    MPIR_FUNC_ENTER;
    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    MPIDIG_EXPOSURE_EPOCH_CHECK_NONE(win, mpi_errno, goto fn_fail);

    MPIR_Group_add_ref(group);
    MPIR_ERR_CHKANDJUMP((MPIDIG_WIN(win, sync).pw.group != NULL),
                        mpi_errno, MPI_ERR_GROUP, "**group");

    MPIDIG_WIN(win, sync).pw.group = group;
    MPIR_Assert(group != NULL);
    if (assert & MPI_MODE_NOCHECK) {
        goto no_check;
    }

    msg.win_id = MPIDIG_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;

    ranks_in_win_grp = (int *) MPL_malloc(sizeof(int) * group->size, MPL_MEM_RMA);
    MPIR_Assert(ranks_in_win_grp);

    mpi_errno = MPIDIG_fill_ranks_in_win_grp(win, group, ranks_in_win_grp);
    MPIR_ERR_CHECK(mpi_errno);

    for (win_grp_idx = 0; win_grp_idx < group->size; ++win_grp_idx) {
        peer = ranks_in_win_grp[win_grp_idx];
        int vci_target = MPIDI_WIN_TARGET_VCI(win, peer);
        CH4_CALL(am_send_hdr
                 (peer, win->comm_ptr, MPIDIG_WIN_POST, &msg, sizeof(msg), vci, vci_target),
                 MPIDI_rank_is_local(peer, win->comm_ptr), mpi_errno);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

  no_check:
    MPIDIG_WIN(win, sync).exposure_epoch_type = MPIDIG_EPOTYPE_POST;
  fn_exit:
    MPL_free(ranks_in_win_grp);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_wait(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *group;

    MPIR_FUNC_ENTER;
    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    MPIDIG_EXPOSURE_EPOCH_CHECK(win, MPIDIG_EPOTYPE_POST, mpi_errno, goto fn_fail);
    group = MPIDIG_WIN(win, sync).pw.group;
    MPIDIU_PROGRESS_WHILE(group->size != (int) MPIDIG_WIN(win, sync).sc.count, vci);

    MPIDIG_WIN(win, sync).sc.count = 0;
    MPIDIG_WIN(win, sync).pw.group = NULL;
    MPIR_Group_release(group);
    MPIDIG_WIN(win, sync).exposure_epoch_type = MPIDIG_EPOTYPE_NONE;

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_test(MPIR_Win * win, int *flag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    int vci = MPIDI_WIN(win, am_vci);
#endif
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    MPIDIG_EXPOSURE_EPOCH_CHECK(win, MPIDIG_EPOTYPE_POST, mpi_errno, goto fn_fail);

    MPIR_Group *group;
    group = MPIDIG_WIN(win, sync).pw.group;

    if (group->size == (int) MPIDIG_WIN(win, sync).sc.count) {
        MPIDIG_WIN(win, sync).sc.count = 0;
        MPIDIG_WIN(win, sync).pw.group = NULL;
        *flag = 1;
        MPIR_Group_release(group);
        MPIDIG_WIN(win, sync).exposure_epoch_type = MPIDIG_EPOTYPE_NONE;
    } else {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
        mpi_errno = MPID_Progress_test(NULL);
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
        *flag = 0;
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_lock(int lock_type, int rank, int assert,
                                                 MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned locked;

    MPIR_FUNC_ENTER;
    int vci = MPIDI_WIN(win, am_vci);
    int vci_target = MPIDI_WIN_TARGET_VCI(win, rank);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    MPIDIG_LOCK_EPOCH_CHECK_NONE(win, rank, mpi_errno, goto fn_fail);

    MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_get(win, rank);

    MPIDIG_win_target_sync_lock_t *slock = &target_ptr->sync.lock;
    MPIR_Assert(slock->locked == 0);
    if (assert & MPI_MODE_NOCHECK) {
        target_ptr->sync.assert_mode |= MPI_MODE_NOCHECK;
        slock->locked = 1;
        goto no_check;
    }

    MPIDIG_win_cntrl_msg_t msg;
    msg.win_id = MPIDIG_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;
    msg.lock_type = lock_type;

    locked = slock->locked + 1;
    CH4_CALL(am_send_hdr(rank, win->comm_ptr, MPIDIG_WIN_LOCK, &msg, sizeof(msg), vci, vci_target),
             MPIDI_rank_is_local(rank, win->comm_ptr), mpi_errno);

    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");

    MPIDIU_PROGRESS_WHILE(slock->locked != locked, vci);
  no_check:
    target_ptr->sync.access_epoch_type = MPIDIG_EPOTYPE_LOCK;

    MPIDIG_WIN(win, sync).access_epoch_type = MPIDIG_EPOTYPE_LOCK;
    MPIDIG_WIN(win, sync).lock.count++;

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_unlock(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned unlocked;
    MPIDIG_win_cntrl_msg_t msg;
    int need_unlock = 0;

    MPIR_FUNC_ENTER;

    /* Check window lock epoch. */
    MPIDIG_ACCESS_EPOCH_CHECK(win, MPIDIG_EPOTYPE_LOCK, mpi_errno, return mpi_errno);

    MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, rank);
    MPIR_Assert(target_ptr);

    /* Check per-target lock epoch */
    MPIDIG_EPOCH_CHECK_TARGET_LOCK(target_ptr, mpi_errno, return mpi_errno);

    MPIDIG_win_target_sync_lock_t *slock = &target_ptr->sync.lock;
    /* NOTE: lock blocking waits till granted */
    MPIR_Assert(slock->locked == 1);

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_target_cmpl_hook(rank, win);
    MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_target_cmpl_hook(rank, win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    int vci = MPIDI_WIN(win, am_vci);
    int vci_target = MPIDI_WIN_TARGET_VCI(win, rank);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    need_unlock = 1;

    /* Ensure completion of AM operations */
    MPIDIU_PROGRESS_DO_WHILE(MPIR_cc_get(target_ptr->remote_cmpl_cnts) != 0 ||
                             MPIR_cc_get(target_ptr->remote_acc_cmpl_cnts) != 0, vci);

    if (target_ptr->sync.assert_mode & MPI_MODE_NOCHECK) {
        target_ptr->sync.lock.locked = 0;
        goto no_check;
    }

    msg.win_id = MPIDIG_WIN(win, win_id);
    msg.origin_rank = win->comm_ptr->rank;
    unlocked = slock->locked - 1;

    CH4_CALL(am_send_hdr
             (rank, win->comm_ptr, MPIDIG_WIN_UNLOCK, &msg, sizeof(msg), vci, vci_target),
             MPIDI_rank_is_local(rank, win->comm_ptr), mpi_errno);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");

    MPIDIU_PROGRESS_WHILE(slock->locked != unlocked, vci);
  no_check:
    /* In performance-efficient mode, all allocated targets are freed at win_finalize. */
    if (MPIR_CVAR_CH4_RMA_MEM_EFFICIENT)
        MPIDIG_win_target_delete(win, target_ptr);

    MPIR_Assert(MPIDIG_WIN(win, sync).lock.count > 0);
    MPIDIG_WIN(win, sync).lock.count--;

    /* Reset window epoch only when all per-target lock epochs are closed. */
    if (MPIDIG_WIN(win, sync).lock.count == 0) {
        MPIDIG_WIN(win, sync).access_epoch_type = MPIDIG_EPOTYPE_NONE;
    }

  fn_exit:
    if (need_unlock) {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_fence(int massert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int need_unlock = 0;
    MPIR_FUNC_ENTER;

    MPIDIG_FENCE_EPOCH_CHECK(win, mpi_errno, goto fn_fail);

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_win_cmpl_hook(win);
    MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_win_cmpl_hook(win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    need_unlock = 1;

    /* Ensure completion of AM operations */
    MPIDIU_PROGRESS_DO_WHILE(MPIR_cc_get(MPIDIG_WIN(win, local_cmpl_cnts)) != 0 ||
                             MPIR_cc_get(MPIDIG_WIN(win, remote_acc_cmpl_cnts)) != 0, vci);
    MPIDIG_EPOCH_FENCE_EVENT(win, massert);

    /*
     * We always make a barrier even if MPI_MODE_NOPRECEDE is specified.
     * This is necessary because we no longer defer executions of RMA ops
     * until synchronization calls as CH3 did. Otherwise, the code like
     * this won't work correctly (cf. f77/rma/wingetf)
     *
     * Rank 0                          Rank 1
     * ----                            ----
     * Store to local mem in window
     * MPI_Win_fence(MODE_NOPRECEDE)   MPI_Win_fence(MODE_NOPRECEDE)
     * MPI_Get(from rank 1)
     */
    /* MPIR_Barrier's state is protected by ALLFUNC_MUTEX.
     * In VCI granularity, individual send/recv/wait operations will take
     * the VCI lock internally. */
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    need_unlock = 0;
    mpi_errno = MPIR_Barrier(win->comm_ptr, &errflag);

  fn_exit:
    if (need_unlock) {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_shared_query(MPIR_Win * win, int rank, MPI_Aint * size,
                                                         int *disp_unit, void *baseptr)
{
    int mpi_errno = MPI_SUCCESS;
    int offset = rank;
    MPIDIG_win_shared_info_t *shared_table = MPIDIG_WIN(win, shared_table);

    MPIR_FUNC_ENTER;

    /* When only single process exists on the node or shared memory allocation fails,
     * should only query MPI_PROC_NULL or local process. Thus, return local window's info. */
    if (win->comm_ptr->node_comm == NULL || !shared_table) {
        *size = win->size;
        *disp_unit = win->disp_unit;
        *((void **) baseptr) = win->base;
        goto fn_exit;
    }

    /* When rank is MPI_PROC_NULL, return the memory region belonging the lowest
     * rank that specified size > 0*/
    if (rank == MPI_PROC_NULL) {
        /* Default, if no process has size > 0. */
        *size = 0;
        *disp_unit = 0;
        *((void **) baseptr) = NULL;

        for (offset = 0; offset < win->comm_ptr->local_size; offset++) {
            if (shared_table[offset].size > 0) {
                *size = shared_table[offset].size;
                *disp_unit = shared_table[offset].disp_unit;
                *((void **) baseptr) = shared_table[offset].shm_base_addr;
                break;
            }
        }
    } else {
        *size = shared_table[offset].size;
        *disp_unit = shared_table[offset].disp_unit;
        *(void **) baseptr = shared_table[offset].shm_base_addr;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_flush(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int need_unlock = 0;
    MPIR_FUNC_ENTER;

    /* Check window lock epoch. */
    MPIDIG_EPOCH_CHECK_PASSIVE(win, mpi_errno, return mpi_errno);

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_target_cmpl_hook(rank, win);
    MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_target_cmpl_hook(rank, win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    need_unlock = 1;

    /* Ensure completion of AM operations issued to the target.
     * If target object is not created (e.g., when all operations issued
     * to the target were via shm and in lockall), we also need trigger
     * progress once to handle remote AM. */
    MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, rank);
    if (target_ptr) {
        if (MPIDIG_WIN(win, sync).access_epoch_type == MPIDIG_EPOTYPE_LOCK)
            MPIDIG_EPOCH_CHECK_TARGET_LOCK(target_ptr, mpi_errno, goto fn_fail);
    }

    int poll_once = MPIDIG_rma_need_poll_am()? 1 : 0;
    MPIDIU_PROGRESS_WHILE((target_ptr && (MPIR_cc_get(target_ptr->remote_cmpl_cnts) != 0 ||
                                          MPIR_cc_get(target_ptr->remote_acc_cmpl_cnts) != 0)) ||
                          poll_once-- > 0, vci);

  fn_exit:
    if (need_unlock) {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int need_unlock = 0;
    MPIR_FUNC_ENTER;

    MPIDIG_EPOCH_CHECK_PASSIVE(win, mpi_errno, goto fn_fail);

    /* Ensure op local completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_win_local_cmpl_hook(win);
    MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_win_local_cmpl_hook(win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    need_unlock = 1;

    /* Ensure completion of AM operations */

    /* FIXME: now we simply set per-target counters for lockall in case
     * user flushes per target, but this should be optimized. */
    int poll_once = MPIDIG_rma_need_poll_am()? 1 : 0;

    MPIDIU_PROGRESS_WHILE((!MPIDIG_win_check_all_targets_local_completed(win) || poll_once-- > 0),
                          vci);

  fn_exit:
    if (need_unlock) {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_unlock_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int need_unlock = 0;
    MPIR_FUNC_ENTER;
    int i;

    MPIDIG_ACCESS_EPOCH_CHECK(win, MPIDIG_EPOTYPE_LOCK_ALL, mpi_errno, return mpi_errno);
    /* NOTE: lockall blocking waits till all locks granted */
    MPIR_Assert(MPIDIG_WIN(win, sync).lockall.allLocked == win->comm_ptr->local_size);

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_win_cmpl_hook(win);
    MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_win_cmpl_hook(win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    need_unlock = 1;

    /* Ensure completion of AM operations */

    /* FIXME: now we simply set per-target counters for lockall in case
     * user flushes per target, but this should be optimized. */
    MPIDIU_PROGRESS_DO_WHILE(!MPIDIG_win_check_all_targets_remote_completed(win), vci);

    if (MPIDIG_WIN(win, sync).assert_mode & MPI_MODE_NOCHECK) {
        MPIDIG_WIN(win, sync).lockall.allLocked = 0;
        goto no_check;
    }
    for (i = 0; i < win->comm_ptr->local_size; i++) {
        MPIDIG_win_cntrl_msg_t msg;
        msg.win_id = MPIDIG_WIN(win, win_id);
        msg.origin_rank = win->comm_ptr->rank;

        int vci_target = MPIDI_WIN_TARGET_VCI(win, i);
        CH4_CALL(am_send_hdr
                 (i, win->comm_ptr, MPIDIG_WIN_UNLOCKALL, &msg, sizeof(msg), vci, vci_target),
                 MPIDI_rank_is_local(i, win->comm_ptr), mpi_errno);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

    MPIDIU_PROGRESS_WHILE(MPIDIG_WIN(win, sync).lockall.allLocked, vci);
  no_check:
    /* In performance-efficient mode, all allocated targets are freed at win_finalize. */
    if (MPIR_CVAR_CH4_RMA_MEM_EFFICIENT)
        MPIDIG_win_target_cleanall(win);
    MPIDIG_WIN(win, sync).access_epoch_type = MPIDIG_EPOTYPE_NONE;
    MPIDIG_WIN(win, sync).assert_mode = 0;

  fn_exit:
    if (need_unlock) {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_flush_local(int rank, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int need_unlock = 0;
    MPIR_FUNC_ENTER;

    /* Check window lock epoch. */
    MPIDIG_EPOCH_CHECK_PASSIVE(win, mpi_errno, return mpi_errno);

    /* Ensure op local completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_target_local_cmpl_hook(rank, win);
    MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_target_local_cmpl_hook(rank, win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    need_unlock = 1;
    /* Ensure completion of AM operations issued to the target.
     * If target object is not created (e.g., when all operations issued
     * to the target were via shm and in lockall), we also need trigger
     * progress once to handle remote AM. */
    MPIDIG_win_target_t *target_ptr = MPIDIG_win_target_find(win, rank);
    if (target_ptr) {
        if (MPIDIG_WIN(win, sync).access_epoch_type == MPIDIG_EPOTYPE_LOCK)
            MPIDIG_EPOCH_CHECK_TARGET_LOCK(target_ptr, mpi_errno, goto fn_fail);
    }

    int poll_once = MPIDIG_rma_need_poll_am()? 1 : 0;
    MPIDIU_PROGRESS_WHILE(((target_ptr && MPIR_cc_get(target_ptr->local_cmpl_cnts) != 0) ||
                           poll_once-- > 0), vci);

  fn_exit:
    if (need_unlock) {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_sync(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDIG_EPOCH_CHECK_PASSIVE(win, mpi_errno, goto fn_fail);
    MPL_atomic_read_write_barrier();

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_flush_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int need_unlock = 0;
    MPIR_FUNC_ENTER;

    MPIDIG_EPOCH_CHECK_PASSIVE(win, mpi_errno, goto fn_fail);

    /* Ensure op completion in netmod and shmmod */
    mpi_errno = MPIDI_NM_rma_win_cmpl_hook(win);
    MPIR_ERR_CHECK(mpi_errno);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_rma_win_cmpl_hook(win);
    MPIR_ERR_CHECK(mpi_errno);
#endif

    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    need_unlock = 1;

    /* Ensure completion of AM operations */

    /* FIXME: now we simply set per-target counters for lockall in case
     * user flushes per target, but this should be optimized. */
    int poll_once = MPIDIG_rma_need_poll_am()? 1 : 0;

    MPIDIU_PROGRESS_WHILE((!MPIDIG_win_check_all_targets_remote_completed(win) || poll_once-- > 0),
                          vci);

  fn_exit:
    if (need_unlock) {
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;
    int vci = MPIDI_WIN(win, am_vci);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    MPIDIG_ACCESS_EPOCH_CHECK_NONE(win, mpi_errno, goto fn_fail);

    MPIR_Assert(MPIDIG_WIN(win, sync).lockall.allLocked == 0);

    int size;
    size = win->comm_ptr->local_size;
    if (assert & MPI_MODE_NOCHECK) {
        MPIDIG_WIN(win, sync).assert_mode |= MPI_MODE_NOCHECK;
        MPIDIG_WIN(win, sync).lockall.allLocked = size;
        goto no_check;
    }

    int i;
    for (i = 0; i < size; i++) {
        MPIDIG_win_cntrl_msg_t msg;
        msg.win_id = MPIDIG_WIN(win, win_id);
        msg.origin_rank = win->comm_ptr->rank;
        msg.lock_type = MPI_LOCK_SHARED;

        int vci_target = MPIDI_WIN_TARGET_VCI(win, i);
        CH4_CALL(am_send_hdr
                 (i, win->comm_ptr, MPIDIG_WIN_LOCKALL, &msg, sizeof(msg), vci, vci_target),
                 MPIDI_rank_is_local(i, win->comm_ptr), mpi_errno);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, goto fn_fail, "**rmasync");
    }

    MPIDIU_PROGRESS_WHILE(size != (int) MPIDIG_WIN(win, sync).lockall.allLocked, vci);
  no_check:
    MPIDIG_WIN(win, sync).access_epoch_type = MPIDIG_EPOTYPE_LOCK_ALL;

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPIDIG_WIN_H_INCLUDED */
