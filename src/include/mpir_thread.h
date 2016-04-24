/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_THREAD_H_INCLUDED
#define MPIR_THREAD_H_INCLUDED

#include "mpichconfconst.h"
#include "mpichconf.h"

typedef struct {
    int thread_provided;        /* Provided level of thread support */

    /* This is a special case for is_thread_main, which must be
     * implemented even if MPICH itself is single threaded.  */
#if MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED
    MPID_Thread_id_t master_thread;     /* Thread that started MPI */
#endif

#if defined MPICH_IS_THREADED
    int isThreaded;             /* Set to true if user requested
                                 * THREAD_MULTIPLE */
#endif                          /* MPICH_IS_THREADED */
} MPIR_Thread_info_t;
extern MPIR_Thread_info_t MPIR_ThreadInfo;

/* ------------------------------------------------------------------------- */
/* thread-local storage macros */
/* arbitrary, just needed to avoid cleaning up heap allocated memory at thread
 * destruction time */
#define MPIR_STRERROR_BUF_SIZE (1024)

/* This structure contains all thread-local variables and will be zeroed at
 * allocation time.
 *
 * Note that any pointers to dynamically allocated memory stored in this
 * structure must be externally cleaned up.
 * */
typedef struct {
    int op_errno;               /* For errors in predefined MPI_Ops */

    /* error string storage for MPIR_Strerror */
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];

#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    int lock_depth;
#endif
} MPIR_Per_thread_t;

#if defined(MPICH_IS_THREADED) && defined(MPL_TLS_SPECIFIER)
extern MPL_TLS_SPECIFIER MPIR_Per_thread_t MPIR_Per_thread;
#else
extern MPIR_Per_thread_t MPIR_Per_thread;
#endif

extern MPID_Thread_tls_t MPIR_Per_thread_key;

#if defined(MPICH_IS_THREADED)
#define MPIR_THREAD_CHECK_BEGIN if (MPIR_ThreadInfo.isThreaded) {
#define MPIR_THREAD_CHECK_END   }
#else
#define MPIR_THREAD_CHECK_BEGIN
#define MPIR_THREAD_CHECK_END
#endif /* MPICH_IS_THREADED */

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL || \
    MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
extern MPID_Thread_mutex_t MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX;
#endif

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
extern MPID_Thread_mutex_t MPIR_THREAD_POBJ_HANDLE_MUTEX;
extern MPID_Thread_mutex_t MPIR_THREAD_POBJ_MSGQ_MUTEX;
extern MPID_Thread_mutex_t MPIR_THREAD_POBJ_COMPLETION_MUTEX;
extern MPID_Thread_mutex_t MPIR_THREAD_POBJ_CTX_MUTEX;
extern MPID_Thread_mutex_t MPIR_THREAD_POBJ_PMI_MUTEX;

#define MPIR_THREAD_POBJ_COMM_MUTEX(_comm_ptr) _comm_ptr->mutex
#define MPIR_THREAD_POBJ_WIN_MUTEX(_win_ptr)   _win_ptr->mutex
#endif

#endif /* MPIR_THREAD_H_INCLUDED */
