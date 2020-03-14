/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#include "tsp_gentran.h"

/* instantiate iallreduce recexch algorithms for the gentran transport */
#include "iallreduce_tsp_recexch_algos_prototypes.h"
#include "iallreduce_tsp_recexch_algos.h"
#include "iallreduce_tsp_recexch_algos_undef.h"

/* instantiate iallreduce tree algorithms for the gentran transport */
#include "iallreduce_tsp_tree_algos_prototypes.h"
#include "iallreduce_tsp_tree_algos.h"
#include "iallreduce_tsp_tree_algos_undef.h"

/* instantiate iallreduce ring algorithms for the gentran transport */
#include "iallreduce_tsp_ring_algos_prototypes.h"
#include "iallreduce_tsp_ring_algos.h"
#include "iallreduce_tsp_ring_algos_undef.h"

#include "iallreduce_tsp_recursive_exchange_common_prototypes.h"
#include "iallreduce_tsp_recursive_exchange_common.h"
#include "iallreduce_tsp_recursive_exchange_common_undef.h"

/* instantiate iallreduce recexch_reduce_scatter_recexch_allgatherv algorithms for the gentran transport */
#include "iallreduce_tsp_recexch_reduce_scatter_recexch_allgatherv_algos_prototypes.h"
#include "iallreduce_tsp_recexch_reduce_scatter_recexch_allgatherv_algos.h"
#include "iallreduce_tsp_recexch_reduce_scatter_recexch_allgatherv_algos_undef.h"

#include "tsp_undef.h"
