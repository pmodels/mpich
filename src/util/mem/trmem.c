/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Various helper functions add thread safety to the MPL_tr* functions.  These
 * have to be functions because we cannot portably wrap the calls as macros and
 * still use real (non-out-argument) return values. */

void MPIU_trinit(int rank)
{
    MPL_trinit(rank);
}

void MPIU_trdump(FILE *fp, int minid)
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    MPL_trdump(fp, minid);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

void *MPIU_trmalloc(unsigned int a, int lineno, const char fname[])
{
    void *retval;
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    retval = MPL_trmalloc(a, lineno, fname);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
    return retval;
}

void MPIU_trfree(void *a_ptr, int line, const char fname[])
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    MPL_trfree(a_ptr, line, fname);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

int MPIU_trvalid(const char str[])
{
    int retval;
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    retval = MPL_trvalid(str);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
    return retval;
}

void MPIU_trspace(int *space, int *fr)
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    MPL_trspace(space, fr);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

void MPIU_trid(int id)
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    MPL_trid(id);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

void MPIU_trlevel(int level)
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    MPL_trlevel(level);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

void MPIU_trDebugLevel(int level)
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    MPL_trDebugLevel(level);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

void *MPIU_trcalloc(unsigned int nelem, unsigned int elsize, int lineno, const char fname[])
{
    void *retval;
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    retval = MPL_trcalloc(nelem, elsize, lineno, fname);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
    return retval;
}

void *MPIU_trrealloc(void *p, int size, int lineno, const char fname[])
{
    void *retval;
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    retval = MPL_trrealloc(p, size, lineno, fname);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
    return retval;
}

void *MPIU_trstrdup(const char *str, int lineno, const char fname[])
{
    void *retval;
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    retval = MPL_trstrdup(str, lineno, fname);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
    return retval;
}

void MPIU_TrSetMaxMem(int size)
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    MPL_TrSetMaxMem(size);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

