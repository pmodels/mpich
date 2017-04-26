/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

typedef struct COLL_child_range_t {
    int startRank;
    int endRank;
} COLL_child_range_t;

typedef struct COLL_tree_t {
    int rank;
    int nranks;
    int parent;
    int numRanges;
    COLL_child_range_t children[COLL_MAX_TREE_BREADTH];
} COLL_tree_t;
