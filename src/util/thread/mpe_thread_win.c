/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include "mpe_thread.h"
#include "mpimem.h"

/*
 * struct MPEI_Thread_info
 *
 * Structure used to pass the user function and data to the intermediate function, MPEI_Thread_start.  See comment in
 * MPEI_Thread_start() header for more information.
 */
struct MPEI_Thread_info
{
    MPE_Thread_func_t func;
    void * data;
};


DWORD WINAPI MPEI_Thread_start(LPVOID arg);

/*
 * MPE_Thread_create()
 */
void MPE_Thread_create(MPE_Thread_func_t func, void * data, MPE_Thread_id_t * idp, int * errp)
{
    struct MPEI_Thread_info * thread_info;
    int err = MPE_THREAD_SUCCESS;

    thread_info = (struct MPEI_Thread_info *) MPIU_Malloc(sizeof(struct MPEI_Thread_info));
    if (thread_info != NULL)
    {
	thread_info->func = func;
	thread_info->data = data;
	*idp = CreateThread(NULL, 0, MPEI_Thread_start, thread_info, 0, NULL);
	if (*idp == NULL)
	{
	    err = GetLastError();
	}
    }
    else
    {
	err = 1000000000;
    }
    
    if (errp != NULL)
    {
	*errp = err;
    }
}


/*
 * MPEI_THread_start()
 *
 * Start functions in Windows are expected to return a DWORD.  Since our start functions do not return a value we must
 * use an intermediate function to perform the call to the user's start function and then return a value of 0.
 */
DWORD WINAPI MPEI_Thread_start(LPVOID arg)
{
    struct MPEI_Thread_info * thread_info = (struct MPEI_Thread_info *) arg;
    MPE_Thread_func_t func = thread_info->func;
    void * data = thread_info->data;

    MPIU_Free(arg);

    func(data);
    
    return 0;
}

void MPE_Thread_exit()
{
    ExitThread(0);
}

void MPE_Thread_self(MPE_Thread_id_t * id)
{
    *id = GetCurrentThread();
}

void MPE_Thread_same(MPE_Thread_id_t * id1, MPE_Thread_id_t * id2, int * same)
{
    *same = (*id1 == *id2) ? TRUE : FALSE;
}

void MPE_Thread_yield()
{
    Sleep(0);
}

/*
 *    Mutexes
 */

void MPE_Thread_mutex_create(MPE_Thread_mutex_t * mutex, int * err)
{
    *mutex = CreateMutex(NULL, FALSE, NULL);
    if (err != NULL)
    {
        if (*mutex == NULL)
        {
	    *err = GetLastError();
        }
        else
        {
            *err = MPE_THREAD_SUCCESS;
        }
    }
}

void MPE_Thread_mutex_destroy(MPE_Thread_mutex_t * mutex, int * err)
{
    BOOL result;

    result = CloseHandle(*mutex);
    if (err != NULL)
    {
        if (result)
        {
	    *err = MPE_THREAD_SUCCESS;
	}
	else
	{
	    *err = GetLastError();
	}
    }
}

void MPE_Thread_mutex_lock(MPE_Thread_mutex_t * mutex, int * err)
{
    DWORD result;

    result = WaitForSingleObject(*mutex, INFINITE);
    if (err != NULL)
    {
        if (result == WAIT_OBJECT_0)
        {
            *err = MPE_THREAD_SUCCESS;
        }
        else
        {
            if (result == WAIT_FAILED)
	    {
	        *err = GetLastError();
	    }
	    else
	    {
	        *err = result;
	    }
	}
    }
}

void MPE_Thread_mutex_unlock(MPE_Thread_mutex_t * mutex, int * err)
{
    BOOL result;

    result = ReleaseMutex(*mutex);
    if (err != NULL)
    {
        if (result)
	{
	    *err = MPE_THREAD_SUCCESS;
	}
	else
	{
	    *err = GetLastError();
	}
    }
}

void MPE_Thread_mutex_trylock(MPE_Thread_mutex_t * mutex, int * flag, int * err)
{
    DWORD result;

    result = WaitForSingleObject(*mutex, 0);
    if (result == WAIT_OBJECT_0)
    {
        *flag = TRUE;
        if (err != NULL)
	{
            *err = MPE_THREAD_SUCCESS;
	}
    }
    else
    {
        *flag = FALSE;
        if (err != NULL)
        {
            if (result == WAIT_TIMEOUT)
            {
                *err = MPE_THREAD_SUCCESS;
            }
            else
            {
                if (result == WAIT_FAILED)
		{
                    *err = GetLastError();
		}
	        else
		{
	            *err = result;
		}
	    }
	}
    }
}

/*
 * Condition Variables
 */

void MPE_Thread_cond_create(MPE_Thread_cond_t * cond, int * err)
{
    /* Create a tls slot to store the events used to wakeup each thread in cond_bcast or cond_signal */
    MPE_Thread_tls_create(NULL, &cond->tls, err);
    if (err != NULL && *err != MPE_THREAD_SUCCESS)
    {
	return;
    }
    /* Create a mutex to protect the fifo queue.  This is required because the mutex passed in to the
       cond functions need not be the same in each thread. */
    MPE_Thread_mutex_create(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPE_THREAD_SUCCESS)
    {
	return;
    }
    cond->fifo_head = NULL;
    cond->fifo_tail = NULL;
    if (err != NULL)
    {
        *err = MPE_THREAD_SUCCESS;
    }
}

void MPE_Thread_cond_destroy(MPE_Thread_cond_t * cond, int * err)
{
    MPE_Thread_cond_fifo_t *iter;

    while (cond->fifo_head)
    {
	iter = cond->fifo_head;
	cond->fifo_head = cond->fifo_head->next;
	MPIU_Free(iter);
    }
    MPE_Thread_mutex_destroy(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPE_THREAD_SUCCESS)
    {
	return;
    }
    MPE_Thread_tls_destroy(&cond->tls, err);
    /*
    if (err != NULL)
    {
        *err = MPE_THREAD_SUCCESS;
    }
    */
}

void MPE_Thread_cond_wait(MPE_Thread_cond_t * cond, MPE_Thread_mutex_t * mutex, int * err)
{
    HANDLE event;
    DWORD result;
    MPE_Thread_tls_get(&cond->tls, &event, err);
    if (err != NULL && *err != MPE_THREAD_SUCCESS)
    {
	return;
    }
    if (event == NULL)
    {
	event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (event == NULL)
	{
	    if (err != NULL)
	    {
		*err = GetLastError();
	    }
	    return;
	}
	MPE_Thread_tls_set(&cond->tls, event, err);
	if (err != NULL && *err != MPE_THREAD_SUCCESS)
	{
	    return;
	}
    }
    MPE_Thread_mutex_lock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPE_THREAD_SUCCESS)
    {
	return;
    }
    if (cond->fifo_tail == NULL)
    {
	cond->fifo_tail = (MPE_Thread_cond_fifo_t*)MPIU_Malloc(sizeof(MPE_Thread_cond_fifo_t));
	cond->fifo_head = cond->fifo_tail;
    }
    else
    {
	cond->fifo_tail->next = (MPE_Thread_cond_fifo_t*)MPIU_Malloc(sizeof(MPE_Thread_cond_fifo_t));
	cond->fifo_tail = cond->fifo_tail->next;
    }
    if (cond->fifo_tail == NULL)
    {
	if (err != NULL)
	{
	    *err = -1;
	}
	return;
    }
    cond->fifo_tail->event = event;
    cond->fifo_tail->next = NULL;
    MPE_Thread_mutex_unlock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPE_THREAD_SUCCESS)
    {
	return;
    }
    MPE_Thread_mutex_unlock(mutex, err);
    if (err != NULL && *err != MPE_THREAD_SUCCESS)
    {
	return;
    }
    result = WaitForSingleObject(event, INFINITE);
    if (err != NULL)
    {
        if (result != WAIT_OBJECT_0)
        {
            if (result == WAIT_FAILED)
	    {
	        *err = GetLastError();
	    }
	    else
	    {
	        *err = result;
	    }
	    return;
	}
    }
    result = ResetEvent(event);
    if (!result && err != NULL)
    {
	*err = GetLastError();
	return;
    }
    MPE_Thread_mutex_lock(mutex, err);
    /*
    if (err != NULL)
    {
        *err = MPE_THREAD_SUCCESS;
    }
    */
}

void MPE_Thread_cond_broadcast(MPE_Thread_cond_t * cond, int * err)
{
    MPE_Thread_cond_fifo_t *fifo, *temp;
    MPE_Thread_mutex_lock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPE_THREAD_SUCCESS)
    {
	return;
    }
    /* remove the fifo queue from the cond variable */
    fifo = cond->fifo_head;
    cond->fifo_head = cond->fifo_tail = NULL;
    MPE_Thread_mutex_unlock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPE_THREAD_SUCCESS)
    {
	return;
    }
    /* signal each event in the fifo queue */
    while (fifo)
    {
	if (!SetEvent(fifo->event) && err != NULL)
	{
	    *err = GetLastError();
	    /* lost memory */
	    return;
	}
	temp = fifo;
	fifo = fifo->next;
	MPIU_Free(temp);
    }
    if (err != NULL)
    {
        *err = MPE_THREAD_SUCCESS;
    }
}

void MPE_Thread_cond_signal(MPE_Thread_cond_t * cond, int * err)
{
    MPE_Thread_cond_fifo_t *fifo;
    MPE_Thread_mutex_lock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPE_THREAD_SUCCESS)
    {
	return;
    }
    fifo = cond->fifo_head;
    if (fifo)
    {
	cond->fifo_head = cond->fifo_head->next;
	if (cond->fifo_head == NULL)
	    cond->fifo_tail = NULL;
    }
    MPE_Thread_mutex_unlock(&cond->fifo_mutex, err);
    if (err != NULL && *err != MPE_THREAD_SUCCESS)
    {
	return;
    }
    if (fifo)
    {
	if (!SetEvent(fifo->event) && err != NULL)
	{
	    *err = GetLastError();
	    MPIU_Free(fifo);
	    return;
	}
	MPIU_Free(fifo);
    }
    if (err != NULL)
    {
        *err = MPE_THREAD_SUCCESS;
    }
}


/*
 * Thread Local Storage
 */

void MPE_Thread_tls_create(MPE_Thread_tls_exit_func_t exit_func, MPE_Thread_tls_t * tls, int * err)
{
    *tls = TlsAlloc();
    if (err != NULL)
    {
        if (*tls == TLS_OUT_OF_INDEXES)
        {
	    *err = GetLastError();
        }
        else
        {
            *err = MPE_THREAD_SUCCESS;
        }
    }
}

void MPE_Thread_tls_destroy(MPE_Thread_tls_t * tls, int * err)
{
    BOOL result;

    result = TlsFree(*tls);
    if (err != NULL)
    {
        if (result)
        {
            *err = MPE_THREAD_SUCCESS;
        }
        else
        {
	    *err = GetLastError();
        }
    }
}

void MPE_Thread_tls_set(MPE_Thread_tls_t * tls, void * value, int * err)
{
    BOOL result;

    result = TlsSetValue(*tls, value);
    if (err != NULL)
    {
        if (result)
        {
            *err = MPE_THREAD_SUCCESS;
        }
        else
        {
	    *err = GetLastError();
        }
    }
}

void MPE_Thread_tls_get(MPE_Thread_tls_t * tls, void ** value, int * err)
{
    *value = TlsGetValue(*tls);
    if (err != NULL)
    {
        if (*value == 0 && GetLastError() != NO_ERROR)
        {
	    *err = GetLastError();
        }
        else
        {
            *err = MPE_THREAD_SUCCESS;
        }
    }
}
