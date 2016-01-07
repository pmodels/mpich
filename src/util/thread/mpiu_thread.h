/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIU_THREAD_H_INCLUDED)
#define MPIU_THREAD_H_INCLUDED

#include "mpi.h"
#include "mpichconf.h"  /* defines MPICH_THREAD_PACKAGE_NAME */
#include "mpichconfconst.h"
#include "mpidbg.h"
#include "mpiu_strerror.h"

/* FIXME: Adding mpimem.h here as a workaround for using MPIU_Calloc
 * functionality. */
#include "mpimem.h"

/* _INVALID exists to avoid accidental macro evaluations to 0 */
#define MPICH_THREAD_PACKAGE_INVALID 0
#define MPICH_THREAD_PACKAGE_NONE    1
#define MPICH_THREAD_PACKAGE_POSIX   2
#define MPICH_THREAD_PACKAGE_SOLARIS 3
#define MPICH_THREAD_PACKAGE_WIN     4

#if defined(MPICH_THREAD_PACKAGE_NAME) && (MPICH_THREAD_PACKAGE_NAME == MPICH_THREAD_PACKAGE_POSIX)
#  include "mpiu_thread_posix.h"
#elif defined(MPICH_THREAD_PACKAGE_NAME) && (MPICH_THREAD_PACKAGE_NAME == MPICH_THREAD_PACKAGE_SOLARIS)
#  include "mpiu_thread_solaris.h"
#elif defined(MPICH_THREAD_PACKAGE_NAME) && (MPICH_THREAD_PACKAGE_NAME == MPICH_THREAD_PACKAGE_WIN)
#  include "mpiu_thread_win.h"
#elif defined(MPICH_THREAD_PACKAGE_NAME) && (MPICH_THREAD_PACKAGE_NAME == MPICH_THREAD_PACKAGE_NONE)
typedef int MPIU_Thread_mutex_t;
typedef int MPIU_Thread_cond_t;
typedef int MPIU_Thread_id_t;
typedef int MPIU_Thread_tls_t;
typedef void (*MPIU_Thread_func_t) (void *data);
#define MPIU_Thread_mutex_create(mutex_ptr_, err_ptr_)  { *((int*)err_ptr_) = 0;}
#define MPIU_Thread_mutex_destroy(mutex_ptr_, err_ptr_) { *((int*)err_ptr_) = 0;}
#else
#  error "thread package not defined or unknown"
#endif

/* Error values */
#define MPIU_THREAD_SUCCESS 0
#define MPIU_THREAD_ERROR   1
/* FIXME: Define other error codes.  For now, any non-zero value is an error. */

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_INVALID
#  error Invalid thread granularity option specified (possibly none)
#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_LOCK_FREE
#  error MPICH_THREAD_GRANULARITY_LOCK_FREE not implemented yet
#endif

/* ------------------------------------------------------------------------- */
/* thread-local storage macros */
/* moved here from mpiimpl.h because they logically belong here */

/* arbitrary, just needed to avoid cleaning up heap allocated memory at thread
 * destruction time */
#define MPIU_STRERROR_BUF_SIZE (1024)

/* This structure contains all thread-local variables and will be zeroed at
 * allocation time.
 *
 * Note that any pointers to dynamically allocated memory stored in this
 * structure must be externally cleaned up.
 * */
typedef struct {
    int op_errno;               /* For errors in predefined MPI_Ops */

    /* error string storage for MPIU_Strerror */
    char strerrbuf[MPIU_STRERROR_BUF_SIZE];

#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    int lock_depth;
#endif
} MPIUI_Per_thread_t;

#include "mpiu_thread_priv.h"

#endif /* !defined(MPIU_THREAD_H_INCLUDED) */
