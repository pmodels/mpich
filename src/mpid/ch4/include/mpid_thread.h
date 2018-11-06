/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef MPID_THREAD_H_INCLUDED
#define MPID_THREAD_H_INCLUDED

#include "mpidu_thread_fallback.h"

/* We simply use the fallback timer functionality and do not define
 * our own */

typedef MPIDU_Thread_cond_t MPID_Thread_cond_t;
typedef MPIDU_Thread_id_t MPID_Thread_id_t;
typedef MPIDU_Thread_tls_t MPID_Thread_tls_t;
typedef MPIDU_Thread_func_t MPID_Thread_func_t;

#ifdef MPIDI_CH4_USE_TICKET_LOCK
#include "mpid_ticketlock.h"
typedef MPIDI_CH4_Ticket_lock MPID_Thread_mutex_t;
#define MPID_THREAD_CS_ENTER       MPIDI_CH4I_THREAD_CS_ENTER
#define MPID_THREAD_CS_EXIT        MPIDI_CH4I_THREAD_CS_EXIT
#define MPID_THREAD_CS_YIELD       MPIDI_CH4I_THREAD_CS_YIELD
#define MPID_Thread_mutex_create   MPIDI_CH4I_Thread_mutex_create
#define MPID_Thread_mutex_destroy  MPIDI_CH4I_Thread_mutex_destroy
#define MPID_Thread_mutex_lock     MPIDI_CH4I_Thread_mutex_lock
#define MPID_Thread_mutex_unlock   MPIDI_CH4I_Thread_mutex_unlock
#define MPID_Thread_cond_wait      MPIDI_CH4I_Thread_cond_wait
#else
typedef MPIDU_Thread_mutex_t MPID_Thread_mutex_t;
typedef MPIDU_thread_mutex_state_t MPID_Thread_mutex_state_t;
#define MPID_THREAD_CS_ENTER       MPIDU_THREAD_CS_ENTER
#define MPID_THREAD_CS_TRYENTER    MPIDU_THREAD_CS_TRYENTER
#define MPID_THREAD_CS_EXIT        MPIDU_THREAD_CS_EXIT
#define MPID_THREAD_CS_ENTER_ST    MPIDU_THREAD_CS_ENTER_ST
#define MPID_THREAD_CS_EXIT_ST     MPIDU_THREAD_CS_EXIT_ST
#define MPID_THREAD_CS_YIELD       MPIDU_THREAD_CS_YIELD

#define MPID_THREAD_SAFE_BEGIN(name, mutex, cs_acq)                     \
do {                                                                    \
    cs_acq = 1;                                                         \
    if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_TRYLOCK) {                   \
        MPID_THREAD_CS_TRYENTER(name, mutex, cs_acq);                   \
    } else if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_HANDOFF) {            \
        cs_acq = 0;                                                     \
    } else {                                                            \
        /* Direct */                                                    \
        MPIDU_THREAD_CS_ENTER(name, mutex);                             \
    }                                                                   \
} while (0)

#define MPID_THREAD_SAFE_END(name, mutex, cs_acq)                       \
do {                                                                    \
    if (cs_acq)                                                         \
        MPIDU_THREAD_CS_EXIT(name, mutex);                              \
} while (0)

#define MPID_Thread_mutex_create   MPIDU_Thread_mutex_create
#define MPID_Thread_mutex_destroy  MPIDU_Thread_mutex_destroy
#define MPID_Thread_mutex_lock     MPIDU_Thread_mutex_lock
#define MPID_Thread_mutex_unlock   MPIDU_Thread_mutex_unlock
#define MPID_Thread_cond_wait      MPIDU_Thread_cond_wait
#endif /* MPIDI_CH4_USE_TICKET_LOCK */

#define MPID_Thread_create       MPIDU_Thread_create
#define MPID_Thread_exit         MPIDU_Thread_exit
#define MPID_Thread_self         MPIDU_Thread_self
#define MPID_Thread_same       MPIDU_Thread_same
#define MPID_Thread_same       MPIDU_Thread_same

#define MPID_Thread_cond_create MPIDU_Thread_cond_create
#define MPID_Thread_cond_destroy MPIDU_Thread_cond_destroy
#define MPID_Thread_cond_broadcast MPIDU_Thread_cond_broadcast
#define MPID_Thread_cond_signal MPIDU_Thread_cond_signal

#define MPID_Thread_tls_create MPIDU_Thread_tls_create
#define MPID_Thread_tls_destroy MPIDU_Thread_tls_destroy
#define MPID_Thread_tls_set MPIDU_Thread_tls_set
#define MPID_Thread_tls_get MPIDU_Thread_tls_get

#define MPID_THREADPRIV_KEY_CREATE  MPIDU_THREADPRIV_KEY_CREATE
#define MPID_THREADPRIV_KEY_GET_ADDR MPIDU_THREADPRIV_KEY_GET_ADDR
#define MPID_THREADPRIV_KEY_DESTROY MPIDU_THREADPRIV_KEY_DESTROY


#endif /* MPID_THREAD_H_INCLUDED */
