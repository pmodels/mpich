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

#if defined(MPICH_IS_THREADED) && !defined(MPICH_TLS_SPECIFIER)

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

#define MPIU_THREADPRIV_KEY_CREATE(key, var, err_ptr_)                  \
    do {                                                                \
        void *thread_ptr;                                               \
                                                                        \
        MPIU_Thread_tls_create(MPIUI_Cleanup_tls, &(key) , err_ptr_);   \
        if (unlikely(*((int *) err_ptr_)))                              \
            break;                                                      \
        thread_ptr = MPIU_Calloc(1, sizeof(var));                       \
        if (unlikely(!thread_ptr)) {                                    \
            *((int *) err_ptr_) = MPIU_THREAD_ERROR;                    \
            break;                                                      \
        }                                                               \
        MPIU_Thread_tls_set(&(key), thread_ptr, err_ptr_);              \
    } while (0)

#define MPIU_THREADPRIV_KEY_GET_ADDR(is_threaded, key, var, addr, err_ptr_) \
    do {                                                                \
        if (is_threaded) {                                              \
            void *thread_ptr;                                           \
            MPIU_Thread_tls_get(&(key), &thread_ptr, err_ptr_);         \
            if (unlikely(*((int *) err_ptr_)))                          \
                break;                                                  \
            if (!thread_ptr) {                                          \
                thread_ptr = MPIU_Calloc(1, sizeof(var));               \
                if (unlikely(!thread_ptr)) {                            \
                    *((int *) err_ptr_) = MPIU_THREAD_ERROR;            \
                    break;                                              \
                }                                                       \
                MPIU_Thread_tls_set(&(key), thread_ptr, err_ptr_);      \
                if (unlikely(*((int *) err_ptr_)))                      \
                    break;                                              \
            }                                                           \
            addr = thread_ptr;                                          \
        }                                                               \
        else {                                                          \
            addr = &(var);                                              \
        }                                                               \
    } while (0)

#define MPIU_THREADPRIV_KEY_DESTROY(key, err_ptr_)              \
    do {                                                        \
        void *thread_ptr;                                       \
                                                                \
        MPIU_Thread_tls_get(&(key), &thread_ptr, err_ptr_);     \
        if (unlikely(*((int *) err_ptr_)))                      \
            break;                                              \
                                                                \
        if (thread_ptr)                                         \
            MPIU_Free(thread_ptr);                              \
                                                                \
        MPIU_Thread_tls_set(&(key), NULL, err_ptr_);            \
        if (unlikely(*((int *) err_ptr_)))                      \
            break;                                              \
                                                                \
        MPIU_Thread_tls_destroy(&(key), err_ptr_);              \
    } while (0)

#else /* !defined(MPICH_IS_THREADED) || defined(MPICH_TLS_SPECIFIER) */
/* We have proper thread-local storage (TLS) support from the compiler, which
 * should yield the best performance and simplest code, so we'll use that. */
#define MPIU_THREADPRIV_KEY_CREATE(...)
#define MPIU_THREADPRIV_KEY_GET_ADDR(is_threaded, key, var, addr, err_ptr_) \
    do {                                                                \
        addr = &(var);                                                  \
        *((int *) err_ptr_) = MPIU_THREAD_SUCCESS;                      \
    } while (0)
#define MPIU_THREADPRIV_KEY_DESTROY(...)

#endif /* defined(MPICH_TLS_SPECIFIER) */

#endif /* !defined(MPIU_THREAD_PRIV_H_INCLUDED) */
