/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2002 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_thread.h"

#if !defined(MPICH_IS_THREADED)

/* If single threaded, we preallocate this.  Otherwise, we create it */
MPIUI_Per_thread_t MPIUI_Thread = { 0 };

#elif defined(MPICH_TLS_SPECIFIER)

MPICH_TLS_SPECIFIER MPIUI_Per_thread_t MPIUI_Thread = { 0 };

#else /* defined(MPICH_IS_THREADED) && !defined(MPICH_TLS_SPECIFIER) */

/* If we may be single threaded, we need a preallocated version to use
 * if we are single threaded case */
MPIUI_Per_thread_t MPIUI_ThreadSingle = { 0 };

/* This routine is called when a thread exits; it is passed the value
 * associated with the key.  In our case, this is simply storage
 * allocated with MPIU_Calloc */
void MPIUI_Cleanup_tls(void *a)
{
    if (a)
        MPIU_Free(a);
}

#endif
