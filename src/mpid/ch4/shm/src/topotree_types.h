/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef TOPOTREE_TYPES_H_INCLUDED
#define TOPOTREE_TYPES_H_INCLUDED

/* Calculate tree information (parent, child) etc since it is a K-ary tree */
#define MPIDI_SHM_TOPOTREE_PARENT(X,i) ((X)->base[i*((X)->k+2)])
#define MPIDI_SHM_TOPOTREE_NUM_CHILD(X,i) ((X)->base[i*((X)->k+2)+1])
#define MPIDI_SHM_TOPOTREE_CHILD(X,i,c) ((X)->base[i*((X)->k+2)+2+c])

/* -1 means that topology trees breaks as package_leaders (socket_leaders) and per_package (per
 * socket). Tune this value if the topology aware tree needs to have the split at something other
 * than package level. Could be needed for future machines.
 * */
#define MPIDI_SHM_TOPOTREE_CUTOFF -1
#define MPIDI_SHM_TOPOTREE_DEBUG 0      /* Enable the logging information as well as the tree file per rank */

typedef struct MPIDI_SHM_topotree_t {
    int *base;
    int k;
    int n;
} MPIDI_SHM_topotree_t;

#endif /* TOPOTREE_TYPES_H_INCLUDED */
