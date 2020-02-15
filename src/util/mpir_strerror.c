/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if defined(NEEDS_STRERROR_R_DECL)
int strerror_r(int errnum, char *strerrbuf, size_t buflen);
#endif

/* thread-safe version of strerror, similar to C11 strerror_s */
const char *MPIR_Strerror(int errnum)
{
    char *buf;
    MPIR_Per_thread_t *per_thread = NULL;
    int err = 0;

    MPID_THREADPRIV_KEY_GET_ADDR(MPIR_Per_thread_key, MPIR_Per_thread, per_thread, &err);
    MPIR_Assert(err == 0);
    buf = per_thread->strerrbuf;
    strerror_r(errnum, buf, MPIR_STRERROR_BUF_SIZE);

    return buf;
}
