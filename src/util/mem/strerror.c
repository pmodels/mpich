/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This would live in safestr.c, but it requires thread-private storage support
 * from mpiimpl.h and friends.  safestr.c is meant to be able to be used in
 * different software packages, perhaps someday by moving it to MPL. */
#include "mpiimpl.h"

#if defined(HAVE_STRERROR_R) && defined(NEEDS_STRERROR_R_DECL)
#if defined(STRERROR_R_CHAR_P)
char *strerror_r(int errnum, char *strerrbuf, size_t buflen);
#else
int strerror_r(int errnum, char *strerrbuf, size_t buflen);
#endif
#endif

/* ideally, provides a thread-safe version of strerror */
const char *MPIU_Strerror(int errnum)
{
#if defined(HAVE_STRERROR_R)
    char *buf;
    MPID_THREADPRIV_DECL;
    MPID_THREADPRIV_GET;
    buf = MPID_THREADPRIV_FIELD(strerrbuf);
#  if defined(STRERROR_R_CHAR_P)
    /* strerror_r returns char ptr (old GNU-flavor).  Static strings for known
     * errnums are in returned buf, unknown errnums put a message in buf and
     * return buf */
    buf = strerror_r(errnum, buf, MPIU_STRERROR_BUF_SIZE);
#  else
    /* strerror_r returns an int */
    strerror_r(errnum, buf, MPIU_STRERROR_BUF_SIZE);
#  endif
    return buf;

#elif defined(HAVE_STRERROR)
    /* MT - not guaranteed to be thread-safe, but on may platforms it will be
     * anyway for the most common cases (looking up an error string in a table
     * of constants).
     *
     * Using a mutex here would be an option, but then you need a version
     * without the mutex to call when interpreting errors from mutex functions
     * themselves. */
    return strerror(errnum);

#else
    /* nowadays this case is most likely to happen because of a configure or
     * internal header file inclusion bug rather than an actually missing
     * strerror routine */
    return "(strerror() unavailable on this platform)"

#endif
}

