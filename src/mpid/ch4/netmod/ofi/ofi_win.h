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
#ifndef OFI_WIN_H_INCLUDED
#define OFI_WIN_H_INCLUDED

#include "ofi_impl.h"
#include <opa_primitives.h>

/* Blocking progress function to complete outstanding RMA operations on the input window.
 *
 * win - Window on which to complete operations
 * do_free - Flag to indicate whether it is safe to free the operations (whether this progress
 *           function is being called internally or by the user).
 */
static inline int MPIDI_OFI_win_progress_fence_impl(MPIR_Win * win, bool do_free)
{
    int mpi_errno = MPI_SUCCESS;
    int itercount = 0;
    int ret;
    uint64_t tcount, donecount;
    MPIDI_OFI_win_request_t *r;




    tcount = *MPIDI_OFI_WIN(win).issued_cntr;
    donecount = fi_cntr_read(MPIDI_OFI_WIN(win).cmpl_cntr);

    MPIR_Assert(donecount <= tcount);

    while (tcount > donecount) {
        MPIR_Assert(donecount <= tcount);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_global.vci_lock);
        MPIDI_OFI_PROGRESS();
        MPID_THREAD_CS_ENTER(VCI, MPIDI_global.vci_lock);
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

    r = MPIDI_OFI_WIN(win).syncQ;

    /* Should not free the RMA request when do_free = 0; manual RMA flush could happen in
     * the middle of issuing one MPI-level RMA op and it could lead to premature freeing of
     * the request. */
    if (do_free) {
        while (r) {
            MPIDI_OFI_win_request_t *next = r->next;
            MPIDI_OFI_rma_done_event(NULL, (MPIR_Request *) r);
            r = next;
        }

        MPIDI_OFI_WIN(win).syncQ = NULL;
        MPIDI_OFI_WIN(win).progress_counter = 1;
    }

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* If the OFI provider does not have automatic progress, check to see if the progress engine should
 * be manually triggered to keep performance from suffering when doing large, non-continuguous
 * transfers. */
static inline int MPIDI_OFI_win_trigger_rma_progress(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;




    if (!MPIDI_OFI_ENABLE_DATA_AUTO_PROGRESS && MPIR_CVAR_CH4_OFI_RMA_PROGRESS_INTERVAL != -1) {
        if (MPIDI_OFI_WIN(win).progress_counter % MPIR_CVAR_CH4_OFI_RMA_PROGRESS_INTERVAL == 0) {
            MPIDI_OFI_win_progress_fence_impl(win, false);
            MPIDI_OFI_WIN(win).progress_counter = 1;
        }
        MPIDI_OFI_WIN(win).progress_counter++;
    }



    return mpi_errno;
}

static inline int MPIDI_OFI_win_progress_fence(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;




    mpi_errno = MPIDI_OFI_win_progress_fence_impl(win, true);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;




    mpi_errno = MPIDIG_mpi_win_start(group, assert, win);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;




    mpi_errno = MPIDIG_mpi_win_complete(win);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;




    mpi_errno = MPIDIG_mpi_win_post(group, assert, win);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;




    mpi_errno = MPIDIG_mpi_win_wait(win);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    int mpi_errno = MPI_SUCCESS;




    mpi_errno = MPIDIG_mpi_win_test(win, flag);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert, MPIR_Win * win,
                                        MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;




    mpi_errno = MPIDIG_mpi_win_lock(lock_type, rank, assert, win);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;



    mpi_errno = MPIDIG_mpi_win_unlock(rank, win);


    return mpi_errno;

}

static inline int MPIDI_NM_mpi_win_fence(int massert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;



    mpi_errno = MPIDIG_mpi_win_fence(massert, win);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_shared_query(MPIR_Win * win,
                                                int rank,
                                                MPI_Aint * size, int *disp_unit, void *baseptr)
{
    int mpi_errno = MPI_SUCCESS;




    mpi_errno = MPIDIG_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;



    mpi_errno = MPIDIG_mpi_win_flush(rank, win);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;



    mpi_errno = MPIDIG_mpi_win_flush_local_all(win);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;



    mpi_errno = MPIDIG_mpi_win_unlock_all(win);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;



    mpi_errno = MPIDIG_mpi_win_flush_local(rank, win);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;



    mpi_errno = MPIDIG_mpi_win_sync(win);


    return mpi_errno;
}

static inline int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;



    mpi_errno = MPIDIG_mpi_win_flush_all(win);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;




    mpi_errno = MPIDIG_mpi_win_lock_all(assert, win);


    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;


    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        mpi_errno = MPIDI_OFI_win_progress_fence(win);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;


    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        mpi_errno = MPIDI_OFI_win_progress_fence(win);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_cmpl_hook(int rank ATTRIBUTE((unused)),
                                                           MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;


    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        mpi_errno = MPIDI_OFI_win_progress_fence(win);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_local_cmpl_hook(int rank ATTRIBUTE((unused)),
                                                                 MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;


    if (MPIDI_OFI_ENABLE_RMA) {
        /* network completion */
        mpi_errno = MPIDI_OFI_win_progress_fence(win);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* OFI_WIN_H_INCLUDED */
