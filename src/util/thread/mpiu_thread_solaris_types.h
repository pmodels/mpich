/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIU_THREAD_SOLARIS_TYPES_H_INCLUDED
#define MPIU_THREAD_SOLARIS_TYPES_H_INCLUDED

#include <thread.h>
#include <synch.h>

typedef mutex_t MPIU_Thread_mutex_t;
typedef cond_t MPIU_Thread_cond_t;
typedef thread_t MPIU_Thread_id_t;
typedef thread_key_t MPIU_Thread_key_t;

#endif /* MPIU_THREAD_SOLARIS_TYPES_H_INCLUDED */
