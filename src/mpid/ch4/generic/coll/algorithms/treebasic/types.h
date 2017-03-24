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
#include "../tree/tree_def.h"

typedef struct COLL_comm_t {
    MPIR_Comm *mpir_comm;
    /*Although rank and size can be obtained from mpir_comm, 
     *storing them here for ease of accessibility*/
    int rank;
    int size;
    int tag;

    int max_k; /*value of radix k up to which we may run collective algorithms*/
    
    /*store kary and knomial trees*/
    COLL_tree_t *kary_tree;
    COLL_tree_t *knomial_tree;
    
    /*Communicators for multileader optimization*/
    MPIR_Comm *subcomm; /*subcommunicator to which I belong*/
    MPIR_Comm *subcomm_roots_comm;/*communicator of rank 0 in each subcomm, 
                                    this will be non-NULL only on rank 0 of comm*/
    bool is_subcomm; /*if a subcomm, do not create further subcomms*/

} COLL_comm_t;

typedef struct COLL_global_t {
} COLL_global_t;
