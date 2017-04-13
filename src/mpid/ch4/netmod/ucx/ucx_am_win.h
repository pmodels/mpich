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
#ifndef UCX_AM_WIN_H_INCLUDED
#define UCX_AM_WIN_H_INCLUDED

#include "ucx_impl.h"

static inline int MPIDI_NM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);

    ret = MPIDI_CH4R_mpi_win_set_info(win, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);
    return ret;
}


static inline int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_START);

    ret = MPIDI_CH4R_mpi_win_start(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_START);
    return ret;
}


static inline int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);

    ret = MPIDI_CH4R_mpi_win_complete(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);
    return ret;
}

static inline int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_POST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_POST);

    ret = MPIDI_CH4R_mpi_win_post(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_POST);
    return ret;
}


static inline int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);

    ret = MPIDI_CH4R_mpi_win_wait(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);
    return ret;
}


static inline int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);

    ret = MPIDI_CH4R_mpi_win_test(win, flag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);
    return ret;
}

static inline int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);

    ret = MPIDI_CH4R_mpi_win_lock(lock_type, rank, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);
    return ret;
}


static inline int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);

    ret = MPIDI_CH4R_mpi_win_unlock(rank, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);
    return ret;
}

static inline int MPIDI_NM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);

    ret = MPIDI_CH4R_mpi_win_get_info(win, info_p_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);
    return ret;
}


static inline int MPIDI_NM_mpi_win_free(MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);

    ret = MPIDI_CH4R_mpi_win_free(win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);
    return ret;
}

static inline int MPIDI_NM_mpi_win_fence(int assert, MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);

    ret = MPIDI_CH4R_mpi_win_fence(assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);
    return ret;
}

static inline int MPIDI_NM_mpi_win_create(void *base,
                                          MPI_Aint length,
                                          int disp_unit,
                                          MPIR_Info * info, MPIR_Comm * comm_ptr,
                                          MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);

    ret = MPIDI_CH4R_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);
    return ret;
}

static inline int MPIDI_NM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);

    ret = MPIDI_CH4R_mpi_win_attach(win, base, size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);
    return ret;
}

static inline int MPIDI_NM_mpi_win_allocate_shared(MPI_Aint size,
                                                   int disp_unit,
                                                   MPIR_Info * info_ptr,
                                                   MPIR_Comm * comm_ptr,
                                                   void **base_ptr, MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);

    ret = MPIDI_CH4R_mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr, base_ptr,
                                             win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);
    return ret;
}

static inline int MPIDI_NM_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);

    ret = MPIDI_CH4R_mpi_win_detach(win, base);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);
    return ret;
}

static inline int MPIDI_NM_mpi_win_shared_query(MPIR_Win * win,
                                                int rank,
                                                MPI_Aint * size, int *disp_unit, void *baseptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);

    ret = MPIDI_CH4R_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);
    return ret;
}

static inline int MPIDI_NM_mpi_win_allocate(MPI_Aint size,
                                            int disp_unit,
                                            MPIR_Info * info,
                                            MPIR_Comm * comm, void *baseptr, MPIR_Win ** win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);

    ret = MPIDI_CH4R_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);
    return ret;
}

static inline int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);

    ret = MPIDI_CH4R_mpi_win_flush(rank, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);
    return ret;
}

static inline int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);

    ret = MPIDI_CH4R_mpi_win_flush_local_all(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);
    return ret;
}

static inline int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);

    ret = MPIDI_CH4R_mpi_win_unlock_all(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);
    return ret;
}

static inline int MPIDI_NM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                  MPIR_Win ** win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);

    ret = MPIDI_CH4R_mpi_win_create_dynamic(info, comm, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);
    return ret;
}

static inline int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);

    ret = MPIDI_CH4R_mpi_win_flush_local(rank, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);
    return ret;
}

static inline int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);

    ret = MPIDI_CH4R_mpi_win_sync(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);
    return ret;
}

static inline int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);

    ret = MPIDI_CH4R_mpi_win_flush_all(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);
    return ret;
}

static inline int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);

    ret = MPIDI_CH4R_mpi_win_lock_all(assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);
    return ret;
}


#endif /* UCX_AM_WIN_H_INCLUDED */
