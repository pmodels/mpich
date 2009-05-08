/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpichconf.h"
#include "mpibase.h"
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

void MPIU_Exit(int exit_code)
{
#ifdef HAVE_WINDOWS_H
    /* exit can hang if libc fflushes output while in/out/err buffers are locked.  ExitProcess does not hang. */
    ExitProcess(exit_code);
#else
    exit(exit_code);
#endif
}
