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
#ifndef MPIDIG_GLUE_H_INCLUDED
#define MPIDIG_GLUE_H_INCLUDED

#include "ch4_impl.h"

#ifndef MPIDI_CH4_EXCLUSIVE_SHM

#define MPIDI_Generic_am(func_, is_local_, ...) \
    MPIDI_NM_am_ ## func_(__VA_ARGS__)

#else /* MPIDI_CH4_EXCLUSIVE_SHM */

#define MPIDI_Generic_am(func_, is_local_, ...)         \
    (is_local_) ?                                       \
                MPIDI_SHM_am_ ## func_(__VA_ARGS__) :   \
                MPIDI_NM_am_ ## func_(__VA_ARGS__)

#endif /* MPIDI_CH4_EXCLUSIVE_SHM */

#endif /* MPIDIG_GLUE_H_INCLUDED */
