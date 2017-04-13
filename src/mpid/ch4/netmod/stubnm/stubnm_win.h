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
#ifndef STUBNM_WIN_H_INCLUDED
#define STUBNM_WIN_H_INCLUDED

#include "stubnm_impl.h"

static inline int MPIDI_NM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    return MPIDI_CH4R_mpi_win_set_info(win, info);
}


static inline int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_start(group, assert, win);
}


static inline int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_complete(win);
}

static inline int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_post(group, assert, win);
}


static inline int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_wait(win);
}


static inline int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    return MPIDI_CH4R_mpi_win_test(win, flag);
}

static inline int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert,
                                        MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    return MPIDI_CH4R_mpi_win_lock(lock_type, rank, assert, win);
}


static inline int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    return MPIDI_CH4R_mpi_win_unlock(rank, win);
}

static inline int MPIDI_NM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    return MPIDI_CH4R_mpi_win_get_info(win, info_p_p);
}


static inline int MPIDI_NM_mpi_win_free(MPIR_Win ** win_ptr)
{
    return MPIDI_CH4R_mpi_win_free(win_ptr);
}

static inline int MPIDI_NM_mpi_win_fence(int assert, MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_fence(assert, win);
}

static inline int MPIDI_NM_mpi_win_create(void *base,
                                          MPI_Aint length,
                                          int disp_unit,
                                          MPIR_Info * info, MPIR_Comm * comm_ptr,
                                          MPIR_Win ** win_ptr)
{
    return MPIDI_CH4R_mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);
}

static inline int MPIDI_NM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    return MPIDI_CH4R_mpi_win_attach(win, base, size);
}

static inline int MPIDI_NM_mpi_win_allocate_shared(MPI_Aint size,
                                                   int disp_unit,
                                                   MPIR_Info * info_ptr,
                                                   MPIR_Comm * comm_ptr,
                                                   void **base_ptr, MPIR_Win ** win_ptr)
{
    return MPIDI_CH4R_mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr, base_ptr,
                                              win_ptr);
}

static inline int MPIDI_NM_mpi_win_detach(MPIR_Win * win, const void *base)
{
    return MPIDI_CH4R_mpi_win_detach(win, base);
}

static inline int MPIDI_NM_mpi_win_shared_query(MPIR_Win * win,
                                                int rank,
                                                MPI_Aint * size, int *disp_unit, void *baseptr)
{
    return MPIDI_CH4R_mpi_win_shared_query(win, rank, size, disp_unit, baseptr);
}

static inline int MPIDI_NM_mpi_win_allocate(MPI_Aint size,
                                            int disp_unit,
                                            MPIR_Info * info,
                                            MPIR_Comm * comm, void *baseptr, MPIR_Win ** win)
{
    return MPIDI_CH4R_mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);
}

static inline int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    return MPIDI_CH4R_mpi_win_flush(rank, win);
}

static inline int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_flush_local_all(win);
}

static inline int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_unlock_all(win);
}

static inline int MPIDI_NM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                  MPIR_Win ** win)
{
    return MPIDI_CH4R_mpi_win_create_dynamic(info, comm, win);
}

static inline int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win,
                                               MPIDI_av_entry_t *addr)
{
    return MPIDI_CH4R_mpi_win_flush_local(rank, win);
}

static inline int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_sync(win);
}

static inline int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_flush_all(win);
}

static inline int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    return MPIDI_CH4R_mpi_win_lock_all(assert, win);
}


#endif /* STUBNM_WIN_H_INCLUDED */
