/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2002 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_thread.h"

#if !defined(MPICH_IS_THREADED) || !defined(MPICH_TLS_SPECIFIER)
MPIUI_Per_thread_t MPIUI_Thread = { 0 };
#else
MPICH_TLS_SPECIFIER MPIUI_Per_thread_t MPIUI_Thread = { 0 };
#endif

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if defined(MPICH_IS_THREADED) && !defined(MPICH_TLS_SPECIFIER)

MPIU_Thread_tls_t MPIUI_Thread_storage;

/* This routine is called when a thread exits; it is passed the value
 * associated with the key.  In our case, this is simply storage
 * allocated with MPIU_Calloc */
void MPIUI_Cleanup_tls(void *a)
{
    if (a)
        MPIU_Free(a);
}

#endif
