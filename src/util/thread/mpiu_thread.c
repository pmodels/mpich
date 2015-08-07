/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2002 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_thread.h"

#if !defined(MPICH_IS_THREADED)

/* If single threaded, we preallocate this.  Otherwise, we create it */
MPICH_PerThread_t MPIU_Thread = { 0 };

#elif defined(MPIU_TLS_SPECIFIER)

MPIU_TLS_SPECIFIER MPICH_PerThread_t MPIU_Thread = { 0 };

#else  /* defined(MPICH_IS_THREADED) && !defined(MPIU_TLS_SPECIFIER) */

/* If we may be single threaded, we need a preallocated version to use
 * if we are single threaded case */
MPICH_PerThread_t MPIU_ThreadSingle = { 0 };

/* This routine is called when a thread exits; it is passed the value
 * associated with the key.  In our case, this is simply storage
 * allocated with MPIU_Calloc */
void MPIUI_Cleanup_tls( void *a )
{
    if (a)
	MPIU_Free( a );
}

#endif
