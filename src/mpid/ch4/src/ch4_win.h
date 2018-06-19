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
#ifndef CH4_WIN_H_INCLUDED
#define CH4_WIN_H_INCLUDED

#include "ch4_impl.h"

#if defined(MPIDI_CH4_USE_WORK_QUEUES)
static inline void MPIDI_win_work_queues_init(MPIR_Win * win)
{
    win->dev.nqueues = 1;
    win->dev.work_queues =
        MPL_malloc(sizeof(MPIDI_workq_list_t) * win->dev.nqueues, MPL_MEM_BUFFER);

    MPIR_Assert(MPIDI_CH4_ENABLE_POBJ_WORKQUEUES);

    int i;
    for (i = 0; i < win->dev.nqueues; i++) {
        MPIDI_workq_init(&win->dev.work_queues[i].pend_ops);
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_locks[i]);
        DL_APPEND(MPIDI_CH4_Global.workqueues.pobj[i], &win->dev.work_queues[i]);
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_locks[i]);
    }
}

static inline void MPIDI_win_work_queues_free(MPIR_Win * win)
{
    int i;

    MPIR_Assert(MPIDI_CH4_ENABLE_POBJ_WORKQUEUES);

    for (i = 0; i < win->dev.nqueues; i++) {
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_locks[i]);
        DL_DELETE(MPIDI_CH4_Global.workqueues.pobj[i], &win->dev.work_queues[i]);
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_locks[i]);
    }

    MPL_free(win->dev.work_queues);
}

/* Get a work queue object pointer for the given VNI index */
#undef FUNCNAME
#define FUNCNAME MPIDI_win_vni_to_workq
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPIDI_workq_t *MPIDI_win_vni_to_workq(MPIR_Win * win_ptr, int vni_idx)
{
    if (MPIDI_CH4_ENABLE_POBJ_WORKQUEUES)
        return &win_ptr->dev.work_queues[vni_idx].pend_ops;
    else
        return &MPIDI_CH4_Global.workqueues.pvni[vni_idx];
}

#else
/* Empty definitions for non-workqueue builds */
static inline void MPIDI_win_work_queues_init(MPIR_Win * win)
{
}

static inline void MPIDI_win_work_queues_free(MPIR_Win * win)
{
}

MPL_STATIC_INLINE_PREFIX MPIDI_workq_t *MPIDI_win_vni_to_workq(MPIR_Win * win_ptr, int vni_idx)
{
}
#endif /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */

#undef FUNCNAME
#define FUNCNAME MPID_Win_set_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_SET_INFO);
    mpi_errno = MPIDI_NM_mpi_win_set_info(win, info);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_SET_INFO);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_start
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_START);
    mpi_errno = MPIDI_NM_mpi_win_start(group, assert, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_START);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_win_complete(int transport, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_WIN_COMPLETE);
    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            OPA_int_t processed;
            OPA_store_int(&processed, 0);
            MPIDI_CH4U_win_drain_queue(win);
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx), COMPLETE, NULL, 0,
                                    MPI_DATATYPE_NULL, NULL, 0, MPI_DATATYPE_NULL, 0, 0, 0,
                                    MPI_DATATYPE_NULL, MPI_OP_NULL, NULL, 0, 0, win, NULL,
                                    &processed);
            /* Busy loop to block until all RMA ops are completed. */
            while (!OPA_load_int(&processed));

            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_win_complete(win);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_WIN_COMPLETE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_complete(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_COMPLETE);
    mpi_errno = MPIDI_win_complete(MPIDI_CH4R_NETMOD, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_post
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_win_post(int transport, MPIR_Group * group,
                                            int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_POST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_WIN_POST);
    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            OPA_int_t processed;
            OPA_store_int(&processed, 0);
            /* Only group, assert, win, vni_idx and processed are required for post */
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx), POST, NULL, 0,
                                    MPI_DATATYPE_NULL, NULL, 0, MPI_DATATYPE_NULL, 0, 0, 0,
                                    MPI_DATATYPE_NULL, MPI_OP_NULL, group, 0, assert, win, NULL,
                                    &processed);
            /* Busy loop to block until all RMA ops are completed. */
            while (!OPA_load_int(&processed));

            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_win_post(group, assert, win);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_WIN_POST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_post
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_POST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_POST);
    mpi_errno = MPIDI_win_post(MPIDI_CH4R_NETMOD, group, assert, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_POST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_wait
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_wait(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_WAIT);
    mpi_errno = MPIDI_NM_mpi_win_wait(win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_WAIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_test(MPIR_Win * win, int *flag)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_TEST);
    mpi_errno = MPIDI_NM_mpi_win_test(win, flag);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_TEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_lock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_win_lock(int transport, int lock_type, int rank, int assert,
                                            MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_LOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_WIN_LOCK);
    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            OPA_int_t processed;
            OPA_store_int(&processed, 0);
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx), LOCK, NULL, 0,
                                    MPI_DATATYPE_NULL, NULL, 0, MPI_DATATYPE_NULL, rank, 0, 0,
                                    MPI_DATATYPE_NULL, MPI_OP_NULL, NULL, lock_type, assert, win,
                                    addr, &processed);
            /* Busy loop to block until all RMA ops are completed. */
            while (!OPA_load_int(&processed));

            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_win_lock(lock_type, rank, assert, win, addr);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_WIN_LOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_lock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_lock(int lock_type, int rank, int assert, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_LOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_LOCK);
    mpi_errno = MPIDI_win_lock(MPIDI_CH4R_NETMOD, lock_type, rank, assert, win, NULL);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_LOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_unlock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_win_unlock(int transport, int rank, MPIR_Win * win,
                                              MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_UNLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_WIN_UNLOCK);

    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            OPA_int_t processed;
            OPA_store_int(&processed, 0);
            MPIDI_CH4U_win_drain_queue(win);
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx), UNLOCK, NULL, 0,
                                    MPI_DATATYPE_NULL, NULL, 0, MPI_DATATYPE_NULL, rank, 0, 0,
                                    MPI_DATATYPE_NULL, MPI_OP_NULL, NULL, 0, 0, win,
                                    addr, &processed);
            /* Busy loop to block until all RMA ops are completed. */
            while (!OPA_load_int(&processed));

            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_win_unlock(rank, win, addr);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }
    MPIR_FUNC_VERBOSE_EXIT(MPIDI_STATE_MPID_WIN_UNLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_unlock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_unlock(int rank, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_UNLOCK);
    mpi_errno = MPIDI_win_unlock(MPIDI_CH4R_NETMOD, rank, win, NULL);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_get_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_GET_INFO);
    mpi_errno = MPIDI_NM_mpi_win_get_info(win, info_p_p);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_GET_INFO);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_FREE);

    if (MPIDI_CH4_ENABLE_POBJ_WORKQUEUES)
        MPIDI_win_work_queues_free(*win_ptr);

    mpi_errno = MPIDI_NM_mpi_win_free(win_ptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_FREE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_fence
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* Alternative Win_fence interface for multiple threading. */
MPL_STATIC_INLINE_PREFIX int MPIDI_win_fence(int transport, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_WIN_FENCE);
    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            /* Flush RMA operations. */
            OPA_int_t processed;
            OPA_store_int(&processed, 0);
            MPIDI_CH4U_win_drain_queue(win);
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx), FENCE_FLUSH, NULL, 0,
                                    MPI_DATATYPE_NULL, NULL, 0, MPI_DATATYPE_NULL, 0, 0, 0,
                                    MPI_DATATYPE_NULL, MPI_OP_NULL, NULL, 0, assert, win, NULL,
                                    &processed);
            /* Busy loop to block until all RMA ops are completed. */
            while (!OPA_load_int(&processed));

            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_win_fence_flush(assert, win);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }
    /* MPI Barrier */
    MPIR_Barrier(win->comm_ptr, &errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_WIN_FENCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_fence
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_fence(int assert, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_FENCE);
    mpi_errno = MPIDI_win_fence(MPIDI_CH4R_NETMOD, assert, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_FENCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_create(void *base,
                                             MPI_Aint length,
                                             int disp_unit,
                                             MPIR_Info * info, MPIR_Comm * comm_ptr,
                                             MPIR_Win ** win_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_CREATE);
    mpi_errno = MPIDI_NM_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    if (MPIDI_CH4_ENABLE_POBJ_WORKQUEUES)
        MPIDI_win_work_queues_init(*win_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_CREATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_attach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_ATTACH);
    mpi_errno = MPIDI_NM_mpi_win_attach(win, base, size);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_ATTACH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_allocate_shared
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_allocate_shared(MPI_Aint size,
                                                      int disp_unit,
                                                      MPIR_Info * info_ptr,
                                                      MPIR_Comm * comm_ptr,
                                                      void **base_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_ALLOCATE_SHARED);
    mpi_errno = MPIDI_NM_mpi_win_allocate_shared(size, disp_unit,
                                                 info_ptr, comm_ptr, base_ptr, win_ptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_ALLOCATE_SHARED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_flush_local
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_win_flush_local(int transport, int rank, MPIR_Win * win,
                                                   MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL);
    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            OPA_int_t processed;
            OPA_store_int(&processed, 0);
            MPIDI_CH4U_win_drain_queue(win);
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx), FLUSH_LOCAL, NULL, 0,
                                    MPI_DATATYPE_NULL, NULL, 0, MPI_DATATYPE_NULL, rank, 0, 0,
                                    MPI_DATATYPE_NULL, MPI_OP_NULL, NULL, 0, 0, win, addr,
                                    &processed);
            /* Busy loop to block until all RMA ops are completed. */
            while (!OPA_load_int(&processed));

            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_win_flush_local(rank, win, addr);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_flush_local
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_local(int rank, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FLUSH_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_FLUSH_LOCAL);
    mpi_errno = MPIDI_win_flush_local(MPIDI_CH4R_NETMOD, rank, win, NULL);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_FLUSH_LOCAL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_detach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_detach(MPIR_Win * win, const void *base)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_DETACH);
    mpi_errno = MPIDI_NM_mpi_win_detach(win, base);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_DETACH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_shared_query
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_shared_query(MPIR_Win * win,
                                                   int rank, MPI_Aint * size, int *disp_unit,
                                                   void *baseptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_SHARED_QUERY);
    mpi_errno = MPIDI_NM_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_SHARED_QUERY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_allocate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_allocate(MPI_Aint size,
                                               int disp_unit,
                                               MPIR_Info * info,
                                               MPIR_Comm * comm, void *baseptr, MPIR_Win ** win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_ALLOCATE);
    mpi_errno = MPIDI_NM_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    if (MPIDI_CH4_ENABLE_POBJ_WORKQUEUES)
        MPIDI_win_work_queues_init(*win);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_ALLOCATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_flush
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_win_flush(int transport, int rank, MPIR_Win * win,
                                             MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_FLUSH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_WIN_FLUSH);
    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            OPA_int_t processed;
            OPA_store_int(&processed, 0);
            MPIDI_CH4U_win_drain_queue(win);
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx), FLUSH, NULL, 0,
                                    MPI_DATATYPE_NULL, NULL, 0, MPI_DATATYPE_NULL, rank, 0, 0,
                                    MPI_DATATYPE_NULL, MPI_OP_NULL, NULL, 0, 0, win, addr,
                                    &processed);
            /* Busy loop to block until all RMA ops are completed. */
            while (!OPA_load_int(&processed));

            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_win_flush(rank, win, addr);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_WIN_FLUSH);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_flush
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_flush(int rank, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FLUSH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_FLUSH);
    mpi_errno = MPIDI_win_flush(MPIDI_CH4R_NETMOD, rank, win, NULL);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_FLUSH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_flush_local_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_win_flush_local_all(int transport, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL_ALL);
    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            OPA_int_t processed;
            OPA_store_int(&processed, 0);
            MPIDI_CH4U_win_drain_queue(win);
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx), FLUSH_LOCAL_ALL, NULL, 0,
                                    MPI_DATATYPE_NULL, NULL, 0, MPI_DATATYPE_NULL, 0, 0, 0,
                                    MPI_DATATYPE_NULL, MPI_OP_NULL, NULL, 0, 0, win, NULL,
                                    &processed);
            /* Busy loop to block until all RMA ops are completed. */
            while (!OPA_load_int(&processed));

            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_win_flush_local_all(win);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL_ALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_flush_local_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FLUSH_LOCAL_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_FLUSH_LOCAL_ALL);
    mpi_errno = MPIDI_win_flush_local_all(MPIDI_CH4R_NETMOD, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_FLUSH_LOCAL_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_unlock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_win_unlock_all(int transport, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_UNLOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_WIN_UNLOCK_ALL);
    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            OPA_int_t processed;
            OPA_store_int(&processed, 0);
            MPIDI_CH4U_win_drain_queue(win);
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx), UNLOCK_ALL, NULL, 0,
                                    MPI_DATATYPE_NULL, NULL, 0, MPI_DATATYPE_NULL, 0, 0, 0,
                                    MPI_DATATYPE_NULL, MPI_OP_NULL, NULL, 0, 0, win, NULL,
                                    &processed);
            /* Busy loop to block until all RMA ops are completed. */
            while (!OPA_load_int(&processed));

            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_win_unlock_all(win);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_WIN_UNLOCK_ALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_unlock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_unlock_all(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_UNLOCK_ALL);
    mpi_errno = MPIDI_win_unlock_all(MPIDI_CH4R_NETMOD, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_UNLOCK_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_create_dynamic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                     MPIR_Win ** win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_CREATE_DYNAMIC);
    mpi_errno = MPIDI_NM_mpi_win_create_dynamic(info, comm, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    if (MPIDI_CH4_ENABLE_POBJ_WORKQUEUES)
        MPIDI_win_work_queues_init(*win);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_CREATE_DYNAMIC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_sync
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_sync(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_SYNC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_SYNC);
    mpi_errno = MPIDI_NM_mpi_win_sync(win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_SYNC);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_flush_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_win_flush_all(int transport, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_FLUSH_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_WIN_FLUSH_ALL);
    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            OPA_int_t processed;
            OPA_store_int(&processed, 0);
            MPIDI_CH4U_win_drain_queue(win);
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx), FLUSH_ALL, NULL, 0,
                                    MPI_DATATYPE_NULL, NULL, 0, MPI_DATATYPE_NULL, 0, 0, 0,
                                    MPI_DATATYPE_NULL, MPI_OP_NULL, NULL, 0, 0, win, NULL,
                                    &processed);
            /* Busy loop to block until all RMA ops are completed. */
            while (!OPA_load_int(&processed));

            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_win_flush_all(win);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_WIN_FLUSH_ALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_flush_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_all(MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_FLUSH_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_FLUSH_ALL);
    mpi_errno = MPIDI_win_flush_all(MPIDI_CH4R_NETMOD, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_FLUSH_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_win_lock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_win_lock_all(int transport, int assert, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_WIN_LOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_WIN_LOCK_ALL);
    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);
        if (!cs_acq) {
            OPA_int_t processed;
            OPA_store_int(&processed, 0);
            MPIDI_workq_rma_enqueue(MPIDI_win_vni_to_workq(win, vni_idx), LOCK_ALL, NULL, 0,
                                    MPI_DATATYPE_NULL, NULL, 0, MPI_DATATYPE_NULL, 0, 0, 0,
                                    MPI_DATATYPE_NULL, MPI_OP_NULL, NULL, 0, assert, win, NULL,
                                    &processed);
            /* Busy loop to block until all RMA ops are completed. */
            while (!OPA_load_int(&processed));

            mpi_errno = MPI_SUCCESS;
        } else {
            mpi_errno = MPIDI_NM_mpi_win_lock_all(assert, win);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_WIN_LOCK_ALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_lock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Win_lock_all(int assert, MPIR_Win * win)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_WIN_LOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_WIN_LOCK_ALL);
    mpi_errno = MPIDI_win_lock_all(MPIDI_CH4R_NETMOD, assert, win);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_WIN_LOCK_ALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_WIN_H_INCLUDED */
