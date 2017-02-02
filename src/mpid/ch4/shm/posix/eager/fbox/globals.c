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

#include "fbox_impl.h"
#include "fbox_types.h"

MPIDI_POSIX_eager_fbox_control_t MPIDI_POSIX_eager_fbox_control_global = { 0 };

MPL_dbg_class MPIDI_CH4_SHM_POSIX_FBOX_GENERAL;
