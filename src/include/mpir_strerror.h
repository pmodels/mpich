/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPIR_STRERROR_H_INCLUDED
#define MPIR_STRERROR_H_INCLUDED

#define MPIR_STRERROR_BUF_SIZE (1024)

const char *MPIR_Strerror(int errnum, char *buf, size_t buflen);

#endif /* MPIR_STRERROR_H_INCLUDED */
