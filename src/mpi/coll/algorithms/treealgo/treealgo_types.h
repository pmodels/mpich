/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef TREEALGO_TYPES_H_INCLUDED
#define TREEALGO_TYPES_H_INCLUDED

typedef struct {
    int rank;
    int nranks;
    int parent;
    int num_children;
    UT_array *children;
} MPII_Treealgo_tree_t;

#endif /* TREEALGO_TYPES_H_INCLUDED */
