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

/* *INDENT-OFF* */
/* forward declaration of funcs structs defined in shared memory modules */
extern MPIDI_SHM_funcs_t MPIDI_SHM_src_funcs;
extern MPIDI_SHM_native_funcs_t MPIDI_SHM_native_src_funcs;

#ifndef SHM_INLINE
MPIDI_SHM_funcs_t *MPIDI_SHM_func = &MPIDI_SHM_src_funcs;
MPIDI_SHM_native_funcs_t *MPIDI_SHM_native_func = &MPIDI_SHM_native_src_funcs;
#else
MPIDI_SHM_funcs_t *MPIDI_SHM_func = NULL;
MPIDI_SHM_native_funcs_t *MPIDI_SHM_native_func = NULL;
#endif
/* *INDENT-ON* */
