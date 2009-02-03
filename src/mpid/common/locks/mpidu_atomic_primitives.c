/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpichconf.h>

/* FIXME For now we rely on pthreads for our IPC locks.  This is fairly
   portable, although it is obviously not 100% portable.  Some day when we
   refactor the MPIDU_Process_locks code we should be able to use that again. */
#if defined(HAVE_PTHREAD_H)
#include <pthread.h>
#include <mpidu_atomic_primitives.h>

/* FIXME This was an MPIDU_Process_lock, but the include path for that stuff is
 * really broken.  Besides, we can't use that once these atomics become a
 * separate library. */
pthread_mutex_t *MPIDU_Atomic_emulation_lock = NULL;

int MPIDU_Interprocess_lock_init(pthread_mutex_t *shm_lock, int isLeader)
{
    int mpi_errno = 0; /*MPI_SUCCESS*/
    MPIDU_Atomic_emulation_lock = shm_lock;

    if (isLeader) {
        pthread_mutex_init(MPIDU_Atomic_emulation_lock, NULL);
    }

    return mpi_errno;
}
#endif /* defined(HAVE_PTHREAD_H) */

