/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Various helper functions add thread safety to the MPL_tr* functions.  These
 * have to be functions because we cannot portably wrap the calls as macros and
 * still use real (non-out-argument) return values. */

void MPIU_trinit(int rank, int need_thread_safety)
{
    MPL_trinit(rank, need_thread_safety);
}

void MPIU_trdump(FILE *fp, int minid)
{
    MPL_trdump(fp, minid);
}

void *MPIU_trmalloc(size_t a, int lineno, const char fname[])
{
    return MPL_trmalloc(a, lineno, fname);
}

void MPIU_trfree(void *a_ptr, int line, const char fname[])
{
    MPL_trfree(a_ptr, line, fname);
}

int MPIU_trvalid(const char str[])
{
    return MPL_trvalid(str);
}

void *MPIU_trcalloc(size_t nelem, size_t elsize, int lineno, const char fname[])
{
    return MPL_trcalloc(nelem, elsize, lineno, fname);
}

void *MPIU_trrealloc(void *p, size_t size, int lineno, const char fname[])
{
    return MPL_trrealloc(p, size, lineno, fname);
}

void *MPIU_trstrdup(const char *str, int lineno, const char fname[])
{
    return MPL_trstrdup(str, lineno, fname);
}
