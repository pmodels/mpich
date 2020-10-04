/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_WIN_H_INCLUDED
#define OFI_WIN_H_INCLUDED

#include "ofi_impl.h"

/*
 * Blocking progress function to complete outstanding RMA operations on the input window.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_win_do_progress(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    int itercount = 0;
    int ret;
    uint64_t tcount, donecount;
    MPIDI_OFI_win_request_t *r;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_DO_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_DO_PROGRESS);

    while (1) {
        tcount = *MPIDI_OFI_WIN(win).issued_cntr;
        donecount = fi_cntr_read(MPIDI_OFI_WIN(win).cmpl_cntr);

        MPIR_Assert(donecount <= tcount);

        while (tcount > donecount) {
            MPIR_Assert(donecount <= tcount);
            MPIDI_OFI_PROGRESS(0);
            /* rma issued_cntr may be updated during MPIDI_OFI_PROGRESS if active messages
             * arrive and trigger RDMA calls, so we need to update it after progress call */
            tcount = *MPIDI_OFI_WIN(win).issued_cntr;
            donecount = fi_cntr_read(MPIDI_OFI_WIN(win).cmpl_cntr);
            itercount++;

            if (itercount == 1000) {
                ret = fi_cntr_wait(MPIDI_OFI_WIN(win).cmpl_cntr, tcount, 0);
                MPIDI_OFI_ERR(ret < 0 && ret != -FI_ETIMEDOUT,
                              mpi_errno,
                              MPI_ERR_RMA_RANGE,
                              "**ofid_cntr_wait",
                              "**ofid_cntr_wait %s %d %s %s",
                              __SHORT_FILE__, __LINE__, __func__, fi_strerror(-ret));
                itercount = 0;
            }
        }

        if (MPIDI_OFI_WIN(win).deferredQ) {
            MPIDI_OFI_issue_deferred_rma(win);
        } else {
            /* any/all deferred operations are complete */
            break;
        }
    }

    r = MPIDI_OFI_WIN(win).syncQ;
    while (r) {
        MPIDI_OFI_win_request_t *next = r->next;
        MPIR_Request **sigreq = r->sigreq;
        MPIDI_OFI_win_request_complete(r);
        MPIDI_OFI_sigreq_complete(sigreq);
        r = next;
    }
    MPIDI_OFI_WIN(win).syncQ = NULL;
    MPIDI_OFI_WIN(win).progress_counter = 1;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_DO_PROGRESS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* If the OFI provider does not have automatic progress, check to see if the progress engine should
 * be manually triggered to keep performance from suffering when doing large, non-continuguous
 * transfers. */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_win_trigger_rma_progress(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_WIN_TRIGGER_RMA_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_WIN_TRIGGER_RMA_PROGRESS);

    if (!MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS && MPIR_CVAR_CH4_OFI_RMA_PROGRESS_INTERVAL != -1) {
        if (MPIDI_OFI_WIN(win).progress_counter % MPIR_CVAR_CH4_OFI_RMA_PROGRESS_INTERVAL == 0) {
            MPIDI_OFI_win_do_progress(win);
            MPIDI_OFI_WIN(win).progress_counter = 1;
        }
        MPIDI_OFI_WIN(win).progress_counter++;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_WIN_TRIGGER_RMA_PROGRESS);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_START);

    mpi_errno = MPIDIG_mpi_win_start(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_START);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);

    mpi_errno = MPIDIG_mpi_win_complete(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_POST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_POST);

    mpi_errno = MPIDIG_mpi_win_post(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_POST);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);

    mpi_errno = MPIDIG_mpi_win_wait(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);

    mpi_errno = MPIDIG_mpi_win_test(win, flag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert,
                                                   MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);

    mpi_errno = MPIDIG_mpi_win_lock(lock_type, rank, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win,
                                                     MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);

    mpi_errno = MPIDIG_mpi_win_unlock(rank, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);
    return mpi_errno;

}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_fence(int massert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);

    mpi_errno = MPIDIG_mpi_win_fence(massert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_shared_query(MPIR_Win * win,
                                                           int rank,
                                                           MPI_Aint * size, int *disp_unit,
                                                           void *baseptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);

    mpi_errno = MPIDIG_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win,
                                                    MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);

    mpi_errno = MPIDIG_mpi_win_flush(rank, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);

    mpi_errno = MPIDIG_mpi_win_flush_local_all(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);

    mpi_errno = MPIDIG_mpi_win_unlock_all(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win,
                                                          MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);

    mpi_errno = MPIDIG_mpi_win_flush_local(rank, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);

    mpi_errno = MPIDIG_mpi_win_sync(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);

    mpi_errno = MPIDIG_mpi_win_flush_all(win);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);

    mpi_errno = MPIDIG_mpi_win_lock_all(assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_RMA_WIN_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_RMA_WIN_CMPL_HOOK);
    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        mpi_errno = MPIDI_OFI_win_do_progress(win);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_RMA_WIN_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_RMA_WIN_LOCAL_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_RMA_WIN_LOCAL_CMPL_HOOK);
    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        mpi_errno = MPIDI_OFI_win_do_progress(win);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_RMA_WIN_LOCAL_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_cmpl_hook(int rank ATTRIBUTE((unused)),
                                                           MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_RMA_TARGET_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_RMA_TARGET_CMPL_HOOK);
    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        mpi_errno = MPIDI_OFI_win_do_progress(win);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_RMA_TARGET_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_local_cmpl_hook(int rank ATTRIBUTE((unused)),
                                                                 MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_RMA_TARGET_LOCAL_CMPL_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_RMA_TARGET_LOCAL_CMPL_HOOK);
    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        mpi_errno = MPIDI_OFI_win_do_progress(win);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_RMA_TARGET_LOCAL_CMPL_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* OFI_WIN_H_INCLUDED */
