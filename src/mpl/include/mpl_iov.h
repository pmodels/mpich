/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPL_IOV_H_INCLUDED
#define MPL_IOV_H_INCLUDED

#include <stdio.h>

#include <sys/types.h>  /* macs need sys/types.h before uio.h can be included */
#include <sys/uio.h>

/* FIXME: How is IOV_LIMIT chosen? */
#define MPL_IOV_LIMIT   16

#endif /* MPL_IOV_H_INCLUDED */
