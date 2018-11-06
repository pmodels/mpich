/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_THREAD_H_INCLUDED
#define MPID_THREAD_H_INCLUDED

#include "mpidu_thread_fallback.h"

/* We simply use the fallback timer functionality and do not define
 * our own */

typedef MPIDU_Thread_mutex_t MPID_Thread_mutex_t;
typedef MPIDU_thread_mutex_state_t MPID_thread_mutex_state_t;
typedef MPIDU_Thread_cond_t  MPID_Thread_cond_t;
typedef MPIDU_Thread_id_t    MPID_Thread_id_t;
typedef MPIDU_Thread_tls_t   MPID_Thread_tls_t;
typedef MPIDU_Thread_func_t  MPID_Thread_func_t;

#define MPID_THREAD_CS_ENTER       MPIDU_THREAD_CS_ENTER
#define MPID_THREAD_CS_EXIT        MPIDU_THREAD_CS_EXIT
#define MPID_THREAD_CS_ENTER_ST    MPIDU_THREAD_CS_ENTER_ST
#define MPID_THREAD_CS_EXIT_ST     MPIDU_THREAD_CS_EXIT_ST
#define MPID_THREAD_CS_YIELD       MPIDU_THREAD_CS_YIELD

#define MPID_Thread_create       MPIDU_Thread_create
#define MPID_Thread_exit         MPIDU_Thread_exit
#define MPID_Thread_self         MPIDU_Thread_self
#define MPID_Thread_same       MPIDU_Thread_same
#define MPID_Thread_yield        MPIDU_Thread_yield

#define MPID_Thread_mutex_create  MPIDU_Thread_mutex_create
#define MPID_Thread_mutex_destroy  MPIDU_Thread_mutex_destroy
#define MPID_Thread_mutex_lock MPIDU_Thread_mutex_lock
#define MPID_Thread_mutex_unlock MPIDU_Thread_mutex_unlock

#define MPID_Thread_cond_create MPIDU_Thread_cond_create
#define MPID_Thread_cond_destroy MPIDU_Thread_cond_destroy
#define MPID_Thread_cond_wait MPIDU_Thread_cond_wait
#define MPID_Thread_cond_broadcast MPIDU_Thread_cond_broadcast
#define MPID_Thread_cond_signal MPIDU_Thread_cond_signal

#define MPID_Thread_tls_create MPIDU_Thread_tls_create
#define MPID_Thread_tls_destroy MPIDU_Thread_tls_destroy
#define MPID_Thread_tls_set MPIDU_Thread_tls_set
#define MPID_Thread_tls_get MPIDU_Thread_tls_get

#define MPID_THREADPRIV_KEY_CREATE  MPIDU_THREADPRIV_KEY_CREATE
#define MPID_THREADPRIV_KEY_GET_ADDR    MPIDU_THREADPRIV_KEY_GET_ADDR
#define MPID_THREADPRIV_KEY_DESTROY MPIDU_THREADPRIV_KEY_DESTROY

#endif /* MPID_THREAD_H_INCLUDED */
