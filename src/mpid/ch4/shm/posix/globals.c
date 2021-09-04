/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "posix_impl.h"
#include "posix_types.h"

MPIDI_POSIX_global_t MPIDI_POSIX_global;

MPIDI_POSIX_eager_funcs_t *MPIDI_POSIX_eager_func = NULL;

MPL_atomic_uint64_t *MPIDI_POSIX_shm_limit_counter = NULL;
