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

#include "posix_impl.h"
#include "posix_types.h"

MPIDI_POSIX_global_t MPIDI_POSIX_global = { 0 };

MPIDI_POSIX_eager_funcs_t *MPIDI_POSIX_eager_func = NULL;

MPL_dbg_class MPIDI_CH4_SHM_POSIX_GENERAL;
