/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_SHM_SEG_H_INCLUDED
#define MPIDU_SHM_SEG_H_INCLUDED

#include "mpidu_init_shm.h"

typedef struct MPIDU_shm_seg {
    size_t segment_len;
    /* Handle to shm seg */
    MPL_shm_hnd_t hnd;
    /* Pointers */
    char *base_addr;
    /* Misc */
    char file_name[MPIDU_SHM_MAX_FNAME_LEN];
    int base_descs;
    int symmetrical;
} MPIDU_shm_seg_t;

#endif /* MPIDU_SHM_SEG_H_INCLUDED */
