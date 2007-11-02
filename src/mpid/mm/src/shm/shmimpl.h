/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef SHMIMPL_H
#define SHMIMPL_H

#include "mm_shm.h"

typedef struct SHM_PerProcess {
    MPID_Thread_lock_t lock;
		  char host[100];
} SHM_PerProcess;

extern SHM_PerProcess SHM_Process;

#endif
