/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_PRE_H
#define MPID_NEM_PRE_H

#include "mpid_nem_net_module_defs.h"
#include "mpid_nem_defs.h"

#if defined(HAVE_PTHREAD_H)
#include <pthread.h>
#endif

typedef pthread_mutex_t MPIDI_CH3I_SHM_MUTEX;

#define MPIDI_CH3_WIN_DECL                                                              \
    void *shm_base_addr;        /* base address of shared memory region */              \
    MPI_Aint shm_segment_len;   /* size of shared memory region */                      \
    MPIU_SHMW_Hnd_t shm_segment_handle; /* handle to shared memory region */            \
    MPIDI_CH3I_SHM_MUTEX *shm_mutex;    /* shared memory windows -- lock for            \
                                           accumulate/atomic operations */              \
    MPIU_SHMW_Hnd_t shm_mutex_segment_handle; /* handle to interprocess mutex memory    \
                                                 region */                              \
                                                                                        \
    void *info_shm_base_addr; /* base address of shared memory region for window info */          \
    MPI_Aint info_shm_segment_len; /* size of shared memory region for window info */             \
    MPIU_SHMW_Hnd_t info_shm_segment_handle; /* handle to shared memory region for window info */ \

#endif /* MPID_NEM_PRE_H */
