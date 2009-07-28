/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <errno.h>
#include <pthread.h>

typedef pthread_mutex_t MPIU_Thread_mutex_t;
typedef pthread_cond_t  MPIU_Thread_cond_t;
typedef pthread_t       MPIU_Thread_id_t;
typedef pthread_key_t   MPIU_Thread_tls_t;

#define MPIU_THREAD_TLS_T_NULL 0
