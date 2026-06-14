/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpidimpl.h"
#include "posix_impl.h"
#include "posix_types.h"

MPIDI_POSIX_global_t MPIDI_POSIX_global;

MPIDI_POSIX_eager_funcs_t *MPIDI_POSIX_eager_func = NULL;

MPL_atomic_uint64_t *MPIDI_POSIX_shm_limit_counter = NULL;
