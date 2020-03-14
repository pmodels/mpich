/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#include "tsp_gentran.h"

/* instantiate iallgatherv recexch algorithms for the gentran transport */
#include "iallgatherv_tsp_recexch_algos_prototypes.h"
#include "iallgatherv_tsp_recexch_algos.h"
#include "iallgatherv_tsp_recexch_algos_undef.h"

/* instantiate iallgatherv ring algorithms for the gentran transport */
#include "iallgatherv_tsp_ring_algos_prototypes.h"
#include "iallgatherv_tsp_ring_algos.h"
#include "iallgatherv_tsp_ring_algos_undef.h"

/* instantiate iallgatherv brucks algorithms for the gentran transport */
#include "iallgatherv_tsp_brucks_algos_prototypes.h"
#include "iallgatherv_tsp_brucks_algos.h"
#include "iallgatherv_tsp_brucks_algos_undef.h"

#include "tsp_undef.h"
