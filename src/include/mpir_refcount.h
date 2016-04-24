/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIR_REFCOUNT_H_INCLUDED)
#define MPIR_REFCOUNT_H_INCLUDED

#include "mpi.h"
#include "mpichconf.h"

#if MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
#include "mpir_refcount_global.h"
#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
#include "mpir_refcount_pobj.h"
#endif

#else
#include "mpir_refcount_single.h"

#endif

#endif /* !defined(MPIR_REFCOUNT_H_INCLUDED) */
