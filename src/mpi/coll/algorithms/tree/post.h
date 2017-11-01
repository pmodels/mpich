/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#define COLL_TREE_RADIX_DEFAULT 2
#define COLL_MAX_TREE_BREADTH  16
/* algrithm namespace */
#define COLL_NAME TREE_
/* algorithm namespace in lower case */
#define COLL_NAME_LC tree
#include "../../src/coll_namespace_def.h"
#include "./coll.h"
#include "../../src/coll_namespace_undef.h"
#undef COLL_TREE_RADIX_DEFAULT
#undef COLL_MAX_TREE_BREADTH
