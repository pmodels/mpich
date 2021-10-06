/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
