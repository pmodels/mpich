/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>

#include <mpi.h>
#include "muteximpl.h"


/* -- Begin Profiling Symbol Block for routine MPIX_Mutex_lock */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Mutex_lock = PMPIX_Mutex_lock
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Mutex_lock  MPIX_Mutex_lock
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Mutex_lock as PMPIX_Mutex_lock
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Mutex_lock(MPIX_Mutex hdl, int mutex, int proc) __attribute__((weak,alias("PMPIX_Mutex_lock")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Mutex_lock
#define MPIX_Mutex_lock PMPIX_Mutex_lock
#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Mutex_lock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/** Lock a mutex.
  *
  * @param[in] hdl   Mutex group that the mutex belongs to
  * @param[in] mutex Desired mutex number [0..count-1]
  * @param[in] proc  Rank of process where the mutex lives
  * @return          MPI status
  */
int MPIX_Mutex_lock(MPIX_Mutex hdl, int mutex, int proc)
{
    int rank, nproc, already_locked, i;
    uint8_t *buf;

    assert(mutex >= 0 && mutex < hdl->max_count);

    MPI_Comm_rank(hdl->comm, &rank);
    MPI_Comm_size(hdl->comm, &nproc);

    assert(proc >= 0 && proc < nproc);

    buf = malloc(nproc * sizeof(uint8_t));
    assert(buf != NULL);

    buf[rank] = 1;

    /* Get all data from the lock_buf, except the byte belonging to
     * me. Set the byte belonging to me to 1. */
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, hdl->windows[mutex]);

    MPI_Put(&buf[rank], 1, MPI_BYTE, proc, rank, 1, MPI_BYTE, hdl->windows[mutex]);

    /* Get data to the left of rank */
    if (rank > 0) {
        MPI_Get(buf, rank, MPI_BYTE, proc, 0, rank, MPI_BYTE, hdl->windows[mutex]);
    }

    /* Get data to the right of rank */
    if (rank < nproc - 1) {
        MPI_Get(&buf[rank + 1], nproc - 1 - rank, MPI_BYTE, proc, rank + 1, nproc - 1 - rank,
                MPI_BYTE, hdl->windows[mutex]);
    }

    MPI_Win_unlock(proc, hdl->windows[mutex]);

    assert(buf[rank] == 1);

    for (i = already_locked = 0; i < nproc; i++)
        if (buf[i] && i != rank)
            already_locked = 1;

    /* Wait for notification */
    if (already_locked) {
        MPI_Status status;
        debug_print("waiting for notification [proc = %d, mutex = %d]\n", proc, mutex);
        MPI_Recv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE, MPIX_MUTEX_TAG + mutex, hdl->comm, &status);
    }

    debug_print("lock acquired [proc = %d, mutex = %d]\n", proc, mutex);
    free(buf);

    return MPI_SUCCESS;
}
