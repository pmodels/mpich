/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIU_THREAD_H_INCLUDED)
#define MPIU_THREAD_H_INCLUDED

#include "mpi.h"
#include "mpichconf.h"  /* defines MPIU_THREAD_PACKAGE_NAME */
#include "mpichconfconst.h"
#include "mpidbg.h"
#include "mpiassert.h"
/* Time stamps */
/* Get the timer definitions.  The source file for this include is
   src/util/timers/mpiu_timer.h.in */
#include "mpiu_strerror.h"
#include "mpiu_timer.h"

/* FIXME: we should not be including an MPIR-level header here.  But
 * the code is currently a rat-hole where the MPIU and MPIR functions
 * are all mixed up.  Till that's resolved, adding mpimem.h here as a
 * workaround for using MPIU_Calloc functionality. */
#include "mpimem.h"

/* _INVALID exists to avoid accidental macro evaluations to 0 */
#define MPIU_THREAD_PACKAGE_INVALID 0
#define MPIU_THREAD_PACKAGE_NONE    1
#define MPIU_THREAD_PACKAGE_POSIX   2
#define MPIU_THREAD_PACKAGE_SOLARIS 3
#define MPIU_THREAD_PACKAGE_WIN     4

#if defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_POSIX)
#  include "mpiu_thread_posix_types.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_SOLARIS)
#  include "mpiu_thread_solaris_types.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_WIN)
#  include "mpiu_thread_win_types.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_NONE)
typedef int MPIU_Thread_mutex_t;
typedef int MPIU_Thread_cond_t;
typedef int MPIU_Thread_id_t;
typedef int MPIU_Thread_tls_t;
#else
#  error "thread package not defined or unknown"
#endif

/*
 * threading function prototypes
 *
 * Typically some or all of these are actually implemented as macros
 * rather than actual or even inline functions.
 */

typedef void (*MPIU_Thread_func_t) (void *data);

void MPIU_Thread_create(MPIU_Thread_func_t func, void *data, MPIU_Thread_id_t * id, int *err);
void MPIU_Thread_exit(void);
void MPIU_Thread_self(MPIU_Thread_id_t * id);
void MPIU_Thread_same(MPIU_Thread_id_t * id1, MPIU_Thread_id_t * id2, int *same);
void MPIU_Thread_yield(void);

void MPIU_Thread_mutex_create(MPIU_Thread_mutex_t * mutex, int *err);
void MPIU_Thread_mutex_destroy(MPIU_Thread_mutex_t * mutex, int *err);
void MPIU_Thread_mutex_lock(MPIU_Thread_mutex_t * mutex, int *err);
void MPIU_Thread_mutex_unlock(MPIU_Thread_mutex_t * mutex, int *err);

void MPIU_Thread_cond_create(MPIU_Thread_cond_t * cond, int *err);
void MPIU_Thread_cond_destroy(MPIU_Thread_cond_t * cond, int *err);
void MPIU_Thread_cond_wait(MPIU_Thread_cond_t * cond, MPIU_Thread_mutex_t * mutex, int *err);
void MPIU_Thread_cond_broadcast(MPIU_Thread_cond_t * cond, int *err);
void MPIU_Thread_cond_signal(MPIU_Thread_cond_t * cond, int *err);


/* Error values */
#define MPIU_THREAD_SUCCESS 0
/* FIXME: Define other error codes.  For now, any non-zero value is an error. */

/* Implementation specific function definitions (usually in the form of macros) */
#if defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_POSIX)
#  include "mpiu_thread_posix_funcs.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_SOLARIS)
#  include "mpiu_thread_solaris_funcs.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_WIN)
#  include "mpiu_thread_win_funcs.h"
#elif defined(MPIU_THREAD_PACKAGE_NAME) && (MPIU_THREAD_PACKAGE_NAME == MPIU_THREAD_PACKAGE_NONE)
/* do nothing */
#else
#  error "thread package not defined or unknown"
#endif


#if MPICH_THREAD_GRANULARITY == MPIR_THREAD_GRANULARITY_INVALID
#  error Invalid thread granularity option specified (possibly none)
#elif MPICH_THREAD_GRANULARITY == MPIR_THREAD_GRANULARITY_LOCK_FREE
#  error MPIR_THREAD_GRANULARITY_LOCK_FREE not implemented yet
#endif


/* I don't have a better place for this at the moment, so it lives here for now. */
enum MPIU_Thread_cs_name {
    MPIU_THREAD_CS_NAME_GLOBAL = 0,
    MPIU_THREAD_CS_NAME_POBJ,
    MPIU_THREAD_CS_NUM_NAMES
};

typedef struct MPICH_ThreadInfo_t {
    int thread_provided;        /* Provided level of thread support */
    /* This is a special case for is_thread_main, which must be
     * implemented even if MPICH itself is single threaded.  */
#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)
#  if !defined(MPIU_TLS_SPECIFIER)
    MPIU_Thread_tls_t thread_storage;   /* Id for perthread data */
#  endif                        /* !TLS */
    MPIU_Thread_id_t master_thread;     /* Thread that started MPI */
#endif

#if defined MPICH_IS_THREADED
    int isThreaded;             /* Set to true if user requested
                                 * THREAD_MULTIPLE */
#endif                          /* MPICH_IS_THREADED */

    /* Define the mutex values used for each kind of implementation */
#if MPICH_THREAD_GRANULARITY == MPIR_THREAD_GRANULARITY_GLOBAL || \
    MPICH_THREAD_GRANULARITY == MPIR_THREAD_GRANULARITY_PER_OBJECT
    MPIU_Thread_mutex_t global_mutex;
    MPIU_Thread_mutex_t handle_mutex;
#endif

#if MPICH_THREAD_GRANULARITY == MPIR_THREAD_GRANULARITY_PER_OBJECT
    MPIU_Thread_mutex_t msgq_mutex;
    MPIU_Thread_mutex_t completion_mutex;
    MPIU_Thread_mutex_t ctx_mutex;
    MPIU_Thread_mutex_t pmi_mutex;
#endif

#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)
    MPIU_Thread_mutex_t memalloc_mutex; /* for MPIU_{Malloc,Free,Calloc} */
    MPIU_Thread_id_t cs_holder[MPIU_THREAD_CS_NUM_NAMES];
#endif
} MPICH_ThreadInfo_t;
extern MPICH_ThreadInfo_t MPIR_ThreadInfo;

#define MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX      MPIR_ThreadInfo.global_mutex
#define MPIR_THREAD_HANDLE_MUTEX      MPIR_ThreadInfo.handle_mutex
#define MPIR_THREAD_MSGQ_MUTEX        MPIR_ThreadInfo.msgq_mutex
#define MPIR_THREAD_COMPLETION_MUTEX  MPIR_ThreadInfo.completion_mutex
#define MPIR_THREAD_CTX_MUTEX         MPIR_ThreadInfo.ctx_mutex
#define MPIR_THREAD_PMI_MUTEX         MPIR_ThreadInfo.pmi_mutex
#define MPIR_THREAD_MEMALLOC_MUTEX    MPIR_ThreadInfo.memalloc_mutex
#define MPIR_THREAD_POBJ_COMM_MUTEX(_comm_ptr) _comm_ptr->mutex
#define MPIR_THREAD_POBJ_WIN_MUTEX(_win_ptr)   _win_ptr->mutex

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
typedef struct MPICH_PerThread_t {
    int op_errno;               /* For errors in predefined MPI_Ops */

    /* error string storage for MPIU_Strerror */
    char strerrbuf[MPIU_STRERROR_BUF_SIZE];
} MPICH_PerThread_t;

#if defined (MPICH_IS_THREADED)
#include "mpiu_thread_multiple.h"
#else
#include "mpiu_thread_single.h"
#endif

#endif /* !defined(MPIU_THREAD_H_INCLUDED) */
