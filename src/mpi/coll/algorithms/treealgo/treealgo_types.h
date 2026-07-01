/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TREEALGO_TYPES_H_INCLUDED
#define TREEALGO_TYPES_H_INCLUDED

#include <utarray.h>

typedef struct {
    int rank;
    int nranks;
    int parent;
    int num_children;
    UT_array *children;
} MPIR_Treealgo_tree_t;

#endif /* TREEALGO_TYPES_H_INCLUDED */
