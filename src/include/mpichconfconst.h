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

#define MPICH_ERROR_MSG_NONE 0
#define MPICH_ERROR_MSG_CLASS 1
#define MPICH_ERROR_MSG_GENERIC 2
#define MPICH_ERROR_MSG_ALL 8


/* -------------------------------------------------------------------- */
/* thread-related constants */
/* -------------------------------------------------------------------- */

/* Define the four ways that we achieve proper thread-safe updates of
 * shared structures and services
 *
 * A configure choice will set MPICH_THREAD_GRANULARITY to one of these values */

/* _INVALID exists to avoid accidental macro evaluations to 0 */
#define MPICH_THREAD_GRANULARITY_INVALID 0
#define MPICH_THREAD_GRANULARITY_GLOBAL 1
#define MPICH_THREAD_GRANULARITY_PER_OBJECT 2
#define MPICH_THREAD_GRANULARITY_LOCK_FREE 3
/* _SINGLE is the "null" granularity, where all processes are single-threaded */
#define MPICH_THREAD_GRANULARITY_SINGLE 4

/* controls the allocation mechanism for MPID_Request handles, which can greatly
 * affect concurrency on the critical path */
#define MPIU_HANDLE_ALLOCATION_MUTEX         0
#define MPIU_HANDLE_ALLOCATION_THREAD_LOCAL  1

/* _INVALID exists to avoid accidental macro evaluations to 0 */
#define MPIU_REFCOUNT_INVALID 0
/* _NONE means no concurrency control, such as when using MPI_THREAD_SINGLE */
#define MPIU_REFCOUNT_NONE 1
#define MPIU_REFCOUNT_LOCKFREE 2

#endif /* MPICHCONFCONST_H_INCLUDED */
