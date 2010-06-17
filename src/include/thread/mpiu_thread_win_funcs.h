/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIU_THREAD_WIN_FUNCS_H_INCLUDED
#define MPIU_THREAD_WIN_FUNCS_H_INCLUDED
/*
 * Thread Local Storage
 */

#define MPIU_Thread_tls_create(exit_func_ptr_, tls_ptr_, err_ptr_)	\
do{									\
    *(tls_ptr_) = TlsAlloc();		                                \
    if ((err_ptr_) != NULL)                                             \
    {                                                                   \
        if (*(tls_ptr_) == TLS_OUT_OF_INDEXES)				\
        {								\
	    *(int *)(err_ptr_) = GetLastError();			\
        }								\
        else                                                            \
        {                                                               \
            *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;                \
        }                                                               \
    }                                                                   \
}while(0)

#define MPIU_Thread_tls_destroy(tls_ptr_, err_ptr_)		\
do{								\
    BOOL result__;                                              \
    result__ = TlsFree(*(tls_ptr_));                            \
    if ((err_ptr_) != NULL)                                     \
    {                                                           \
        if (result__)                                           \
        {                                                       \
            *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;        \
        }                                                       \
        else                                                    \
        {							\
	    *(int *)(err_ptr_) = GetLastError();		\
        }							\
    }                                                           \
}while(0)

#define MPIU_Thread_tls_set(tls_ptr_, value_, err_ptr_)		\
do{								\
    BOOL result__;                                              \
    result__ = TlsSetValue(*(tls_ptr_), (value_));		\
    if ((err_ptr_) != NULL)                                     \
    {                                                           \
        if (result__)                                           \
        {                                                       \
            *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;        \
        }                                                       \
        else                                                    \
        {							\
	    *(int *)(err_ptr_) = GetLastError();		\
        }							\
    }                                                           \
}while(0)

#define MPIU_Thread_tls_get(tls_ptr_, value_ptr_, err_ptr_)	\
do{								\
    *((void **)value_ptr_) = TlsGetValue(*(tls_ptr_));		        \
    if ((err_ptr_) != NULL)                                     \
    {                                                           \
        if (*(value_ptr_) == 0 && GetLastError() != NO_ERROR)	\
        {							\
	    *(int *)(err_ptr_) = GetLastError();		\
        }							\
        else                                                    \
        {                                                       \
            *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;        \
        }                                                       \
    }                                                           \
}while(0)

#endif /* MPIU_THREAD_WIN_FUNCS_H_INCLUDED */
#if 0
/*
 * Threads
 */

/* MPIU_Thread_create() defined in mpiu_thread.c */

#define MPIU_Thread_exit()			\
{						\
    ExitThread(0);				\
}

#define MPIU_Thread_self(id_)			\
{						\
    *(id_) = GetCurrentThread();		\
}

#define MPIU_Thread_same(id1_, id2_, same_)		\
{							\
    *(same_) = (*(id1_) == *(id2_)) ? TRUE : FALSE;	\
}

#define MPIU_Thread_yield()				\
{							\
    Sleep(0);						\
}


/*
 *    Mutexes
 */

#define MPIU_Thread_mutex_create(mutex_ptr_, err_ptr_)		\
{								\
    *(mutex_ptr_) = CreateMutex(NULL, FALSE, NULL);		\
    if ((err_ptr_) != NULL)                                     \
    {                                                           \
        if (*(mutex_ptr_) == NULL)				\
        {							\
	    *(int *)(err_ptr_) = GetLastError();		\
        }							\
        else                                                    \
        {                                                       \
            *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;        \
        }                                                       \
    }                                                           \
}

#define MPIU_Thread_mutex_destroy(mutex_ptr_, err_ptr_)		\
{								\
    BOOL err__;							\
								\
    err__ = CloseHandle(*(mutex_ptr_));			        \
    if ((err_ptr_) != NULL)					\
    {								\
        if (err__)                                              \
        {                                                       \
	    *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;	\
	}                                                       \
	else                                                    \
	{                                                       \
	    *(int *)(err_ptr_) = GetLastError();		\
	}                                                       \
    }								\
}

#define MPIU_Thread_mutex_lock(mutex_ptr_, err_ptr_)		\
{								\
    DWORD err__;						\
								\
    err__ = WaitForSingleObject(*(mutex_ptr_), INFINITE);	\
    if ((err_ptr_) != NULL)					\
    {								\
        if (err__ == WAIT_OBJECT_0)                             \
        {                                                       \
            *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;        \
        }                                                       \
        else                                                    \
        {                                                       \
            if (err__ == WAIT_FAILED))                          \
	        *(int *)(err_ptr_) = GetLastError();		\
	    else                                                \
	        *(int *)(err_ptr_) = err__;                     \
	}                                                       \
    }								\
}

#define MPIU_Thread_mutex_unlock(mutex_ptr_, err_ptr_)		\
{								\
    BOOL result__;						\
								\
    result__ = ReleaseMutex(*(mutex_ptr_));			\
    if ((err_ptr_) != NULL)					\
    {								\
        if (result__)                                           \
	    *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;	\
	else                                                    \
	    *(int *)(err_ptr_) = GetLastError();                \
    }								\
}

#define MPIU_Thread_mutex_trylock(mutex_ptr_, flag_ptr_, err_ptr_)		\
{										\
    DWORD err__;						\
								\
    err__ = WaitForSingleObject(*(mutex_ptr_), 0);	        \
    if (err__ == WAIT_OBJECT_0)                                 \
    {                                                           \
        *(flag_ptr_) = TRUE;                                    \
        if ((err_ptr_) != NULL)                                 \
            *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;        \
    }                                                           \
    else                                                        \
    {                                                           \
        *(flag_ptr_) = FALSE;                                   \
        if ((err_ptr_) != NULL)                                 \
        {                                                       \
            if (err__ == WAIT_TIMEOUT)                          \
            {                                                   \
                *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;    \
            }                                                   \
            else                                                \
            {                                                   \
                if (err__ == WAIT_FAILED))                      \
                    *(int *)(err_ptr_) = GetLastError();	\
	        else                                            \
	            *(int *)(err_ptr_) = err__;                 \
	    }                                                   \
	}                                                       \
    }								\
}


/*
 * Condition Variables
 */

#define MPIU_Thread_cond_create(cond_ptr_, err_ptr_)		\
{								\
    *(cond_ptr_) = 0;	                                        \
    if ((err_ptr_) != NULL)					\
    {								\
        *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;            \
    }								\
}

#define MPIU_Thread_cond_destroy(cond_ptr_, err_ptr_)		\
{								\
    *(cond_ptr_) = -1;	                                        \
    if ((err_ptr_) != NULL)					\
    {								\
        *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;            \
    }								\
}

#define MPIU_Thread_cond_wait(cond_ptr_, mutex_ptr_, err_ptr_)	\
{								\
    if ((err_ptr_) != NULL)					\
    {								\
        *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;            \
    }								\
}

#define MPIU_Thread_cond_broadcast(cond_ptr_, err_ptr_)		\
{								\
    if ((err_ptr_) != NULL)					\
    {								\
        *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;            \
    }								\
}

#define MPIU_Thread_cond_signal(cond_ptr_, err_ptr_)		\
{								\
    if ((err_ptr_) != NULL)					\
    {								\
        *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;            \
    }                                                           \
}


/*
 * Thread Local Storage
 */

#define MPIU_Thread_tls_create(exit_func_ptr_, tls_ptr_, err_ptr_)	\
{									\
    *(tls_ptr_) = TlsAlloc();		                                \
    if ((err_ptr_) != NULL)                                             \
    {                                                                   \
        if (*(tls_ptr_) == TLS_OUT_OF_INDEXES)				\
        {								\
	    *(int *)(err_ptr_) = GetLastError();			\
        }								\
        else                                                            \
        {                                                               \
            *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;                \
        }                                                               \
    }                                                                   \
}

#define MPIU_Thread_tls_destroy(tls_ptr_, err_ptr_)		\
{								\
    BOOL result__;                                              \
    result__ = TlsFree(*(tls_ptr_));                            \
    if ((err_ptr_) != NULL)                                     \
    {                                                           \
        if (result__)                                           \
        {                                                       \
            *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;        \
        }                                                       \
        else                                                    \
        {							\
	    *(int *)(err_ptr_) = GetLastError();		\
        }							\
    }                                                           \
}

#define MPIU_Thread_tls_set(tls_ptr_, value_, err_ptr_)		\
{								\
    BOOL result__;                                              \
    result__ = TlsSetValue(*(tls_ptr_), (value_));		\
    if ((err_ptr_) != NULL)                                     \
    {                                                           \
        if (result__)                                           \
        {                                                       \
            *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;        \
        }                                                       \
        else                                                    \
        {							\
	    *(int *)(err_ptr_) = GetLastError();		\
        }							\
    }                                                           \
}

#define MPIU_Thread_tls_get(tls_ptr_, value_ptr_, err_ptr_)	\
{								\
    *(value_ptr_) = TlsGetValue(*(tls_ptr_));		        \
    if ((err_ptr_) != NULL)                                     \
    {                                                           \
        if (*(value_ptr_) == 0 && GetLastError() != NO_ERROR)	\
        {							\
	    *(int *)(err_ptr_) = GetLastError();		\
        }							\
        else                                                    \
        {                                                       \
            *(int *)(err_ptr_) = MPIU_THREAD_ERR_SUCCESS;        \
        }                                                       \
    }                                                           \
}

#endif
