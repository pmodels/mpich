/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_PRE_H_INCLUDED
#define SHM_PRE_H_INCLUDED

#include <mpi.h>

#include "../posix/posix_pre.h"
#include "../ipc/src/ipc_pre.h"

typedef struct {
    int *local_ranks;
    MPIDI_POSIX_Global_t posix;
} MPIDI_SHM_Global_t;

#define MPIDI_SHM_global MPIDI_global.shm

#endif /* SHM_PRE_H_INCLUDED */
