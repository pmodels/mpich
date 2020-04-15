/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#include "tsp_gentran.h"

/* instantiate ialltoall ring algorithms for the gentran transport */
#include "ialltoall_tsp_ring_algos_prototypes.h"
#include "ialltoall_tsp_ring_algos.h"
#include "ialltoall_tsp_ring_algos_undef.h"
/* instantiate ialltoall brucks algorithms for the gentran transport */
#include "ialltoall_tsp_brucks_algos_prototypes.h"
#include "ialltoall_tsp_brucks_algos.h"
#include "ialltoall_tsp_brucks_algos_undef.h"

/* instantiate ialltoall scattered algorithms for the gentran transport */
#include "ialltoall_tsp_scattered_algos_prototypes.h"
#include "ialltoall_tsp_scattered_algos.h"
#include "ialltoall_tsp_scattered_algos_undef.h"

#include "tsp_undef.h"
