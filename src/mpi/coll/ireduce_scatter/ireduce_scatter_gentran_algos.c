/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#include "tsp_gentran.h"

/* instantiate ireduce_scatter recexch algorithms for the gentran transport */
#include "ireduce_scatter_tsp_recexch_algos_prototypes.h"
#include "ireduce_scatter_tsp_recexch_algos.h"
#include "ireduce_scatter_tsp_recexch_algos_undef.h"

#include "tsp_undef.h"
