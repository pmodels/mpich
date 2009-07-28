/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* For "historical" reasons windows.h includes winsock.h 
 * internally and WIN32_MEAN_AND_LEAN is to be defined
 * if we plan to include winsock2.h later on -- in the 
 * include hierarchy -- to prevent type redefinition 
 * errors...
 */

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

typedef HANDLE MPIU_Thread_mutex_t;
typedef HANDLE MPIU_Thread_id_t;
typedef DWORD MPIU_Thread_tls_t;

typedef struct MPIU_Thread_cond_fifo_t
{
    HANDLE event;
    struct MPIU_Thread_cond_fifo_t *next;
} MPIU_Thread_cond_fifo_t;
typedef struct MPIU_Thread_cond_t
{
    MPIU_Thread_tls_t tls;
    MPIU_Thread_mutex_t fifo_mutex;
    MPIU_Thread_cond_fifo_t *fifo_head, *fifo_tail;
} MPIU_Thread_cond_t;
