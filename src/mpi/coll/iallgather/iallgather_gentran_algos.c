/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#include "tsp_gentran.h"

/* instantiate iallgather recexch algorithms for the gentran transport */
#include "iallgather_tsp_recexch_algos_prototypes.h"
#include "iallgather_tsp_recexch_algos.h"
#include "iallgather_tsp_recexch_algos_undef.h"

/* instantiate iallgather brucks algorithms for the gentran transport */
#include "iallgather_tsp_brucks_algos_prototypes.h"
#include "iallgather_tsp_brucks_algos.h"
#include "iallgather_tsp_brucks_algos_undef.h"

/* instantiate iallgather ring algorithms for the gentran transport */
#include "iallgather_tsp_ring_algos_prototypes.h"
#include "iallgather_tsp_ring_algos.h"
#include "iallgather_tsp_ring_algos_undef.h"

#include "tsp_undef.h"
