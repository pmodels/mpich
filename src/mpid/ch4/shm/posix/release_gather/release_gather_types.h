/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef RELEASE_GATHER_TYPES_H_INCLUDED
#define RELEASE_GATHER_TYPES_H_INCLUDED

#include "treealgo_types.h"
#include "common/zm_common.h"

typedef enum {
    MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST,
    MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE,
} MPIDI_POSIX_release_gather_opcode_t;

typedef enum MPIDI_POSIX_release_gather_tree_type_t {
    MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KARY,
    MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KNOMIAL_1,
    MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KNOMIAL_2,
} MPIDI_POSIX_release_gather_tree_type_t;

typedef struct MPIDI_POSIX_release_gather_comm_t {
    MPIR_Treealgo_tree_t bcast_tree, reduce_tree;
    MPL_shm_hnd_t shm_flags_handle, shm_bcast_buf_handle, shm_reduce_buf_handle;
    int flags_shm_size;
    uint64_t gather_state, release_state;

    volatile void *flags_addr, *bcast_buf_addr, *reduce_buf_addr;
    volatile void **child_reduce_buf_addr;
    volatile zm_atomic_uint_t *release_flag_addr, *gather_flag_addr;
} MPIDI_POSIX_release_gather_comm_t;

#endif /* RELEASE_GATHER_TYPES_H_INCLUDED */
