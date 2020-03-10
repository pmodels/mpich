/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl_atomic.h"

#ifdef MPL_USE_LOCK_BASED_PRIMITIVES

/* FIXME For now we rely on pthreads for our IPC locks.  This is fairly
 * portable, although it is obviously not 100% portable.  We need to figure out
 * how to support other threading packages and lock implementations, such as the
 * BG/P lockbox. */

#include <pthread.h>

pthread_mutex_t MPL_emulation_lock = PTHREAD_MUTEX_INITIALIZER;
MPL_emulation_ipl_t *MPL_emulation_interprocess_lock = NULL;

int MPL_atomic_interprocess_lock_init(MPL_emulation_ipl_t * shm_lock, int isLeader)
{
    int mpi_errno = 0;          /* MPI_SUCCESS */
    pthread_mutexattr_t attr;

    /* Take the intranode-process lock. */
    pthread_mutex_lock(&MPL_emulation_lock);

    MPL_emulation_interprocess_lock = shm_lock;

    if (isLeader) {
        /* Set the mutex attributes to work correctly on inter-process
         * shared memory as well. This is required for some compilers
         * (such as SUN Studio) that don't enable it by default. */
        if (pthread_mutexattr_init(&attr) ||
            pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) ||
            pthread_mutex_init(MPL_emulation_interprocess_lock, &attr))
            mpi_errno = 16;     /* MPI_ERR_INTERN */
    }

    /* Release the intranode-process lock. */
    pthread_mutex_unlock(&MPL_emulation_lock);

    return mpi_errno;
}

int MPL_atomic_interprocess_lock_free(void)
{
    int mpi_errno = 0;          /* MPI_SUCCESS */

    /* Take the intranode-process lock. */
    pthread_mutex_lock(&MPL_emulation_lock);

    if (MPL_emulation_interprocess_lock) {
        if (pthread_mutex_destroy(MPL_emulation_interprocess_lock)) {
            mpi_errno = 16;
        } else {
            MPL_emulation_interprocess_lock = NULL;
        }
    }

    /* Release the intranode-process lock. */
    pthread_mutex_unlock(&MPL_emulation_lock);
    return mpi_errno;
}

#endif /* MPL_USE_LOCK_BASED_PRIMITIVES */
