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
#ifndef POSIX_WIN_H_INCLUDED
#define POSIX_WIN_H_INCLUDED

#include "posix_impl.h"

static inline int MPIDI_POSIX_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_SET_INFO);

    mpi_errno = MPIDI_NM_mpi_win_set_info(win, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_SET_INFO);

    return mpi_errno;
}


static inline int MPIDI_POSIX_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_START);

    mpi_errno = MPIDI_NM_mpi_win_start(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_START);

    return mpi_errno;
}


static inline int MPIDI_POSIX_mpi_win_complete(MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_COMPLETE);

    mpi_errno = MPIDI_NM_mpi_win_complete(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_COMPLETE);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_POST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_POST);

    mpi_errno = MPIDI_NM_mpi_win_post(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_POST);

    return mpi_errno;
}


static inline int MPIDI_POSIX_mpi_win_wait(MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_WAIT);

    mpi_errno = MPIDI_NM_mpi_win_wait(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_WAIT);

    return mpi_errno;
}


static inline int MPIDI_POSIX_mpi_win_test(MPIR_Win * win, int *flag)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_TEST);

    mpi_errno = MPIDI_NM_mpi_win_test(win, flag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_TEST);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_lock(int lock_type, int rank, int assert, MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_LOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_LOCK);

    mpi_errno = MPIDI_NM_mpi_win_lock(lock_type, rank, assert, win, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_LOCK);

    return mpi_errno;
}


static inline int MPIDI_POSIX_mpi_win_unlock(int rank, MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_UNLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_UNLOCK);

    mpi_errno = MPIDI_NM_mpi_win_unlock(rank, win, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_UNLOCK);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_GET_INFO);

    mpi_errno = MPIDI_NM_mpi_win_get_info(win, info_p_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_GET_INFO);

    return mpi_errno;
}


static inline int MPIDI_POSIX_mpi_win_free(MPIR_Win ** win_ptr)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_FREE);

    mpi_errno = MPIDI_NM_mpi_win_free(win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_FREE);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_fence(int assert, MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_FENCE);

    mpi_errno = MPIDI_NM_mpi_win_fence(assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_FENCE);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_create(void *base,
                                             MPI_Aint length,
                                             int disp_unit,
                                             MPIR_Info * info, MPIR_Comm * comm_ptr,
                                             MPIR_Win ** win_ptr)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE);

    mpi_errno = MPIDI_NM_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_ATTACH);

    mpi_errno = MPIDI_NM_mpi_win_attach(win, base, size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_ATTACH);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_allocate_shared(MPI_Aint size,
                                                      int disp_unit,
                                                      MPIR_Info * info_ptr,
                                                      MPIR_Comm * comm_ptr,
                                                      void **base_ptr, MPIR_Win ** win_ptr)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_SHARED);

    mpi_errno = MPIDI_NM_mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr, base_ptr,
                                                 win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE_SHARED);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_DETACH);

    mpi_errno = MPIDI_NM_mpi_win_detach(win, base);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_DETACH);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_shared_query(MPIR_Win * win,
                                                   int rank,
                                                   MPI_Aint * size, int *disp_unit, void *baseptr)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_SHARED_QUERY);

    mpi_errno = MPIDI_NM_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_SHARED_QUERY);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_allocate(MPI_Aint size,
                                               int disp_unit,
                                               MPIR_Info * info,
                                               MPIR_Comm * comm, void *baseptr, MPIR_Win ** win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE);

    mpi_errno = MPIDI_NM_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_ALLOCATE);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_flush(int rank, MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH);

    mpi_errno = MPIDI_NM_mpi_win_flush(rank, win, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_flush_local_all(MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH_LOCAL_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH_LOCAL_ALL);

    mpi_errno = MPIDI_NM_mpi_win_flush_local_all(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH_LOCAL_ALL);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_unlock_all(MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_UNLOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_UNLOCK_ALL);

    mpi_errno = MPIDI_NM_mpi_win_unlock_all(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_UNLOCK_ALL);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                     MPIR_Win ** win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_DYNAMIC);

    mpi_errno = MPIDI_NM_mpi_win_create_dynamic(info, comm, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_CREATE_DYNAMIC);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_flush_local(int rank, MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH_LOCAL);

    mpi_errno = MPIDI_NM_mpi_win_flush_local(rank, win, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH_LOCAL);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_sync(MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_SYNC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_SYNC);

    mpi_errno = MPIDI_NM_mpi_win_sync(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_SYNC);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_flush_all(MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH_ALL);

    mpi_errno = MPIDI_NM_mpi_win_flush_all(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_FLUSH_ALL);

    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_WIN_LOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_WIN_LOCK_ALL);

    mpi_errno = MPIDI_NM_mpi_win_lock_all(assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_WIN_LOCK_ALL);

    return mpi_errno;
}


#endif /* POSIX_WIN_H_INCLUDED */
