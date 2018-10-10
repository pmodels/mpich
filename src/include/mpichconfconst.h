/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This header file contains constants that might end up on the right hand side
 * of a #define in mpichconf.h.
 *
 * In particular, this file must provide any constants that might be used for
 * conditional compilation in mpidpre.h, before most of the other headers have
 * been included in mpiimpl.h. */
#ifndef MPICHCONFCONST_H_INCLUDED
#define MPICHCONFCONST_H_INCLUDED

#define MPICH_ERROR_MSG__NONE 0
#define MPICH_ERROR_MSG__CLASS 1
#define MPICH_ERROR_MSG__GENERIC 2
#define MPICH_ERROR_MSG__ALL 8

/* -------------------------------------------------------------------- */
/* thread-related constants */
/* -------------------------------------------------------------------- */

/* Define the four ways that we achieve proper thread-safe updates of
 * shared structures and services
 *
 * A configure choice will set MPICH_THREAD_GRANULARITY to one of these values */

/* _INVALID exists to avoid accidental macro evaluations to 0 */
#define MPICH_THREAD_GRANULARITY__INVALID 0
#define MPICH_THREAD_GRANULARITY__GLOBAL 1
#define MPICH_THREAD_GRANULARITY__POBJ 2
#define MPICH_THREAD_GRANULARITY__LOCKFREE 3
/* _SINGLE is the "null" granularity, where all processes are single-threaded */
#define MPICH_THREAD_GRANULARITY__SINGLE 4
#define MPICH_THREAD_GRANULARITY__VNI 5

/* _NONE means no concurrency control, such as when using MPI_THREAD_SINGLE */
#define MPICH_REFCOUNT__NONE 1
#define MPICH_REFCOUNT__LOCKFREE 2

/* Possible values for timing */
#define MPICH_TIMING_KIND__NONE 0
#define MPICH_TIMING_KIND__TIME 1
#define MPICH_TIMING_KIND__LOG 2
#define MPICH_TIMING_KIND__LOG_DETAILED 3
#define MPICH_TIMING_KIND__ALL 4
#define MPICH_TIMING_KIND__RUNTIME 5

/* Possible values for USE_LOGGING */
#define MPICH_LOGGING__NONE 0
#define MPICH_LOGGING__RLOG 1
#define MPICH_LOGGING__EXTERNAL 4

/* Possible values for process state */
#define MPICH_MPI_STATE__PRE_INIT 0
#define MPICH_MPI_STATE__IN_INIT 1
#define MPICH_MPI_STATE__POST_INIT 2
#define MPICH_MPI_STATE__POST_FINALIZED 3

#endif /* MPICHCONFCONST_H_INCLUDED */
