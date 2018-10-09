/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *  See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *   Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"

#include "tsp_gentran.h"

/* instantiate ialltoallw blocked algorithms for the gentran transport */
#include "ialltoallw_tsp_blocked_algos_prototypes.h"
#include "ialltoallw_tsp_blocked_algos.h"
#include "ialltoallw_tsp_blocked_algos_undef.h"

/* instantiate ialltoallw inplace algorithms for the gentran transport */
#include "ialltoallw_tsp_inplace_algos_prototypes.h"
#include "ialltoallw_tsp_inplace_algos.h"
#include "ialltoallw_tsp_inplace_algos_undef.h"

#include "tsp_undef.h"
