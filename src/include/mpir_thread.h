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
#include "utlist.h"

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

/* This structure contains all thread-local variables and will be zeroed at
 * allocation time.
 *
 * Note that any pointers to dynamically allocated memory stored in this
 * structure must be externally cleaned up.
 * */
typedef struct {
    int dummy;
} MPIR_Per_thread_t;

#if defined(MPICH_IS_THREADED) && defined(MPL_TLS)
extern MPL_TLS MPIR_Per_thread_t MPIR_Per_thread;
#else
extern MPIR_Per_thread_t MPIR_Per_thread;
#endif

extern MPID_Thread_tls_t MPIR_Per_thread_key;

/* During Init time, `isThreaded` is not set until the very end of init -- preventing
 * usage of mutexes during init-time; `thread_provided` is set by MPID_Init_thread_level
 * early in the stage so it can be used instead.
 */
#if defined(MPICH_IS_THREADED)
#define MPIR_THREAD_CHECK_BEGIN if (MPIR_ThreadInfo.thread_provided == MPI_THREAD_MULTIPLE) {
#define MPIR_THREAD_CHECK_END   }
#else
#define MPIR_THREAD_CHECK_BEGIN
#define MPIR_THREAD_CHECK_END
#endif /* MPICH_IS_THREADED */

void MPIR_Add_mutex(MPID_Thread_mutex_t * p_mutex);
void MPIR_Thread_CS_Init(void);
void MPIR_Thread_CS_Finalize(void);

/* ------------------------------------------------------------ */
/* Global thread model, used for non-performance-critical paths */
/* CONSIDER:
 * - should we restrict to MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX only?
 * - once we isolate the mutexes, we should replace MPID with MPL
 */

#if defined(MPICH_IS_THREADED)
MPIR_EXTERN MPID_Thread_mutex_t MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX;

/* DIRECT macros are only to be used in MPI_Init/MPI_Finalize, and
 * MPIR_ThreadInfo.isThreaded should be set to 0 to disable other CS
 */
#define MPIR_THREAD_CS_ENTER_DIRECT(mutex) \
    do { \
        int err_ = 0; \
        MPID_Thread_mutex_lock(&mutex, &err_); \
        MPIR_Assert(err_ == 0); \
    } while (0)

#define MPIR_THREAD_CS_EXIT_DIRECT(mutex) \
    do { \
        int err_ = 0; \
        MPID_Thread_mutex_unlock(&mutex, &err_); \
        MPIR_Assert(err_ == 0); \
    } while (0)

/* CS macros with runtime bypass
 */
#define MPIR_THREAD_CS_ENTER(mutex) \
    if (MPIR_ThreadInfo.isThreaded) { \
        MPIR_THREAD_CS_ENTER_DIRECT(mutex); \
    }

#define MPIR_THREAD_CS_EXIT(mutex) \
    if (MPIR_ThreadInfo.isThreaded) { \
        MPIR_THREAD_CS_EXIT_DIRECT(mutex); \
    }

#else
#define MPIR_THREAD_CS_ENTER_DIRECT(mutex)
#define MPIR_THREAD_CS_EXIT_DIRECT(mutex)
#define MPIR_THREAD_CS_ENTER(mutex)
#define MPIR_THREAD_CS_EXIT(mutex)
#endif

/* ------------------------------------------------------------ */
/* Other thread models, for performance-critical paths          */

#if defined(MPICH_IS_THREADED)

#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ
MPIR_EXTERN MPID_Thread_mutex_t MPIR_THREAD_POBJ_HANDLE_MUTEX;
MPIR_EXTERN MPID_Thread_mutex_t MPIR_THREAD_POBJ_MSGQ_MUTEX;
MPIR_EXTERN MPID_Thread_mutex_t MPIR_THREAD_POBJ_COMPLETION_MUTEX;
MPIR_EXTERN MPID_Thread_mutex_t MPIR_THREAD_POBJ_CTX_MUTEX;
MPIR_EXTERN MPID_Thread_mutex_t MPIR_THREAD_POBJ_PMI_MUTEX;

#define MPIR_THREAD_POBJ_COMM_MUTEX(_comm_ptr) _comm_ptr->mutex
#define MPIR_THREAD_POBJ_WIN_MUTEX(_win_ptr)   _win_ptr->mutex

#elif MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
MPIR_EXTERN MPID_Thread_mutex_t MPIR_THREAD_VCI_GLOBAL_MUTEX;
MPIR_EXTERN MPID_Thread_mutex_t MPIR_THREAD_VCI_HANDLE_MUTEX;

#endif /* MPICH_THREAD_GRANULARITY */

#endif /* MPICH_IS_THREADED */

#endif /* MPIR_THREAD_H_INCLUDED */
