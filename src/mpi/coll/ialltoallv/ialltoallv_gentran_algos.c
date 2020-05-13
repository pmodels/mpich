/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#include "tsp_gentran.h"

/* instantiate ialltoall scattered algorithms for the gentran transport */
#include "ialltoallv_tsp_scattered_algos_prototypes.h"
#include "ialltoallv_tsp_scattered_algos.h"
#include "ialltoallv_tsp_scattered_algos_undef.h"

#include "ialltoallv_tsp_blocked_algos_prototypes.h"
#include "ialltoallv_tsp_blocked_algos.h"
#include "ialltoallv_tsp_blocked_algos_undef.h"

/* instantiate ialltoallv  algorithms for the gentran transport */
#include "ialltoallv_tsp_inplace_algos_prototypes.h"
#include "ialltoallv_tsp_inplace_algos.h"
#include "ialltoallv_tsp_inplace_algos_undef.h"

#include "tsp_undef.h"
