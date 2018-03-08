/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_EAGER_FBOX_TYPES_H_INCLUDED
#define POSIX_EAGER_FBOX_TYPES_H_INCLUDED

#include <mpidimpl.h>
#include "mpidu_shm.h"

#define MPIDI_POSIX_FBOX_DATA_LEN  (16 * 1024 - sizeof(uint64_t) - 2 * sizeof(int))
#define MPIDI_POSIX_FBOX_THRESHOLD (MPIDI_POSIX_FBOX_DATA_LEN)

typedef struct {

    volatile uint64_t flag;

    int is_header;
    int payload_sz;

    uint8_t payload[MPIDI_POSIX_FBOX_DATA_LEN];

} MPIDI_POSIX_fastbox_t;

typedef struct MPIDI_POSIX_fbox_arrays {
    MPIDI_POSIX_fastbox_t **in;
    MPIDI_POSIX_fastbox_t **out;
} MPIDI_POSIX_fbox_arrays_t;

typedef struct MPIDI_POSIX_eager_fbox_control {

    MPIDU_shm_seg_t memory;
    MPIDU_shm_seg_info_t *seg;

    int num_seg;

    MPIDI_POSIX_fbox_arrays_t mailboxes;

    MPIDU_shm_barrier_t *barrier;
    void *barrier_region;

    int num_local;
    int local_rank;
    int *local_ranks;
    int *local_procs;

    int next_poll_local_rank;

} MPIDI_POSIX_eager_fbox_control_t;

extern MPIDI_POSIX_eager_fbox_control_t MPIDI_POSIX_eager_fbox_control_global;
extern MPL_dbg_class MPIDI_CH4_SHM_POSIX_FBOX_GENERAL;

#define POSIX_FBOX_TRACE(...) \
    MPL_DBG_MSG_FMT(MPIDI_CH4_SHM_POSIX_FBOX_GENERAL,VERBOSE,(MPL_DBG_FDEST, __VA_ARGS__))

#endif /* POSIX_EAGER_FBOX_TYPES_H_INCLUDED */
