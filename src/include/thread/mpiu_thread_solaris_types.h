/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <thread.h>
#include <synch.h>

typedef mutex_t MPIU_Thread_mutex_t;
typedef cond_t MPIU_Thread_cond_t;
typedef thread_t MPIU_Thread_id_t;
typedef thread_key_t MPIU_Thread_key_t;
