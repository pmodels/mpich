/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

typedef struct COLL_comm_t {
    MPIR_Comm *mpir_comm;
    /*Although rank and size can be obtained from mpir_comm, 
     *storing them here for ease of accessibility*/
    int rank;
    int size;
    int tag;

    int max_k; /*value of radix k up to which we may run collective algorithms*/
    int *log_size; /*stores floor(log_k(nranks)) values for all possible values of k*/

    /*store kary tree*/
    int *kary_parent; /*stores parent in the tree for each value of k*/
    int *kary_nchildren; /*number of children in the tree for each value of k*/
    int **kary_children; /*list of children in each tree*/

    /*store knomial trees (same arrays as kary trees above*/
    int *knomial_parent;
    int *knomial_nchildren;
    int **knomial_children;
} COLL_comm_t;

typedef struct COLL_global_t {
} COLL_global_t;
