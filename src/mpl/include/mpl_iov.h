/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_IOV_H_INCLUDED
#define MPL_IOV_H_INCLUDED

#include "mplconfig.h"

#include <stdio.h>

#include <sys/types.h>  /* macs need sys/types.h before uio.h can be included */
#ifdef MPL_HAVE_SYS_UIO_H
#include <sys/uio.h>
#else
/* e.g. native Windows; same layout as POSIX */
struct iovec {
    void *iov_base;
    size_t iov_len;
};
#endif

/* FIXME: How is IOV_LIMIT chosen? */
#define MPL_IOV_LIMIT   16

#endif /* MPL_IOV_H_INCLUDED */
