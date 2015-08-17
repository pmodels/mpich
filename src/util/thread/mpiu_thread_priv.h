/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIU_THREAD_PRIV_H_INCLUDED)
#define MPIU_THREAD_PRIV_H_INCLUDED

/* The following three macros define a way to portably access thread-private
   storage in MPICH, and avoid extra overhead when MPICH is single
   threaded
   INITKEY - Create the key.  Must happen *before* the other threads
             are created
   INIT    - Create the thread-private storage.  Must happen once per thread
   DECL    - Declare local variables
   GET     - Access the thread-private storage
   FIELD   - Access the thread-private field (by name)
   FINALIZE - to be invoked when all threads no longer need access to the thread
              local storage, such as at MPI_Finalize time

   The "DECL" is the extern so that there is always a statement for
   the declaration.
*/

#if !defined(MPIU_TLS_SPECIFIER)
/* We need to provide a function that will cleanup the storage attached
 * to the key.  */
void MPIUI_Cleanup_tls(void *a);

/* In the case where the thread level is set in MPI_Init_thread, we
   need a blended version of the non-threaded and the thread-multiple
   definitions.

   The approach is to have TWO MPICH_PerThread_t pointers.  One is local
   (The MPIU_THREADPRIV_DECL is used in the routines local definitions),
   as in the threaded version of these macros.  This is set by using a routine
   to get thread-private storage.  The second is a preallocated, extern
   MPICH_PerThread_t struct, as in the single threaded case.  Based on
   MPIR_Process.isThreaded, one or the other is used.

 */
/* For the single threaded case, we use a preallocated structure
   This structure is allocated in src/mpi/init/initthread.c */
extern MPICH_PerThread_t MPIU_ThreadSingle;

#define MPIU_THREADPRIV_INITKEY                                         \
    do {                                                                \
        if (MPIR_ThreadInfo.isThreaded) {                               \
            int initkey_err_;                                           \
            MPIU_Thread_tls_create(MPIUI_Cleanup_tls,&MPIR_ThreadInfo.thread_storage,&initkey_err_); \
            MPIU_Assert(initkey_err_ == 0);                             \
        }                                                               \
    } while (0)

#define MPIU_THREADPRIV_INIT                                            \
    do {                                                                \
        if (MPIR_ThreadInfo.isThreaded) {                               \
            int init_err_;                                              \
            MPIU_Thread_ptr = (MPICH_PerThread_t *) MPIU_Calloc(1, sizeof(MPICH_PerThread_t)); \
            MPIU_Assert(MPIU_Thread_ptr);                               \
            MPIU_Thread_tls_set(&MPIR_ThreadInfo.thread_storage, (void *)MPIU_Thread_ptr, &init_err_); \
            MPIU_Assert(init_err_ == 0);                                \
        }                                                               \
    } while (0)

#define MPIU_THREADPRIV_GET                                             \
    do {                                                                \
        if (!MPIU_Thread_ptr) {                                         \
            if (MPIR_ThreadInfo.isThreaded) {                           \
                int get_err_;                                           \
                MPIU_Thread_tls_get(&MPIR_ThreadInfo.thread_storage, (void **) &MPIU_Thread_ptr, &get_err_); \
                MPIU_Assert(get_err_ == 0);                             \
                if (!MPIU_Thread_ptr) {                                 \
                    MPIU_THREADPRIV_INIT; /* subtle, sets MPIU_Thread_ptr */ \
                }                                                       \
            }                                                           \
            else {                                                      \
                MPIU_Thread_ptr = &MPIU_ThreadSingle;                   \
            }                                                           \
            MPIU_Assert(MPIU_Thread_ptr);                               \
        }                                                               \
    } while (0)

/* common definitions when using MPIU_Thread-based TLS */
#define MPIU_THREADPRIV_DECL MPICH_PerThread_t *MPIU_Thread_ptr = NULL
#define MPIU_THREADPRIV_FIELD(a_) (MPIU_Thread_ptr->a_)
#define MPIU_THREADPRIV_FINALIZE                                        \
    do {                                                                \
        MPIU_THREADPRIV_DECL;                                           \
        if (MPIR_ThreadInfo.isThreaded) {                               \
            int tpf_err_; /* unique name to not conflict with vars in called macros */ \
            MPIU_THREADPRIV_GET;                                        \
            MPIU_Free(MPIU_Thread_ptr);                                 \
            MPIU_Thread_tls_set(&MPIR_ThreadInfo.thread_storage,NULL, &tpf_err_); \
            MPIU_Assert(tpf_err_ == 0);                                 \
            MPIU_Thread_tls_destroy(&MPIR_ThreadInfo.thread_storage,&tpf_err_); \
            MPIU_Assert(tpf_err_ == 0);                                 \
        }                                                               \
        } while (0)

#else /* defined(MPIU_TLS_SPECIFIER) */

/* We have proper thread-local storage (TLS) support from the compiler, which
 * should yield the best performance and simplest code, so we'll use that. */
extern MPIU_TLS_SPECIFIER MPICH_PerThread_t MPIU_Thread;

#define MPIU_THREADPRIV_INITKEY
#define MPIU_THREADPRIV_INIT
#define MPIU_THREADPRIV_DECL
#define MPIU_THREADPRIV_GET
#define MPIU_THREADPRIV_FIELD(a_) (MPIU_Thread.a_)
#define MPIU_THREADPRIV_FINALIZE do {} while (0)

#endif /* defined(MPIU_TLS_SPECIFIER) */

#endif /* !defined(MPIU_THREAD_PRIV_H_INCLUDED) */
