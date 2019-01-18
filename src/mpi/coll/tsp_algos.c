/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
*  (C) 2019 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.

*  Portions of this code were written by Intel Corporation.
*  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
*  to Argonne National Laboratory subject to Software Grant and Corporate
*  Contributor License Agreement dated February 8, 2012.
*/

/* Include guard is intentionally omitted.
* include specific tsp namespace before include this file
* e.g. #include "tsp_gentran.h"
*/

/* Transport based i-collective algorithm functions.
* The actual implementation code is inside icoll/icoll_tsp_xxx_algos.h
*/

#include "tsp_gentran.h"
#include "coll_tsp_algos.h"
#include "ibcast/ibcast_tsp_tree_algos.h"
#include "ibcast/ibcast_tsp_scatter_recexch_allgather_algos.h"
#include "igather/igather_tsp_tree_algos.h"
#include "iscatter/iscatter_tsp_tree_algos.h"
#include "iallgather/iallgather_tsp_brucks_algos.h"
#include "iallgather/iallgather_tsp_recexch_algos.h"
#include "iallgatherv/iallgatherv_tsp_ring_algos.h"
#include "iallgatherv/iallgatherv_tsp_recexch_algos.h"
#include "ireduce/ireduce_tsp_tree_algos.h"
#include "iallreduce/iallreduce_tsp_tree_algos.h"
#include "iallreduce/iallreduce_tsp_recexch_algos.h"
#include "ireduce_scatter/ireduce_scatter_tsp_recexch_algos.h"
#include "ireduce_scatter_block/ireduce_scatter_block_tsp_recexch_algos.h"
#include "tsp_undef.h"
