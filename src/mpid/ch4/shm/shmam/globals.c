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

#include <mpidimpl.h>
#include "shmam_impl.h"
#include "shmam_types.h"
#include "shmam_eager.h"
#include "shmam_eager_impl.h"

MPIDI_SHM_Shmam_global_t MPIDI_SHM_Shmam_global = { 0 };
MPIDI_SHMAM_eager_funcs_t *MPIDI_SHMAM_eager_func = NULL;
