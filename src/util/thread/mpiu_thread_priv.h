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

#if !defined(MPICH_TLS_SPECIFIER)
/* We need to provide a function that will cleanup the storage attached
 * to the key.  */
void MPIUI_Cleanup_tls(void *a);

/* In the case where the thread level is set in MPI_Init_thread, we
   need a blended version of the non-threaded and the thread-multiple
   definitions.

   The approach is to have TWO MPIUI_Per_thread_t pointers.  One is local
   (The MPIU_THREADPRIV_DECL is used in the routines local definitions),
   as in the threaded version of these macros.  This is set by using a routine
   to get thread-private storage.  The second is a preallocated, extern
   MPIUI_Per_thread_t struct, as in the single threaded case.  Based on
   whether MPICH is initialized with thread safety, one or the other is used.

 */
/* For the single threaded case, we use a preallocated structure
   This structure is allocated in src/mpi/init/initthread.c */
extern MPIUI_Per_thread_t MPIUI_ThreadSingle;

#define MPIU_THREADPRIV_INITKEY(is_threaded, initkey_err_ptr_)          \
    do {                                                                \
        if (is_threaded) {                                              \
            MPIU_Thread_tls_create(MPIUI_Cleanup_tls,&MPIR_ThreadInfo.thread_storage,initkey_err_ptr_); \
        }                                                               \
    } while (0)

#define MPIU_THREADPRIV_INIT(is_threaded, init_err_ptr_)                \
    do {                                                                \
        if (is_threaded) {                                              \
            MPIUI_Thread_ptr = (MPIUI_Per_thread_t *) MPIU_Calloc(1, sizeof(MPIUI_Per_thread_t)); \
            if (unlikely(MPIUI_Thread_ptr == NULL)) {                   \
                *((int *) init_err_ptr_) = MPIU_THREAD_ERROR;           \
                break;                                                  \
            }                                                           \
            MPIU_Thread_tls_set(&MPIR_ThreadInfo.thread_storage, (void *)MPIUI_Thread_ptr, init_err_ptr_); \
        }                                                               \
    } while (0)

#define MPIU_THREADPRIV_GET(is_threaded, get_err_ptr_)                  \
    do {                                                                \
        if (!MPIUI_Thread_ptr) {                                        \
            if (is_threaded) {                                          \
                MPIU_Thread_tls_get(&MPIR_ThreadInfo.thread_storage, (void **) &MPIUI_Thread_ptr, get_err_ptr_); \
                if (unlikely(*((int *) get_err_ptr_)))                  \
                    break;                                              \
                if (!MPIUI_Thread_ptr) {                                \
                    MPIU_THREADPRIV_INIT(is_threaded, get_err_ptr_); /* subtle, sets MPIUI_Thread_ptr */ \
                }                                                       \
            }                                                           \
            else {                                                      \
                MPIUI_Thread_ptr = &MPIUI_ThreadSingle;                 \
            }                                                           \
                                                                        \
            if (unlikely(MPIUI_Thread_ptr == NULL)) {                   \
                *((int *) get_err_ptr_) = MPIU_THREAD_ERROR;            \
                break;                                                  \
            }                                                           \
        }                                                               \
    } while (0)

/* common definitions when using MPIU_Thread-based TLS */
#define MPIU_THREADPRIV_DECL MPIUI_Per_thread_t *MPIUI_Thread_ptr = NULL
#define MPIU_THREADPRIV_FIELD(a_) (MPIUI_Thread_ptr->a_)
#define MPIU_THREADPRIV_FINALIZE(is_threaded, finalize_err_ptr_)        \
    do {                                                                \
        MPIU_THREADPRIV_DECL;                                           \
        if (is_threaded) {                                              \
            MPIU_THREADPRIV_GET(is_threaded, finalize_err_ptr_);        \
            MPIU_Free(MPIUI_Thread_ptr);                                \
            MPIU_Thread_tls_set(&MPIR_ThreadInfo.thread_storage,NULL,finalize_err_ptr_); \
            if (unlikely(*((int *) finalize_err_ptr_)))                 \
                break;                                                  \
            MPIU_Thread_tls_destroy(&MPIR_ThreadInfo.thread_storage,finalize_err_ptr_); \
            if (unlikely(*((int *) finalize_err_ptr_)))                 \
                break;                                                  \
        }                                                               \
    } while (0)

#else /* defined(MPICH_TLS_SPECIFIER) */

/* We have proper thread-local storage (TLS) support from the compiler, which
 * should yield the best performance and simplest code, so we'll use that. */
extern MPICH_TLS_SPECIFIER MPIUI_Per_thread_t MPIUI_Thread;

#define MPIU_THREADPRIV_INITKEY(...)
#define MPIU_THREADPRIV_INIT(...)
#define MPIU_THREADPRIV_DECL
#define MPIU_THREADPRIV_GET(...)
#define MPIU_THREADPRIV_FIELD(a_) (MPIUI_Thread.a_)
#define MPIU_THREADPRIV_FINALIZE(...) do {} while (0)

#endif /* defined(MPICH_TLS_SPECIFIER) */

#endif /* !defined(MPIU_THREAD_PRIV_H_INCLUDED) */
