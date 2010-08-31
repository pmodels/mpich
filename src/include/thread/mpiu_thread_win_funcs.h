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
