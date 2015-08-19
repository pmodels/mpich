/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* common header includes */
#include <stdlib.h>
#include "mpichconf.h"  /* defines MPICH_THREAD_PACKAGE_NAME */
#include "mpl.h"
#include "mpiutil.h"    /* for HAS_NO_SYMBOLS_WARNING */
#include "mpiu_thread.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

/* This file currently implements these as a preprocessor if/elif/else sequence.
 * This has the upside of not doing #includes for .c files or (poorly
 * named) .i files.  It has the downside of making this file large-ish
 * and a little harder to read in some cases.  If this becomes
 * unmanagable at some point these should be separated back out into
 * header files and included as needed. [goodell@ 2009-06-24] */

/* Implementation specific function definitions (usually in the form of macros) */

#if defined(MPICH_THREAD_PACKAGE_NAME) && (MPICH_THREAD_PACKAGE_NAME == MPICH_THREAD_PACKAGE_WIN)
/* begin win impl */

#include "mpimem.h"

/*
 * struct MPIUI_Thread_info
 *
 * Structure used to pass the user function and data to the intermediate function, MPIUI_Thread_start.  See comment in
 * MPIUI_Thread_start() header for more information.
 */
struct MPIUI_Thread_info {
    MPIU_Thread_func_t func;
    void *data;
};


DWORD WINAPI MPIUI_Thread_start(LPVOID arg);

/*
 * MPIU_Thread_create()
 */
void MPIU_Thread_create(MPIU_Thread_func_t func, void *data, MPIU_Thread_id_t * idp, int *errp)
{
    struct MPIUI_Thread_info *thread_info;
    int err = MPIU_THREAD_SUCCESS;

    thread_info = (struct MPIUI_Thread_info *) MPIU_Malloc(sizeof(struct MPIUI_Thread_info));
    if (thread_info != NULL) {
        thread_info->func = func;
        thread_info->data = data;
        *idp = CreateThread(NULL, 0, MPIUI_Thread_start, thread_info, 0, NULL);
        if (*idp == NULL) {
            err = GetLastError();
        }
    }
    else {
        err = 1000000000;
    }

    if (errp != NULL) {
        *errp = err;
    }
}


/*
 * MPIUI_Thread_start()
 *
 * Start functions in Windows are expected to return a DWORD.  Since our start functions do not return a value we must
 * use an intermediate function to perform the call to the user's start function and then return a value of 0.
 */
DWORD WINAPI MPIUI_Thread_start(LPVOID arg)
{
    struct MPIUI_Thread_info *thread_info = (struct MPIUI_Thread_info *) arg;
    MPIU_Thread_func_t func = thread_info->func;
    void *data = thread_info->data;

    MPIU_Free(arg);

    func(data);

    return 0;
}

void MPIU_Thread_exit()
{
    ExitThread(0);
}

void MPIU_Thread_self(MPIU_Thread_id_t * id)
{
    *id = GetCurrentThread();
}

void MPIU_Thread_same(MPIU_Thread_id_t * id1, MPIU_Thread_id_t * id2, int *same)
{
    *same = (*id1 == *id2) ? TRUE : FALSE;
}

void MPIU_Thread_yield(MPIU_Thread_mutex_t * mutex)
{
    int err;

    MPIU_Thread_mutex_unlock(mutex, &err);
    Sleep(0);
    MPIU_Thread_mutex_lock(mutex, &err);
}

/*
 *    Mutexes
 */

void MPIU_Thread_mutex_create(MPIU_Thread_mutex_t * mutex, int *err)
{
    *mutex = CreateMutex(NULL, FALSE, NULL);
    if (err != NULL) {
        if (*mutex == NULL) {
            *err = GetLastError();
        }
        else {
            *err = MPIU_THREAD_SUCCESS;
        }
    }
}

void MPIU_Thread_mutex_destroy(MPIU_Thread_mutex_t * mutex, int *err)
{
    BOOL result;

    result = CloseHandle(*mutex);
    if (err != NULL) {
        if (result) {
            *err = MPIU_THREAD_SUCCESS;
        }
        else {
            *err = GetLastError();
        }
    }
}

void MPIU_Thread_mutex_lock(MPIU_Thread_mutex_t * mutex, int *err)
{
    DWORD result;

    result = WaitForSingleObject(*mutex, INFINITE);
    if (err != NULL) {
        if (result == WAIT_OBJECT_0) {
            *err = MPIU_THREAD_SUCCESS;
        }
        else {
            if (result == WAIT_FAILED) {
                *err = GetLastError();
            }
            else {
                *err = result;
            }
        }
    }
}

void MPIU_Thread_mutex_unlock(MPIU_Thread_mutex_t * mutex, int *err)
{
    BOOL result;

    result = ReleaseMutex(*mutex);
    if (err != NULL) {
        if (result) {
            *err = MPIU_THREAD_SUCCESS;
        }
        else {
            *err = GetLastError();
        }
    }
}


/*
 * Condition Variables
 */

void MPIU_Thread_cond_create(MPIU_Thread_cond_t * cond, int *err)
{
    /* Create a tls slot to store the events used to wakeup each thread in cond_bcast or cond_signal */
    MPIU_Thread_tls_create(NULL, &cond->tls, err);
    if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
        return;
    }
    /* Create a mutex to protect the fifo queue.  This is required because the mutex passed in to the
     * cond functions need not be the same in each thread. */
    MPIU_Thread_mutex_create(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
        return;
    }
    cond->fifo_head = NULL;
    cond->fifo_tail = NULL;
    if (err != NULL) {
        *err = MPIU_THREAD_SUCCESS;
    }
}

void MPIU_Thread_cond_destroy(MPIU_Thread_cond_t * cond, int *err)
{
    MPIUI_Win_thread_cond_fifo_t *iter;

    while (cond->fifo_head) {
        iter = cond->fifo_head;
        cond->fifo_head = cond->fifo_head->next;
        MPIU_Free(iter);
    }
    MPIU_Thread_mutex_destroy(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
        return;
    }
    MPIU_Thread_tls_destroy(&cond->tls, err);
    /*
     * if (err != NULL)
     * {
     * *err = MPIU_THREAD_SUCCESS;
     * }
     */
}

void MPIU_Thread_cond_wait(MPIU_Thread_cond_t * cond, MPIU_Thread_mutex_t * mutex, int *err)
{
    HANDLE event;
    DWORD result;
    MPIU_Thread_tls_get(&cond->tls, &event, err);
    if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
        return;
    }
    if (event == NULL) {
        event = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (event == NULL) {
            if (err != NULL) {
                *err = GetLastError();
            }
            return;
        }
        MPIU_Thread_tls_set(&cond->tls, event, err);
        if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
            return;
        }
    }
    MPIU_Thread_mutex_lock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
        return;
    }
    if (cond->fifo_tail == NULL) {
        cond->fifo_tail = (MPIUI_Win_thread_cond_fifo_t *) MPIU_Malloc(sizeof(MPIUI_Win_thread_cond_fifo_t));
        cond->fifo_head = cond->fifo_tail;
    }
    else {
        cond->fifo_tail->next =
            (MPIUI_Win_thread_cond_fifo_t *) MPIU_Malloc(sizeof(MPIUI_Win_thread_cond_fifo_t));
        cond->fifo_tail = cond->fifo_tail->next;
    }
    if (cond->fifo_tail == NULL) {
        if (err != NULL) {
            *err = -1;
        }
        return;
    }
    cond->fifo_tail->event = event;
    cond->fifo_tail->next = NULL;
    MPIU_Thread_mutex_unlock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
        return;
    }
    MPIU_Thread_mutex_unlock(mutex, err);
    if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
        return;
    }
    result = WaitForSingleObject(event, INFINITE);
    if (err != NULL) {
        if (result != WAIT_OBJECT_0) {
            if (result == WAIT_FAILED) {
                *err = GetLastError();
            }
            else {
                *err = result;
            }
            return;
        }
    }
    result = ResetEvent(event);
    if (!result && err != NULL) {
        *err = GetLastError();
        return;
    }
    MPIU_Thread_mutex_lock(mutex, err);
    /*
     * if (err != NULL)
     * {
     * *err = MPIU_THREAD_SUCCESS;
     * }
     */
}

void MPIU_Thread_cond_broadcast(MPIU_Thread_cond_t * cond, int *err)
{
    MPIUI_Win_thread_cond_fifo_t *fifo, *temp;
    MPIU_Thread_mutex_lock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
        return;
    }
    /* remove the fifo queue from the cond variable */
    fifo = cond->fifo_head;
    cond->fifo_head = cond->fifo_tail = NULL;
    MPIU_Thread_mutex_unlock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
        return;
    }
    /* signal each event in the fifo queue */
    while (fifo) {
        if (!SetEvent(fifo->event) && err != NULL) {
            *err = GetLastError();
            /* lost memory */
            return;
        }
        temp = fifo;
        fifo = fifo->next;
        MPIU_Free(temp);
    }
    if (err != NULL) {
        *err = MPIU_THREAD_SUCCESS;
    }
}

void MPIU_Thread_cond_signal(MPIU_Thread_cond_t * cond, int *err)
{
    MPIUI_Win_thread_cond_fifo_t *fifo;
    MPIU_Thread_mutex_lock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
        return;
    }
    fifo = cond->fifo_head;
    if (fifo) {
        cond->fifo_head = cond->fifo_head->next;
        if (cond->fifo_head == NULL)
            cond->fifo_tail = NULL;
    }
    MPIU_Thread_mutex_unlock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPIU_THREAD_SUCCESS) {
        return;
    }
    if (fifo) {
        if (!SetEvent(fifo->event) && err != NULL) {
            *err = GetLastError();
            MPIU_Free(fifo);
            return;
        }
        MPIU_Free(fifo);
    }
    if (err != NULL) {
        *err = MPIU_THREAD_SUCCESS;
    }
}


/*
 * Thread Local Storage
 * - Defined in src/include/thread/mpiu_thread_win_funcs.h
 */
/* end win impl */
#endif
