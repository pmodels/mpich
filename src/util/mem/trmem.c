/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
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
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    MPL_trdump(fp, minid);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
}

void *MPIU_trmalloc(size_t a, int lineno, const char fname[])
{
    void *retval;
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    retval = MPL_trmalloc(a, lineno, fname);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    return retval;
}

void MPIU_trfree(void *a_ptr, int line, const char fname[])
{
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    MPL_trfree(a_ptr, line, fname);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
}

int MPIU_trvalid(const char str[])
{
    int retval;
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    retval = MPL_trvalid(str);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    return retval;
}

void MPIU_trspace(size_t *space, size_t *fr)
{
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    MPL_trspace(space, fr);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
}

void MPIU_trid(int id)
{
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    MPL_trid(id);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
}

void MPIU_trlevel(int level)
{
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    MPL_trlevel(level);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
}

void MPIU_trDebugLevel(int level)
{
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    MPL_trDebugLevel(level);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
}

void *MPIU_trcalloc(size_t nelem, size_t elsize, int lineno, const char fname[])
{
    void *retval;
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    retval = MPL_trcalloc(nelem, elsize, lineno, fname);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    return retval;
}

void *MPIU_trrealloc(void *p, size_t size, int lineno, const char fname[])
{
    void *retval;
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    retval = MPL_trrealloc(p, size, lineno, fname);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    return retval;
}

void *MPIU_trstrdup(const char *str, int lineno, const char fname[])
{
    void *retval;
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    retval = MPL_trstrdup(str, lineno, fname);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    return retval;
}

void MPIU_TrSetMaxMem(size_t size)
{
    MPID_THREAD_CS_ENTER(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
    MPL_TrSetMaxMem(size);
    MPID_THREAD_CS_EXIT(ALLGRAN, MPIR_THREAD_ALLGRAN_MEMALLOC_MUTEX);
}

