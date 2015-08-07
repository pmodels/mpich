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


/* -- Begin Profiling Symbol Block for routine MPIX_Mutex_unlock */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Mutex_unlock = PMPIX_Mutex_unlock
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Mutex_unlock  MPIX_Mutex_unlock
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Mutex_unlock as PMPIX_Mutex_unlock
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Mutex_unlock(MPIX_Mutex hdl, int mutex, int proc) __attribute__((weak,alias("PMPIX_Mutex_unlock")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Mutex_unlock
#define MPIX_Mutex_unlock PMPIX_Mutex_unlock
#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Mutex_unlock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/** Unlock a mutex.
  *
  * @param[in] hdl   Mutex group that the mutex belongs to.
  * @param[in] mutex Desired mutex number [0..count-1]
  * @param[in] proc  Rank of process where the mutex lives
  * @return          MPI status
  */
int MPIX_Mutex_unlock(MPIX_Mutex hdl, int mutex, int proc)
{
    int rank, nproc, i;
    uint8_t *buf;

    assert(mutex >= 0 && mutex < hdl->max_count);

    MPI_Comm_rank(hdl->comm, &rank);
    MPI_Comm_size(hdl->comm, &nproc);

    assert(proc >= 0 && proc < nproc);

    buf = malloc(nproc * sizeof(uint8_t));
    assert(buf != NULL);

    buf[rank] = 0;

    /* Get all data from the lock_buf, except the byte belonging to
     * me. Set the byte belonging to me to 0. */
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

    assert(buf[rank] == 0);

    /* Notify the next waiting process, starting to my right for fairness */
    for (i = 1; i < nproc; i++) {
        int p = (rank + i) % nproc;
        if (buf[p] == 1) {
            debug_print("notifying %d [proc = %d, mutex = %d]\n", p, proc, mutex);
            MPI_Send(NULL, 0, MPI_BYTE, p, MPIX_MUTEX_TAG + mutex, hdl->comm);
            break;
        }
    }

    debug_print("lock released [proc = %d, mutex = %d]\n", proc, mutex);
    free(buf);

    return MPI_SUCCESS;
}
