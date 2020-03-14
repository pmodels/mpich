/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#include "tsp_gentran.h"

/* instantiate allgatherv linear algorithms for the gentran transport */
#include "ineighbor_allgatherv_tsp_linear_algos_prototypes.h"
#include "ineighbor_allgatherv_tsp_linear_algos.h"
#include "ineighbor_allgatherv_tsp_linear_algos_undef.h"

#include "tsp_undef.h"
