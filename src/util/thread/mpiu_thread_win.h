/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIU_THREAD_WIN_H_INCLUDED
#define MPIU_THREAD_WIN_H_INCLUDED

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

typedef HANDLE MPIU_Thread_mutex_t;
typedef HANDLE MPIU_Thread_id_t;
typedef DWORD MPIU_Thread_tls_t;

typedef struct MPIUI_Win_thread_cond_fifo_t {
    HANDLE event;
    struct MPIUI_Win_thread_cond_fifo_t *next;
} MPIUI_Win_thread_cond_fifo_t;
typedef struct MPIU_Thread_cond_t {
    MPIU_Thread_tls_t tls;
    MPIU_Thread_mutex_t fifo_mutex;
    MPIUI_Win_thread_cond_fifo_t *fifo_head, *fifo_tail;
} MPIU_Thread_cond_t;

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

/*
 * Thread Local Storage
 */

#define MPIU_Thread_tls_create(exit_func_ptr_, tls_ptr_, err_ptr_)	\
    do {                                                                \
        *(tls_ptr_) = TlsAlloc();                                       \
        if ((err_ptr_) != NULL) {                                       \
            if (*(tls_ptr_) == TLS_OUT_OF_INDEXES) {                    \
                *(int *)(err_ptr_) = GetLastError();			\
            }								\
            else {                                                      \
                *(int *)(err_ptr_) = MPIU_THREAD_SUCCESS;               \
            }                                                           \
        }                                                               \
    } while (0)

#define MPIU_Thread_tls_destroy(tls_ptr_, err_ptr_)		\
    do {                                                        \
        BOOL result__;                                          \
        result__ = TlsFree(*(tls_ptr_));                        \
        if ((err_ptr_) != NULL) {                               \
            if (result__) {                                     \
                *(int *)(err_ptr_) = MPIU_THREAD_SUCCESS;       \
            }                                                   \
            else {                                              \
                *(int *)(err_ptr_) = GetLastError();		\
            }							\
        }                                                       \
    } while (0)

#define MPIU_Thread_tls_set(tls_ptr_, value_, err_ptr_)		\
    do {                                                        \
        BOOL result__;                                          \
        result__ = TlsSetValue(*(tls_ptr_), (value_));		\
        if ((err_ptr_) != NULL) {                               \
            if (result__) {                                     \
                *(int *)(err_ptr_) = MPIU_THREAD_SUCCESS;       \
            }                                                   \
            else {                                              \
                *(int *)(err_ptr_) = GetLastError();		\
            }							\
        }                                                       \
    } while (0)

#define MPIU_Thread_tls_get(tls_ptr_, value_ptr_, err_ptr_)             \
    do {								\
        *((void **)value_ptr_) = TlsGetValue(*(tls_ptr_));              \
        if ((err_ptr_) != NULL) {                                       \
            if (*(value_ptr_) == 0 && GetLastError() != NO_ERROR) {     \
                *(int *)(err_ptr_) = GetLastError();                    \
            }                                                           \
            else {                                                      \
                *(int *)(err_ptr_) = MPIU_THREAD_SUCCESS;               \
            }                                                           \
        }                                                               \
    } while (0)

#endif /* MPIU_THREAD_WIN_H_INCLUDED */
