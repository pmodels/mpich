/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_PRE_H_INCLUDED
#define SHM_PRE_H_INCLUDED

#include <mpi.h>

#include "../posix/posix_pre.h"
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
#include "../xpmem/xpmem_pre.h"
#endif

#ifdef MPIDI_CH4_SHM_ENABLE_GPU_IPC
#include "../gpu/gpu_pre.h"
#endif

typedef struct {
    MPIDI_POSIX_Global_t posix;
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    MPIDI_XPMEM_Global_t xpmem;
#endif
#ifdef MPIDI_CH4_SHM_ENABLE_GPU_IPC
    MPIDI_GPU_Global_t gpu;
#endif
} MPIDI_SHM_Global_t;

#endif /* SHM_PRE_H_INCLUDED */
