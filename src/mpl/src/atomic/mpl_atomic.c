/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl_atomic.h"

#ifdef MPL_USE_LOCK_BASED_PRIMITIVES

/* FIXME For now we rely on pthreads for our IPC locks.  This is fairly
 * portable, although it is obviously not 100% portable.  We need to figure out
 * how to support other threading packages and lock implementations, such as the
 * BG/P lockbox. */

#include <pthread.h>

pthread_mutex_t MPL_emulation_lock = PTHREAD_MUTEX_INITIALIZER;

#endif /* MPL_USE_LOCK_BASED_PRIMITIVES */
