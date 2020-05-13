/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_STRERROR_H_INCLUDED
#define MPIR_STRERROR_H_INCLUDED

#define MPIR_STRERROR_BUF_SIZE (1024)

const char *MPIR_Strerror(int errnum, char *buf, size_t buflen);

#endif /* MPIR_STRERROR_H_INCLUDED */
