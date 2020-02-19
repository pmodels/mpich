/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if defined(NEEDS_STRERROR_R_DECL)
#if defined(STRERROR_R_CHAR_P)
char *strerror_r(int errnum, char *buf, size_t buflen);
#else
int strerror_r(int errnum, char *buf, size_t buflen);
#endif
#endif

/* wrapper for strerror_r to return the buffer used */
const char *MPIR_Strerror(int errnum, char *buf, size_t buflen)
{
#if defined(STRERROR_R_CHAR_P)
    return strerror_r(errnum, buf, buflen);
#else
    strerror_r(errnum, buf, buflen);
    return buf;
#endif
}
