/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
