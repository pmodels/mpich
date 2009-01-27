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

#include <mpiatomic.h>

/*
    These macros are analogous to the MPIDU_THREAD_XXX_CS_{ENTER,EXIT} macros.
    TODO Consider putting debugging macros in here that utilize 'msg'.
*/
#define MPIDU_IPC_SINGLE_CS_ENTER(msg)          \
    do {                                        \
        MPIU_Assert(emulation_lock);            \
        pthread_mutex_lock(emulation_lock);     \
    } while (0)

#define MPIDU_IPC_SINGLE_CS_EXIT(msg)           \
    do {                                        \
        MPIU_Assert(emulation_lock);            \
        pthread_mutex_unlock(emulation_lock);   \
    } while (0)


/* FIXME This was an MPIDU_Process_lock, but the include path for that stuff is
 * really broken.  Besides, we can't use that once these atomics become a
 * separate library. */
static pthread_mutex_t *emulation_lock = NULL;

int MPIDU_Interprocess_lock_init(pthread_mutex_t *shm_lock, int isLeader)
{
    int mpi_errno = MPI_SUCCESS;
    emulation_lock = shm_lock;

    if (isLeader) {
        pthread_mutex_init(emulation_lock, NULL);
    }

    return mpi_errno;
}
#endif /* defined(HAVE_PTHREAD_H) */

