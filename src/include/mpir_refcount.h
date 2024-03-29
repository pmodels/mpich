/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_REFCOUNT_H_INCLUDED
#define MPIR_REFCOUNT_H_INCLUDED

#include "mpi.h"
#include "mpichconf.h"

#if MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
#include "mpir_refcount_global.h"
/* For the VCI granularity, is this overkill? */
#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
#include "mpir_refcount_vci.h"
#endif

#else
#include "mpir_refcount_single.h"

#endif

#endif /* MPIR_REFCOUNT_H_INCLUDED */
