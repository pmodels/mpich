/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHM_SHMAM_EAGER_FBOX_TYPES_H_INCLUDED
#define SHM_SHMAM_EAGER_FBOX_TYPES_H_INCLUDED

#include <mpidimpl.h>
#include "mpidu_shm.h"

#define MPIDI_SHMAM_FBOX_DATA_LEN  (16 * 1024 - sizeof(uint64_t) - 2 * sizeof(int))
#define MPIDI_SHMAM_FBOX_THRESHOLD (MPIDI_SHMAM_FBOX_DATA_LEN)

typedef struct {

    volatile uint64_t flag;

    int is_header;
    int payload_sz;

    uint8_t payload[MPIDI_SHMAM_FBOX_DATA_LEN];

} MPIDI_SHMAM_fastbox_t;

typedef struct MPIDI_SHMAM_fbox_arrays {
    MPIDI_SHMAM_fastbox_t **in;
    MPIDI_SHMAM_fastbox_t **out;
} MPIDI_SHMAM_fbox_arrays_t;

typedef struct MPIDI_SHMAM_eager_fbox_control {

    MPIDU_shm_seg_t memory;
    MPIDU_shm_seg_info_t *seg;

    int num_seg;

    MPIDI_SHMAM_fbox_arrays_t mailboxes;

    MPIDU_shm_barrier_t *barrier;
    void *barrier_region;

    int my_rank;
    int my_grank;
    int num_local;

    int last_polled_grank;

} MPIDI_SHMAM_eager_fbox_control_t;

extern MPIDI_SHMAM_eager_fbox_control_t MPIDI_SHMAM_eager_fbox_control_global;

#endif /* SHM_SHMAM_EAGER_FBOX_TYPES_H_INCLUDED */
