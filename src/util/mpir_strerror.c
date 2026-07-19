/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
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
