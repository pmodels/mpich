/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined MCSMUTEX_H_INCLUDED
#define MCSMUTEX_H_INCLUDED

#include <mpi.h>

#define MCS_MUTEX_TAG 100

#ifdef ENABLE_DEBUG
#define debug_print(...) do { printf(__VA_ARGS__); } while (0)
#else
#define debug_print(...)
#endif

struct mcs_mutex_s {
    int tail_rank;
    MPI_Comm comm;
    MPI_Win window;
    int *base;
    MPI_Info win_info;
};

typedef struct mcs_mutex_s *MCS_Mutex;

#define MCS_MTX_ELEM_DISP 0
#define MCS_MTX_TAIL_DISP 1

int MCS_Mutex_create(int tail_rank, MPI_Comm comm, MCS_Mutex * hdl_out);
int MCS_Mutex_free(MCS_Mutex * hdl_ptr);
int MCS_Mutex_lock(MCS_Mutex hdl);
int MCS_Mutex_trylock(MCS_Mutex hdl, int *success);
int MCS_Mutex_unlock(MCS_Mutex hdl);

#endif /* MCSMUTEX_H_INCLUDED */
