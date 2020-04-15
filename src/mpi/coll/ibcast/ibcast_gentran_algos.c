/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#include "tsp_gentran.h"

/* instantiate ibcast tree algorithms for the gentran transport */
#include "ibcast_tsp_tree_algos_prototypes.h"
#include "ibcast_tsp_tree_algos.h"
#include "ibcast_tsp_tree_algos_undef.h"

/* instantiate ibcast ring algorithms for the gentran transport */
#include "ibcast_tsp_scatterv_allgatherv_algos_prototypes.h"
#include "ibcast_tsp_scatterv_allgatherv_algos.h"
#include "ibcast_tsp_scatterv_allgatherv_algos_undef.h"

#include "tsp_undef.h"
