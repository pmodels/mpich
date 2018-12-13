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

    volatile uint64_t data_ready;

    int is_header;
    size_t payload_sz;

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

    MPIDI_POSIX_fbox_arrays_t mailboxes;        /* The array of buffers that make up the total collection
                                                 * of mailboxes */

    /* A small cache of local ranks that have posted receives that we use to poll fastboxes more
     * efficiently. The last entry in this array is a counter to keep track of the most recently
     * checked fastbox so we can make sure we don't starve the fastboxes where receives haven't been
     * posted (for unexpected messages). Ideally this array should remain small. By default, it has
     * three entries for the cache and one entry for the counter, which (at 16 bits per entry)
     * stays under a 64 byte cache line size. 16 bits should be plenty for storing local ranks (2^16
     * is much bigger than any currently immaginable single-node without something like wildly
     * oversubscribed ranks as threads). */
    int16_t *first_poll_local_ranks;
    int next_poll_local_rank;

} MPIDI_POSIX_eager_fbox_control_t;

extern MPIDI_POSIX_eager_fbox_control_t MPIDI_POSIX_eager_fbox_control_global;

#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_CH4_SHM_POSIX_FBOX_GENERAL;
#endif

#define POSIX_FBOX_TRACE(...) \
    MPL_DBG_MSG_FMT(MPIDI_CH4_SHM_POSIX_FBOX_GENERAL,VERBOSE,(MPL_DBG_FDEST, __VA_ARGS__))

#endif /* POSIX_EAGER_FBOX_TYPES_H_INCLUDED */
